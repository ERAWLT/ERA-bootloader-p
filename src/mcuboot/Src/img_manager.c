/**
 * @file    img_manager.c
 * @brief HWLT-bootloader image manager module.
 */

#include "img_manager.h"
#include "board_partial_download.h"

#include "sysflash/sysflash.h"
#include "bootutil/bootutil.h"
#include <bootutil/fault_injection_hardening.h>
#include <bootutil/image.h>
#include "bootutil_priv.h"

#include "elog.h"
#define LOG_MODULE_NAME     "bootloader"
#define LOG_INF(...)        elog_i(LOG_MODULE_NAME, __VA_ARGS__)
#define LOG_WRN(...)        elog_w(LOG_MODULE_NAME, __VA_ARGS__)
#define LOG_ERR(...)        elog_e(LOG_MODULE_NAME, __VA_ARGS__)

static bool update_MF_passed = false;
static int eraseBootStatus(const struct flash_area *fap);

void imgManager_initiateUpdate(const struct flash_area *fap)
{
    if (fap == NULL) {
        LOG_ERR("NULL flash area when initiating update.");
        return;
    }
    int rc;
    struct boot_swap_state state = {0};
    rc = boot_read_swap_state(fap, &state);
    if (rc != 0) {
        LOG_ERR("boot_read_swap_state = %d", rc);
        if (eraseBootStatus(fap) != 0) {
            LOG_ERR("Error occurred during erase boot status!");
            return;
        }
    }
    
    if (state.magic == BOOT_MAGIC_GOOD) {
        LOG_INF("Magic value in boot status already exist.");
        return;
    }
    
    if (state.magic != BOOT_MAGIC_UNSET) {
        if (eraseBootStatus(fap) != 0) {
            LOG_ERR("Error occurred during erase boot status!");
        }
    }
    LOG_INF("Setup magic value to initialize new update. ");
    rc = boot_write_magic(fap);
    if (rc != 0) {
        LOG_ERR("Error writing magic value!");
    }
}

void imgManager_setCopyDone(const struct flash_area *fap)
{
    if (fap == NULL) {
        LOG_ERR("NULL flash area when copy done.");
        return;
    }
    int rc;
    struct boot_swap_state state = {0};
    rc = boot_read_swap_state(fap, &state);
    if (rc != 0) {
        LOG_ERR("boot_read_swap_state = %d", rc);
        return;
    }
    if (state.magic == BOOT_MAGIC_GOOD) {
        if (state.copy_done == BOOT_MAGIC_UNSET) {
            LOG_INF("Set flag - copy done.");
            boot_write_copy_done(fap);
        }
    }
    update_MF_passed = true;
}

bool imgManager_isUpdateMFpassed(void)
{
    return update_MF_passed;
}

void imgManager_setMagicMF(void)
{
    const struct flash_area *fap;
    int rc;

    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY(IMG_ID_MAIN_APP), &fap);
    if (rc != 0) {
        LOG_ERR("flash_area_open = %d", rc);
        return;
    }

    struct boot_swap_state state = {0};
    rc = boot_read_swap_state(fap, &state);
    if (rc != 0) {
        LOG_ERR("boot_read_swap_state = %d", rc);
        return;
    }
    
    if (state.magic == BOOT_MAGIC_UNSET) {
        LOG_INF("Set the magic value to Primary image state.");
        boot_write_magic(fap);
    }

    if (state.copy_done == BOOT_MAGIC_UNSET) {
        LOG_INF("Set the copy done value to Primary image state.");
        boot_write_copy_done(fap);
    }
    
    update_MF_passed = true;
    flash_area_close(fap);
}

void imgManager_setOkMF(void)
{
    const struct flash_area *fap;
    int rc;

    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY(IMG_ID_MAIN_APP), &fap);
    if (rc != 0) {
        LOG_ERR("flash_area_open = %d", rc);
        return;
    }

    struct boot_swap_state state = {0};
    rc = boot_read_swap_state(fap, &state);

    if (rc != 0) {
        LOG_ERR("boot_read_swap_state = %d", rc);
    } else if (state.magic == BOOT_MAGIC_GOOD) {
        if (state.image_ok == BOOT_FLAG_UNSET) {
            LOG_INF("Primary slot will marked as valid.");
            boot_write_image_ok(fap);
        }
    }
    
    flash_area_close(fap);
}

void imgManager_eraseUpdateMF(void)
{
    const struct flash_area *fap;
    int rc = 0;
    rc = flash_area_open(FLASH_AREA_IMAGE_SECONDARY(IMG_ID_MAIN_APP), &fap);
    if (rc != 0) {
        LOG_ERR("slot %d: flash_area_open = %d", FLASH_AREA_IMAGE_SECONDARY(IMG_ID_MAIN_APP), rc);
    }
    flash_area_reset_start(fap);
    imgManager_eraseArea(fap);
    flash_area_close(fap);
}

