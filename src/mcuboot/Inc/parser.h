/**
 * @file parser.h
 * @brief Parser module based on mcuboot library.
*/

#include <bootutil/image.h>

#ifndef _PARSER_H_
#define _PARSER_H_

#define TLV_FW_DESCRIPTOR_BOOTLOADER ("BOOTLOADER")
#define TLV_FW_DESCRIPTOR_MAINAPP ("APPLICATION")
#define MAX_LEN_TLV_DESCRIPTOR (15U)
#define TLV_LEN_PUBLIC_KEY (32U)
#define PROTECTED_TLV (true)

/** Bootable data about downloaded image 
 *  from secondary slot in external flash. */
typedef struct {
    const struct flash_area *fap;               /*!< Pointer to flash area to read bootable data. */
    struct image_header hdr;                    /*!< Header for image that stored in flash area. */
    struct image_tlv_iter image_iter;           /*!< Struct to save tlv parsing progress. */
    char cur_fw_desc[MAX_LEN_TLV_DESCRIPTOR];   /*!< Array to save current firmware descriptor. */
    uint32_t security_cnt;                      /*!< Security counter value. */
    struct image_dependency dep;                /*!< Dependency for the image. */
} bootutil_data_t;

/**
 * @brief Read the new public key from the flash area from bootable data.
 * @param[in]   boot_data Pointer to @ref bootutil_data_t.
 * @param[out]  outBuff Output buffer to copy TLV
 * @param       buff_len Buffer length
 * @return int. 1 if key is not found or length is not supported. Zero otherwise.
 */
int parser_readNewPublicKey(bootutil_data_t *boot_data, uint8_t *outBuff, uint16_t buff_len);

/**
 * @brief Get image size according to the boot data.
 * @param boot_data Pointer to @ref bootutil_data_t.
 * @return uint32_t Full image size including all TLVs fields. Zero on error.
 */
uint32_t parser_getImageSize(bootutil_data_t *boot_data);

/** 
 * @brief Compare descriptor in bootable data with main app descriptor.
 * @param[in]   boot_data Pointer to @ref bootutil_data_t.
 * @return true if there is main app descriptor, false otherwise.
 */
bool parser_isMainAppDescriptor(bootutil_data_t *boot_data);

/**
 * @brief Compare descriptor in bootable data with bootloader descriptor.
 * @param[in]   boot_data Pointer to @ref bootutil_data_t.
 * @return true if there is bootloader descriptor, false otherwise.
 */
bool parser_isBootloaderDescriptor(bootutil_data_t *boot_data);

/**
 * @brief Checks downgrade prevention conditions.
 * @param  primary_slot     Pointer to @ref bootutil_data_t for primary slot
 * @param  secondary_slot   Pointer to @ref bootutil_data_t for secondary slot with update
 * @return  true if update update has an older security, false if update has same or newer security
 * counter.
 */
bool parser_isDowngradeVersion(bootutil_data_t *primary_slot, bootutil_data_t *secondary_slot);

/**
 * @brief Compare versions in primary and secondary slots.
 * @param  primary_slot     Pointer to @ref bootutil_data_t for primary slot
 * @param  secondary_slot   Pointer to @ref bootutil_data_t for secondary slot with update
 * @return  true if update has same version number, false otherwise.
 */
bool parser_isVersionEqual(bootutil_data_t *primary_slot, bootutil_data_t *secondary_slot);

/**
 * @brief Verify dependency of an image in a slot against the bootloader.
 * @param[in]   bootloader Pointer to @ref bootutil_data_t for the bootloader slot.
 * @param[in]   slot Pointer to @ref bootutil_data_t for the slot being verified.
 * @return  true if dependency is satisfied, false otherwise.
 */
bool parser_verifyDependency(bootutil_data_t *bootloader, bootutil_data_t *slot);

/**
 * @brief initialize data for boot.
 * @param[out]  boot_data Pointer to @ref bootutil_data_t.
 * @param       flash_area_id @ref flash_area_id_e.
 * @return int. 0 on success, BOOT_E code otherwise (see bootutil_public.h).
 */
int parser_initBootData(bootutil_data_t *boot_data, uint8_t flash_area_id);

/**
 * @brief Close flash area and deinit necessary data for boot.
 * @param[in]   boot_data Pointer to @ref bootutil_data_t.
 * @return None
 */
void parser_deinitBootData(bootutil_data_t *boot_data);

#endif /* _PARSER_H_ */