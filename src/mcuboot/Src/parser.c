/**
 * @file parser.c
 * @brief Parser module based on mcuboot library.
*/
#include "parser.h"


#include "sysflash/sysflash.h"
#include "bootutil/bootutil.h"
#include <bootutil/fault_injection_hardening.h>
#include "bootutil_priv.h"
#include "flash_map_backend/flash_map_backend.h"

#include "elog.h"
#define LOG_MODULE_NAME     "parser"
#define LOG_INF(...)        elog_i(LOG_MODULE_NAME, __VA_ARGS__)
#define LOG_ERR(...)        elog_e(LOG_MODULE_NAME, __VA_ARGS__)


static int getTlvValue(bootutil_data_t *boot_data, uint16_t tlv_type, bool tlv_area,
                       uint8_t *outBuff, uint16_t buff_len);
static inline int readFirmwareDescriptor(bootutil_data_t *boot_data, uint8_t *outBuff,
                                         uint16_t buff_len);
static inline int readSecurityCounter(bootutil_data_t *boot_data, uint8_t *outBuff,
                                      uint16_t buff_len);
static inline int readDependencies(bootutil_data_t *boot_data, uint8_t *outBuff,
                                         uint16_t buff_len);
                                         
int parser_initBootData(bootutil_data_t *boot_data, uint8_t flash_area_id)
{
    if (boot_data == NULL) {
        return BOOT_EBADARGS;
    }

    int rc = flash_area_open(flash_area_id, &boot_data->fap);
    if (rc != 0) {
        LOG_ERR("fail: flash_area_open");
        return BOOT_EFLASH_SEC;
    }

    if (boot_data->fap == NULL) {
        LOG_ERR("fail: fap");
        return BOOT_EBADARGS;
    }

    rc = flash_area_read(boot_data->fap, 0, &boot_data->hdr, sizeof(boot_data->hdr));
    if (rc != 0) {
        LOG_ERR("fail in slot %d: flash_area_read", boot_data->fap->fa_id);
        return BOOT_EFLASH_SEC;
    }

    if (boot_data->hdr.ih_magic != IMAGE_MAGIC) {
        LOG_INF("image header magic doesn't match in slot %d", boot_data->fap->fa_id);
        return BOOT_EBADIMAGE;
    }

    /* Images always have protected TLV. 
     * The firmware TLV is in the protected part of the TLV area. */
    if (boot_data->hdr.ih_protect_tlv_size == 0) {
        LOG_INF("bad image in slot %d: ih_protect_tlv_size equal zero", boot_data->fap->fa_id);
        return BOOT_EBADIMAGE;
    }

    /* Read the firmware descriptor */
    if (readFirmwareDescriptor(boot_data, (uint8_t *)boot_data->cur_fw_desc, \
                               MAX_LEN_TLV_DESCRIPTOR)) {
        LOG_INF("bad image in slot %d: fw descriptor doesn't exist", boot_data->fap->fa_id);
        return BOOT_EBADIMAGE;
    }

    /* Read the security counter */
    if (readSecurityCounter(boot_data, (uint8_t *)&boot_data->security_cnt,
                            sizeof(boot_data->security_cnt))) {
        LOG_INF("bad image in slot %d: security counter doesn't exist", boot_data->fap->fa_id);
        return BOOT_EBADIMAGE;
    }

    /* Read the dependencies if exist */
    if (readDependencies(boot_data, (uint8_t *)&boot_data->dep,
                            sizeof(boot_data->dep))) {
        LOG_INF("Dependencies doesn't exist", boot_data->fap->fa_id);
        memset(&boot_data->dep, 0, sizeof(boot_data->dep));
    }

    return 0;
}

void parser_deinitBootData(bootutil_data_t *boot_data)
{
    if (boot_data->fap != NULL) {
        flash_area_close(boot_data->fap);
    }
}

uint32_t parser_getImageSize(bootutil_data_t *boot_data)
{
    if (boot_data != NULL && boot_data->fap != NULL) {
        return boot_data->image_iter.tlv_end;
    }
    return 0;
}

bool parser_isMainAppDescriptor(bootutil_data_t *boot_data)
{
    size_t len = strlen(boot_data->cur_fw_desc);
    return (strncmp(boot_data->cur_fw_desc, TLV_FW_DESCRIPTOR_MAINAPP, \
                    MAX_LEN_TLV_DESCRIPTOR) == 0) && \
           (sizeof(TLV_FW_DESCRIPTOR_MAINAPP) - 1 == len);
}

bool parser_isBootloaderDescriptor(bootutil_data_t *boot_data)
{
    size_t len = strlen(boot_data->cur_fw_desc);
    return (strncmp(boot_data->cur_fw_desc, TLV_FW_DESCRIPTOR_BOOTLOADER, \
                    MAX_LEN_TLV_DESCRIPTOR) == 0) && \
           (sizeof(TLV_FW_DESCRIPTOR_BOOTLOADER) - 1 == len);
}