void imgManager_eraseBadMF(void)
{
    const struct flash_area *fap;
    int rc;
    
    LOG_INF("Primary slot should be cleared.");
    rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY(IMG_ID_MAIN_APP), &fap);
    if (rc != 0) {
        LOG_ERR("slot %d: flash_area_open = %d", FLASH_AREA_IMAGE_PRIMARY(IMG_ID_MAIN_APP), rc);
        return;
    }
    
    imgManager_eraseArea(fap);

    flash_area_close(fap);
}

void imgManager_eraseArea(const struct flash_area *fap)
{
    if (fap == NULL) {
        LOG_ERR("NULL flash area when erasing slot.");
        return;
    }
    int rc;
    struct image_header hdr = {0};

    rc = flash_area_read(fap, 0, &hdr, sizeof(hdr));
    if (rc != 0) {
        LOG_INF("Error to read header from first sector in slot %d!", fap->fa_id);
        /* Erase for header sector */
        flash_area_erase(fap, 0, FLASH_SECTOR_SIZE);
    } else {
        uint8_t *b = (uint8_t *)&hdr;
        uint8_t erased_val = flash_area_erased_val(fap);

        for (uint32_t i = 0; i < sizeof(hdr); i++) {
            if (erased_val != b[i]) {
                LOG_INF("First sector in slot %d will be erased now!", fap->fa_id);
                /* Erase for header sector */
                flash_area_erase(fap, 0, FLASH_SECTOR_SIZE);
                break;
            }
        }
    }

    struct boot_swap_state state = {0};
    rc = boot_read_swap_state(fap, &state);
    if (rc != 0 || state.magic != BOOT_MAGIC_UNSET) {
        LOG_INF("Last sector in slot %d will be erased now!", fap->fa_id);
        /* Erase for swap info sector is also required */
        flash_area_erase(fap, fap->fa_size-FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE);
    }

    if (fap->fa_id == FLASH_AREA_IMAGE_SECONDARY_ID) {
        partialDownload_resetProgress(PARTIAL_DOWNLOAD_MODE_FIRMWARE);
    }
}

imgManager_status_MF_e imgManager_getStatusMF(void)
{
    imgManager_status_MF_e primary_status = IMG_MANAGER_MF_OK;
    const struct flash_area *fap;
    int rc;

    do {
        rc = flash_area_open(FLASH_AREA_IMAGE_PRIMARY(IMG_ID_MAIN_APP), &fap);
        if (rc != 0) {
            LOG_ERR("flash_area_open = %d", rc);
            primary_status = IMG_MANAGER_MF_READ_FAILED;
            break;
        }

        struct boot_swap_state state = {0};
        rc = boot_read_swap_state(fap, &state);
        if (rc != 0) {
            LOG_ERR("boot_read_swap_state = %d", rc);
            primary_status = IMG_MANAGER_MF_READ_FAILED;
            break;
        }

        LOG_INF("status flags:\nmagic=%x\ncopy_done=%x\nimage_num=%x\nimage_ok=%x\nswap_type=%x",
                state.magic, state.copy_done, state.image_num, state.image_ok, state.swap_type);

        if (state.magic == BOOT_MAGIC_BAD || state.copy_done == BOOT_FLAG_BAD ||
            state.image_ok == BOOT_FLAG_BAD) {
            primary_status = IMG_MANAGER_MF_READ_FAILED;
            break;
        }

        if (state.magic == BOOT_MAGIC_GOOD) {
            if (state.copy_done == BOOT_FLAG_SET) {
                if (state.image_ok != BOOT_FLAG_SET) {
                    primary_status = IMG_MANAGER_MF_TEST_IN_PROGRESS;
                }
            } else {
                LOG_WRN("Copy done flag - failed!");
                primary_status = IMG_MANAGER_MF_COPY_DONE_UNSET;
            }
        } else {
            LOG_WRN("Magic of image state is not exist");
            primary_status = IMG_MANAGER_MF_MAGIC_UNSET;
        }
    } while (0);

    flash_area_close(fap);
    return primary_status;
}

/**
 * @brief Erase image boot status in a given flash area.
 * @param[in]   fap Flash area to be erased.
 * @return int. Zero on success, or negative value in case of error.
 */
static int eraseBootStatus(const struct flash_area *fap)
{
    if (fap == NULL) {
        LOG_ERR("NULL flash area when erasing the boot status.");
        return -1;
    }
    uint32_t trailer_sz = boot_trailer_sz(flash_area_align(fap));
    uint32_t off = boot_status_off(fap);
    return flash_area_erase(fap, off, trailer_sz);
}