bool parser_isVersionEqual(bootutil_data_t *primary_slot, bootutil_data_t *secondary_slot)
{
    if (primary_slot != NULL && secondary_slot != NULL) {
        return memcmp((void *)&primary_slot->hdr.ih_ver, (void *)&secondary_slot->hdr.ih_ver,
                      sizeof(struct image_version)) == 0;
    }
    return false;
}

bool parser_isDowngradeVersion(bootutil_data_t *primary_slot, bootutil_data_t *secondary_slot)
{
    if (primary_slot != NULL && secondary_slot != NULL) {
        return primary_slot->security_cnt > secondary_slot->security_cnt;
    }
    return false;
}


bool parser_verifyDependency(bootutil_data_t *bootloader, bootutil_data_t *slot)
{
    bool verify_passed = false;
    if (bootloader != NULL && slot != NULL) {
        if (bootloader->dep.image_min_version.iv_major < slot->hdr.ih_ver.iv_major ||
            (bootloader->dep.image_min_version.iv_major == slot->hdr.ih_ver.iv_major &&
             (bootloader->dep.image_min_version.iv_minor < slot->hdr.ih_ver.iv_minor ||
              (bootloader->dep.image_min_version.iv_minor == slot->hdr.ih_ver.iv_minor &&
               (bootloader->dep.image_min_version.iv_revision < slot->hdr.ih_ver.iv_revision ||
                (bootloader->dep.image_min_version.iv_revision == slot->hdr.ih_ver.iv_revision &&
                 bootloader->dep.image_min_version.iv_build_num <= slot->hdr.ih_ver.iv_build_num)))))) {
            verify_passed = true;
        }
    }
    return verify_passed;
}

int parser_readNewPublicKey(bootutil_data_t *boot_data, uint8_t *outBuff, uint16_t buff_len)
{
    return buff_len != getTlvValue(boot_data, IMAGE_TLV_NEW_PUBKEY, \
                                   PROTECTED_TLV, outBuff, buff_len);
}

/**
 * @brief Read the firmware descriptor TLV from the flash area
 * @param[in]   boot_data Pointer to @ref bootutil_data_t.
 * @param[out]  outBuff Output buffer to copy TLV
 * @param       buff_len Buffer length
 * @return int. 1 if not found or length is not supported. Zero otherwise.
 */
static inline int readFirmwareDescriptor(bootutil_data_t *boot_data, uint8_t *outBuff,
                                         uint16_t buff_len)
{
    return getTlvValue(boot_data, IMAGE_TLV_FW_DESCRIPTOR, PROTECTED_TLV, outBuff, buff_len) <= 0;
}

/**
 * @brief Read the security counter TLV from the flash area
 * @param[in]   boot_data Pointer to @ref bootutil_data_t.
 * @param[out]  outBuff Output buffer to copy TLV
 * @param       buff_len Buffer length
 * @return int. 1 if not found or length is not supported. Zero otherwise.
 */
static inline int readSecurityCounter(bootutil_data_t *boot_data, uint8_t *outBuff,
                                         uint16_t buff_len)
{
    return getTlvValue(boot_data, IMAGE_TLV_SEC_CNT, PROTECTED_TLV, outBuff, buff_len) <= 0;
}

/**
 * @brief Read the dependency TLV from the flash area
 * @param[in]   boot_data Pointer to @ref bootutil_data_t.
 * @param[out]  outBuff Output buffer to copy TLV
 * @param       buff_len Buffer length
 * @return int. 1 if not found or length is not supported. Zero otherwise.
 */
static inline int readDependencies(bootutil_data_t *boot_data, uint8_t *outBuff,
                                         uint16_t buff_len)
{
    return getTlvValue(boot_data, IMAGE_TLV_DEPENDENCY, PROTECTED_TLV, outBuff, buff_len) <= 0;
}

/**
 * @brief Get the Tlv Value from the flash area from bootable data.
 * @param[in]   boot_data Pointer to @ref bootutil_data_t.
 * @param       tlv_type tlv type
 * @param       tlv_protected is tlv in protected area or no. 
 * @param[out]  outBuff output buffer to copy tlv value
 * @param       buff_len buffer length
 * @return int -1 if not found or length is not supported. Length of TLV otherwise
 */
static int getTlvValue(bootutil_data_t *boot_data, uint16_t tlv_type, bool tlv_protected,
                       uint8_t *outBuff, uint16_t buff_len)
{
    struct image_tlv_iter it = {0};
    uint32_t off = 0;
    uint16_t len = 0;

    int rc = bootutil_tlv_iter_begin(&boot_data->image_iter, &boot_data->hdr, boot_data->fap,
                                     tlv_type, tlv_protected);
    if (rc || it.tlv_end > bootutil_max_image_size(boot_data->fap)) {
        return -1;
    }
    
    rc = bootutil_tlv_iter_next(&boot_data->image_iter, &off, &len, NULL);
    if (rc || buff_len < len) {
        /* TLV has not been found or length is not supported. */
        return -1;
    }

    rc = LOAD_IMAGE_DATA(&boot_data->hdr, boot_data->fap, off, outBuff, len);
    if (rc != 0) {
        return -1;
    }

    return len;
}
