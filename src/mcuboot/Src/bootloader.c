/**
 * @file    bootloader.c
 * @brief HWLT-bootloader logic, based on mcuboot. Allow to update main-firmware
 *  and boot it. Allow to share image of new bootloader to bootstrapper
 *  to update it. Parser for external flash is also included.
 */

#include "bootloader.h"

#include "sysflash/sysflash.h"
#include "bootutil/bootutil.h"
#include "bootutil_priv.h"
#include "flash_map_backend/flash_map_backend.h"

#include "board.h"
#include "board_pwr.h"
#include "board_batt_control.h"
#include "board_backup_sram.h"
#include "board_internal_memory.h"
#include "board_partial_download.h"
#include "board_flash.h"

#include "endOfWork_manager.h"
#include "img_manager.h"
#include "parser.h"
#include "mail_engine.h"
#include "libse.h" /* To replace public key */
#include "secure.h"

#include "task_gui.h"
#include "boot_app.h"

#include "shell_bootloader.h"
#include "cmsis_os.h" /* To use delay if needed */
#include "elog.h"

#define BATTERY_LEVEL_TO_FW_UPDATE (30U) /*!< at 5 percent higher then bootstrapper level */

#define LOG_MODULE_NAME "bootloader"
#define LOG_INF(...)    elog_i(LOG_MODULE_NAME, __VA_ARGS__)
#define LOG_WRN(...)    elog_w(LOG_MODULE_NAME, __VA_ARGS__)
#define LOG_ERR(...)    elog_e(LOG_MODULE_NAME, __VA_ARGS__)

#define CLASSIC_BOOT   (false)
#define RESTORE_UPDATE (true)

#define JUMP_TO_BS_VIA_REBOOT

/** When this flag is set, bootloader initiated update via mcuboot.
 *  So after call @ref boot_go function, in good case we should found
 *  IMG_MANAGER_MF_TEST_IN_PROGRESS (see @ref imgManager_getStatusMF).
 */
static bool update_via_mcuboot_was_initiated = false;

/** When this flag is active update always allowed
 *  without user agreements. */
static bool MF_boot_failed = false;

/**
 * MCUboot response instance provided by the boot loader code; indicates where to jump
 * to execute the main image.
 */
struct target_instanse_s {
    struct boot_rsp rsp;
    bool inited;
} boot_target_instanse = {.inited = false};

/** Struct type of current update progress */
typedef struct {
    uint32_t operation_size;
    uint32_t operation_progress;
    const int operation_percent;
} bootloader_updateProgress_s;

/** Current update progress */
bootloader_updateProgress_s update_operation_progress[BOOTLOADER_UPDATE_OPERATION_NUM] = {
    [BOOTLOADER_UPDATE_OPERATION_VERIFY] = {
        .operation_percent = 10,
        .operation_size = 1,    /* Const for verify */
    },
    [BOOTLOADER_UPDATE_OPERATION_ERASE_PRIMARY] = {
        .operation_percent = 20,
    },
    [BOOTLOADER_UPDATE_OPERATION_COPY] = {
        .operation_percent = 50,
    },
    [BOOTLOADER_UPDATE_OPERATION_ERASE_SECONDARY] = {
        .operation_percent = 20,
    },
};

/** Local result for each bootloader_letStart method call */
static bootloader_status_e BL_status = BOOTLOADER_OK;

/** The last status of the execution bootloader (useful if MF is still valid). */
static bootloader_status_e BL_downloadStatus = BOOTLOADER_OK;

static bool isUpdateAllowed(void);
static int initiateUpdateViaMcuboot(bootutil_data_t *boot_data);
static int checkDownloadedUpdate(void);
static bool verifyDepencency(void);

static bootloader_status_e incomingMessageHandler(void);
static int newBootloaderHandler(bootutil_data_t *boot_data);
static int newMainFirmwareHandler(bootutil_data_t *boot_data);
static void BootFailedHandler(bool tryToRestoreUpdate);
static bool isMFReadyToStart(void);

static inline uint8_t get_current_level();

bool bootloader_updateInitiated(void)
{
    return update_via_mcuboot_was_initiated;
}

bootloader_status_e bootloader_init(void)
{
    REGISTER_SHELL_FOR_BOOTLOADER;
    TEST_WAIT_CONSOLE();

    secure_init();

    mailEngine_readMsgToBL();
    BL_downloadStatus = incomingMessageHandler();
    return BL_downloadStatus;
}

bootloader_status_e bootloader_letStart(void)
{
    /* clear flags before */
    BL_status = BL_downloadStatus = BOOTLOADER_OK;
    boot_target_instanse.inited = false;
    update_via_mcuboot_was_initiated = false;
    MF_boot_failed = false;

    if (checkDownloadedUpdate()) {
        LOG_WRN("Update doesn't exist in external flash.");
    }

    if (BL_status != BOOTLOADER_JUMP_TO_BS_REQUIRED) {
        switch (imgManager_getStatusMF()) {
            case IMG_MANAGER_MF_OK:
                LOG_WRN("MF status in primary slot is ok.");
                if (BL_status != BOOTLOADER_TOO_LOW_BATTERY_LEVEL) {
                    if (isMFReadyToStart()) {
                        BL_downloadStatus = BL_status;
                        BL_status = BOOTLOADER_READY_TO_JUMP_TO_MF;
                    } else {
                        BootFailedHandler(CLASSIC_BOOT);
                    }
                }
                break;
            case IMG_MANAGER_MF_TEST_IN_PROGRESS:
                BL_status = BOOTLOADER_MF_DID_NOT_RESPOND;
                LOG_ERR("MF don't response. Update failed?");
                BootFailedHandler(CLASSIC_BOOT);
                break;
            case IMG_MANAGER_MF_READ_FAILED:
                LOG_ERR("Primary slot is corrupted.");
                BootFailedHandler(CLASSIC_BOOT);
                BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_FOUND_NOTHING : BL_status;
                break;
            default:
                LOG_WRN("Update was probably interrupted.");
                /*  In this case Mcuboot will be trying to update without any agreements.
                 *   Just setup MF_boot_failed flag.
                 */
                BootFailedHandler(RESTORE_UPDATE);
                BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_FOUND_NOTHING : BL_status;
                break;
        }
    }

    BL_downloadStatus = BL_downloadStatus == BOOTLOADER_OK? BL_status : BL_downloadStatus;
    return BL_status == BOOTLOADER_OK ? BOOTLOADER_FOUND_NOTHING : BL_status;
}

void bootloader_tryUpdateProgress(bootloader_updateOperation_id_e id, uint32_t next_chunk)
{
    if (id < BOOTLOADER_UPDATE_OPERATION_NUM) {
        if (update_operation_progress[id].operation_progress <
            update_operation_progress[id].operation_size) {
            if (next_chunk < update_operation_progress[id].operation_size) {
                update_operation_progress[id].operation_progress += next_chunk;
            } else {
                update_operation_progress[id].operation_progress =
                    update_operation_progress[id].operation_size;
            }
            if (update_operation_progress[id].operation_progress >
                update_operation_progress[id].operation_size) {
                LOG_WRN("Incompatible operation size!");
                update_operation_progress[id].operation_progress =
                    update_operation_progress[id].operation_size;
            }
            taskGui_sendEvent(TASK_GUI_UPDATE_PROCESS);
        }
    }
}

int bootloader_getUpdateProgress(void)
{
    int progress = 0;
    if (bootloader_updateInitiated()) {
        for (int i = 0; i < BOOTLOADER_UPDATE_OPERATION_NUM; i++) {
            if (update_operation_progress[i].operation_size) {
                progress += update_operation_progress[i].operation_progress *
                            update_operation_progress[i].operation_percent /
                            update_operation_progress[i].operation_size;
            } else {
                progress += update_operation_progress[i].operation_percent;
            }
        }
    } else {
        progress = -1;
    }
    return progress;
}

bool bootloader_isThereValidMF(void)
{
    return BL_status == BOOTLOADER_READY_TO_JUMP_TO_MF;
}

bootloader_status_e bootloader_getDownloadStatus(void)
{
    return BL_downloadStatus;
}

bool bootloader_jumpToMF(void)
{
    /* WARNING Do not touch this value - optimization may throw out the branch with a jump*/
    bool result = false;
    if (boot_target_instanse.inited) {
        struct boot_rsp *rsp = &boot_target_instanse.rsp;
        /* initialize watchdog timer if not already done. it should be
         * updated from user app to mark successful start up of this app. if
         * the watchdog is not updated, reset will be initiated by watchdog
         * timer and swap revert operation started to roll back to operable
         * image.
         */
        secure_sendIOPtoMF();
        secure_deinitIOP();
        /* Call to application entry address. Never return. */
        endOfWork_jumpTo(rsp->br_image_off + rsp->br_hdr->ih_hdr_size);
        result = true;
    } else {
        BL_status = BOOTLOADER_FOUND_NOTHING;
        LOG_ERR("Empty pointer to MF, please check preparation before jumping");
    }
    return result;
}

void bootloader_jumpToBS(void)
{
#ifdef JUMP_TO_BS_VIA_REBOOT
    LOG_WRN("Jump to %lX...", FLASH_BASE);
    TEST_WAIT_CONSOLE();
    #ifdef MCUBOOT_HAVE_LOGGING
    osDelay(1000); /* Temporary to check log */
    #endif         /* MCUBOOT_HAVE_LOGGING */
    endOfWork_reboot();
#endif /* JUMP_TO_BS_VIA_REBOOT */
    endOfWork_jumpTo(FLASH_BASE);
}

/**
 * @brief Check incoming messages at BL startup
 * @return @ref bootloader_status_e: Nothig, Jump to BS, Error screen request.
 */
static bootloader_status_e incomingMessageHandler(void)
{
    bootloader_status_e initial_bl_status = BOOTLOADER_OK;
    int ret = 0;
    bootutil_data_t boot_data = {0};

    /* Read the secondary slot in external flash memory. */
    ret = parser_initBootData(&boot_data, FLASH_AREA_IMAGE_SECONDARY_ID);

    if (mailEngine_previousBLupdateFailed()) {
        initial_bl_status = BOOTLOADER_PREVIOUS_UPDATE_FAILED;
        if (ret == 0) {
            LOG_WRN("Update BL failed! Removing image from external slot.");
            imgManager_eraseArea(boot_data.fap);
        } else {
            LOG_WRN("Something wrong, image of BL not found in external flash after \"failed to "
                    "update BL\" msg from BS!");
        }
    }

    if (mailEngine_isUpdateCompletedBL()) {
        if (ret == 0) {
            LOG_WRN("Update BL completed successfully. Removing image from external slot.");
            imgManager_eraseArea(boot_data.fap);
        } else {
            LOG_WRN("Something wrong, image of BL not found in external flash after update BL was "
                    "completed!");
        }
    }

    /* Check if it's the first start of the Bootloader (BL) */
    if (mailEngine_firstStartBL()) {
        LOG_INF("First BL start detected.");

        /* Determine if sharing the new BL is required */
        if (mailEngine_shareNewBLRequired()) {
            /* Check if errors occurred while processing the boot data */
            bool isError = (ret != 0) ||
                           !parser_isBootloaderDescriptor(&boot_data) ||
                           (mailEngine_shareNewBL(boot_data.fap, parser_getImageSize(&boot_data)) != MAIL_ENGINE_OK);

            if (isError) {
                LOG_ERR("Mailbox sharing error encountered!");
                /* Despite the error, we still jump back to the Bootstrapper (BS) */
            } else {
                LOG_WRN("BL started successfully with the new public key.");
                mailEngine_sendEventToBS(RAM_MBOX_FW_EVENT_BL_FIRST_RUN_PUBLIC_KEY_SUCCESS);
            }
        } else {
            LOG_WRN("BL started successfully.");
            mailEngine_sendEventToBS(RAM_MBOX_FW_EVENT_BL_FIRST_RUN_SUCCESS);
        }

        /* If the BL descriptor is found, erase the outdated image */
        if ((ret == 0) && parser_isBootloaderDescriptor(&boot_data)) {
            LOG_WRN("Erasing image from external memory slot.");
            imgManager_eraseArea(boot_data.fap);
        } else {
            LOG_ERR("Error: BL image not found in external memory after the first BL start!");
        }

        /* Indicate that a jump to the BS is required */
        initial_bl_status = BOOTLOADER_JUMP_TO_BS_REQUIRED;
    }

    if (mailEngine_successFirstStartMF()) {
        if ((ret == 0) && parser_isMainAppDescriptor(&boot_data)) {
            LOG_WRN("The main application has been successfully updated! External flash will be "
                    "cleared.");
            imgManager_eraseArea(boot_data.fap);
        } else {
#ifndef MCUBOOT_OVERWRITE_ONLY
            /* Probably external flash was cleared in main application... */
            LOG_WRN("Image of MF not found in external flash after first start MF.");
#endif /* MCUBOOT_OVERWRITE_ONLY */
            LOG_WRN("Recieved first run message from MF. Update MF completed successfully.");
        }
        mailEngine_sendEventToMF(RAM_MBOX_FW_EVENT_MF_UPDATE_SUCCESS);
        imgManager_setOkMF();
    }

    parser_deinitBootData(&boot_data);
    return initial_bl_status;
}

/**
 * @brief Update MF was found, check ability to use.
 * @return  true - MF udpate is allowed.
 *          false - mcuboot can't use update.
 */
static bool isUpdateAllowed(void)
{
    if (MF_boot_failed || mailEngine_updateAllowed()) {
        if (get_current_level() >= BATTERY_LEVEL_TO_FW_UPDATE) {
            LOG_WRN(" battery =%d%%... update allowed.", get_current_level());
            return true;
        } else {
            BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_TOO_LOW_BATTERY_LEVEL : BL_status;
            LOG_WRN(" update prohibited. Battery level is too low (%d%%).", get_current_level());
            return false;
        }
    } else {
        /**Keep in mind - in case when update was interrupted then
         *                mcuboot will be trying to continue update without any agreements. */
        LOG_WRN(" update forbidden!\nContinue boot to current MF without changes.");
        return false;
    }
}

/**
 * @brief Handle update for bootloader according to the detected descriptor in @ref boot_data.
 * @param boot_data Initialized bootable data from secondary flash area (@ref bootutil_data_t).
 * @return int. 0 on success, BOOT_E code otherwise (see bootutil_public.h).
 */
static int newBootloaderHandler(bootutil_data_t *boot_data)
{
    if (isUpdateAllowed()) {
        if (mailEngine_shareNewBL(boot_data->fap, parser_getImageSize(boot_data)) !=
            MAIL_ENGINE_OK) {
            LOG_ERR("Mailbox error sharing!");
            return BOOT_EFLASH;
        }
        LOG_WRN("New image BL shared to BS via RAM mailBox, jump to BS...");
        BL_status = BOOTLOADER_JUMP_TO_BS_REQUIRED;
    } else {
        LOG_WRN("New image of BL found in external flash... update forbidden.\
                \nContinue boot to MF.");
        /* Passed: return 0 */
    }
    return 0;
}

/**
 * @brief Handle update for main firmware according to the detected descriptor in @ref boot_data.
 * @param[in] boot_data Initialized bootable data from secondary flash area (@ref bootutil_data_t).
 * @return int. 0 on success, BOOT_E code otherwise (see bootutil_public.h).
 */
static int newMainFirmwareHandler(bootutil_data_t *boot_data)
{
    if (isUpdateAllowed()) {
        bootutil_data_t primary_slot = {0};
        /* Read the primary slot in internal flash memory. */
        if (parser_initBootData(&primary_slot, FLASH_AREA_IMAGE_MAIN_APP_PRIMARY_ID) == 0) {
            if (parser_isVersionEqual(&primary_slot, boot_data)) {
                LOG_WRN("Update has the same version");
                BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_UPDATE_TO_SAME_VERSION
                                                       : BL_status;
                return BOOT_EBADVERSION;
            }
            if (parser_isDowngradeVersion(&primary_slot, boot_data)) {
                LOG_WRN("Downgrade has been canceled!");
                BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_SECURE_COUNTER_LESS : BL_status;
                return BOOT_EBADVERSION;
            }
            bootutil_data_t bootloader = {0};
            if (parser_initBootData(&bootloader, FLASH_AREA_IMAGE_SELF_ID) == 0) {
                if (parser_verifyDependency(&bootloader, boot_data) == false) {
                    LOG_WRN("Dependency error!");
                    BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_DEPENDENCY_ERROR : BL_status;
                    return BOOT_EBADVERSION;
                }
            } else {
                LOG_ERR("No bootloader found in flash area!");
            }
            update_operation_progress[BOOTLOADER_UPDATE_OPERATION_ERASE_PRIMARY].operation_size =
                parser_getImageSize(&primary_slot);
        } else {
            update_operation_progress[BOOTLOADER_UPDATE_OPERATION_ERASE_PRIMARY].operation_size = 0;
        }

        uint8_t public_key[SE_KEY_LEN] = {0};
        if (parser_readNewPublicKey(boot_data, public_key, SE_KEY_LEN) != 0) {
            /* new public key is not present in TLV, continue boot */

            LOG_WRN("New MainApp image was found...");
            initiateUpdateViaMcuboot(boot_data);
            /* Passed: return 0 */

        } else {
            /* new public key is present */
            LOG_WRN("New MainApp image with new public key was found...");

#ifndef MCUBOOT_ENC_IMAGES
            /* Temporary buffer required for hash calculation */
            static uint8_t tmpbuf[BOOT_TMPBUF_SZ] = {0};

            FIH_DECLARE(fih_rc, FIH_FAILURE);
            fih_rc = bootutil_img_validate(NULL, IMG_ID_MAIN_APP, &boot_data->hdr, boot_data->fap,
                                           tmpbuf, BOOT_TMPBUF_SZ, NULL, 0, NULL);

            board_wdRefresh(); /* Image validation may take a long time... */
            if (fih_rc != 0) {
                /* Image validation failed */

                if (MF_boot_failed) {
                    /** When update with new public key was interrupted
                     *  it's last chance to continue update. */
                    LOG_WRN("Trying to recover boot process...");
                    if (!flash_area_move_start(boot_data->fap, boot_data->hdr.ih_hdr_size)) {
                        BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_FLASH_ERROR : BL_status;
                        LOG_WRN(" nothing to recover.");
                        return BOOT_EBADVERSION;
                    }
                    initiateUpdateViaMcuboot(boot_data);
                    /* Passed: return 0 */

                } else {
                    LOG_WRN("Image wrapper check failed.");
                    return BOOT_EBADIMAGE;
                }

            } else {
                /* Wrapper validation passed successfully */

                if (!flash_area_move_start(boot_data->fap, boot_data->hdr.ih_hdr_size)) {
                    LOG_ERR("Bad image wrapper.");
                    BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_FLASH_ERROR : BL_status;
                    return BOOT_EBADVERSION;
                }
                if (libse_secureReplacePublicKeyMF(public_key, SE_KEY_LEN) != LIBSE_OK) {
                    BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_SE_FAILED : BL_status;
                    LOG_ERR("Update public key failed.");
                    return BOOT_EBADSTATUS;
                }

                initiateUpdateViaMcuboot(boot_data);
                /* Passed: return 0 */
            }
#else
    #error "Encoded image data is not supported now."
#endif
        }
    } else {
        LOG_WRN("MainApp update was found in external flash.");
    }
    return 0;
}

/**
 * @brief Call this function when main firmware is ready to boot.
 *  	   Doing some additional steps before jump and then jump.
 * @return false if MF is not ready to jump
 */
static bool isMFReadyToStart()
{
    bool result = true;

    struct boot_rsp *rsp = &boot_target_instanse.rsp;
    FIH_DECLARE(fih_rc, FIH_FAILURE);
    TEST_WAIT_CONSOLE();
    board_wdRefresh();
    FIH_CALL(boot_go, fih_rc, rsp);

    if (FIH_EQ(fih_rc, FIH_SUCCESS)) {

        if (verifyDepencency() == false) {
            LOG_ERR("Dependency verification failed for the main firmware.");
            result = false;
            return result;
        }

        boot_target_instanse.inited = true;
        LOG_WRN("MF is ready to start.");
        LOG_WRN("br_image_off = 0x%x", rsp->br_image_off);
        LOG_WRN("ih_hdr_size = 0x%x", rsp->br_hdr->ih_hdr_size);
        if (rsp->br_flash_dev_id != FLASH_DEVICE_INTERNAL_FLASH) {
            LOG_ERR("Impossible case, boot allowed for internal flash only. ");
            result = false;
        } else if (bootloader_updateInitiated()) {
            switch (imgManager_getStatusMF()) {
                case IMG_MANAGER_MF_READ_FAILED:
                    LOG_WRN("Primary slot is corrupted.");
                    imgManager_eraseBadMF();
                    result = false;
                    break;
                case IMG_MANAGER_MF_MAGIC_UNSET:
                case IMG_MANAGER_MF_COPY_DONE_UNSET:
                    imgManager_setMagicMF();
                    /* fallthrough */
                case IMG_MANAGER_MF_TEST_IN_PROGRESS:
                    mailEngine_sendEventToMF(RAM_MBOX_FW_EVENT_MF_FIRST_RUN);
                    /* Also this case occured when update over not tested image was failed. */
                    LOG_WRN("Update complete, first start MF.");
                    break;
                case IMG_MANAGER_MF_OK:
                default:
                    mailEngine_sendEventToMF(RAM_MBOX_FW_EVENT_MF_FAILED_TO_UPDATE);
                    LOG_WRN("Update canceled via mcuboot or error occured.");
                    break;
            }
        } else {
            switch (imgManager_getStatusMF()) {
                case IMG_MANAGER_MF_OK:
                    if (mailEngine_previousBLupdateFailed()) { 
                        LOG_WRN("Sending 'Failed to update' msg to MF.");
                        mailEngine_sendEventToMF(RAM_MBOX_FW_EVENT_BL_FAILED_TO_UPDATE);
                    }
                    if (mailEngine_isUpdateCompletedBL()) {
                        LOG_WRN("Sending 'Update BL completed successfully' msg to MF.");
                        mailEngine_sendEventToMF(RAM_MBOX_FW_EVENT_BL_UPDATE_SUCCESS);
                    }
                    break;
                case IMG_MANAGER_MF_MAGIC_UNSET:
                case IMG_MANAGER_MF_COPY_DONE_UNSET:
                    LOG_WRN("Initial launch of the firmware?");
                    imgManager_setMagicMF();
                    mailEngine_sendEventToMF(RAM_MBOX_FW_EVENT_MF_FIRST_RUN);
                    break;
                case IMG_MANAGER_MF_READ_FAILED:
                    LOG_WRN("Primary slot is corrupted.");
                    imgManager_eraseBadMF();
                    result = false;
                    break;
                case IMG_MANAGER_MF_TEST_IN_PROGRESS: /* MF don't response. */
                default:
                    LOG_WRN("Boot to MF canceled.");
                    imgManager_eraseBadMF();
                    result = false;
                    break;
            }
        }
    } else {
        result = false;
    }

    return result;
}

/**
 * @brief Continue update if image wrapper MF has been detected in external flash,
 *  otherwise just check external flash without user agreements.
 *  Also here can be present gui message to user.
 * @param tryToRestoreUpdate    True if update was interrupted and attempt is required
 *                              to try to recovery progress of update.
 *                              False otherwise.
 */
static void BootFailedHandler(bool tryToRestoreUpdate)
{
    if (bootloader_updateInitiated()) {
        /* Nothing to do. Should never be here. */
        LOG_ERR("It was not possible to update via mcuboot!");
        imgManager_eraseBadMF();
    } else {
        if (!tryToRestoreUpdate) {
            imgManager_eraseBadMF();
        }

        LOG_WRN("Search for update...");
        TEST_WAIT_CONSOLE();

        MF_boot_failed = true;
        checkDownloadedUpdate(); /* call again now, when failed flag is active */

        if (isMFReadyToStart()) {
            BL_status = BOOTLOADER_READY_TO_JUMP_TO_MF;
            /* Success */
            return;
        }

        LOG_WRN("MCUBoot Bootloader found none of bootable images.");
        if (tryToRestoreUpdate) {
            imgManager_eraseBadMF();
        }
    }

    if (bootloader_updateInitiated()) {
        BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_IMAGE_BAD_SIGNATURE : BL_status;
        imgManager_eraseUpdateMF();
    }

    /* Clear all progress */
    partialDownload_loadImage(NULL, 0, 0, 0, PARTIAL_DOWNLOAD_MODE_FIRMWARE);

    mailEngine_removeMsgToMF();
}

/**
 * @brief Read external flash to check update data.
 * @return int. 0 if no additional action is required.
 *              BOOT code otherwise (see bootutil_public.h).
 */
static int checkDownloadedUpdate(void)
{
    int ret = 0;
    bootutil_data_t boot_data = {0};

    /* Read the secondary slot in external flash memory. */
    ret = parser_initBootData(&boot_data, FLASH_AREA_IMAGE_SECONDARY_ID);
    if (ret == 0) {
        if (parser_isBootloaderDescriptor(&boot_data)) {
            if (newBootloaderHandler(&boot_data) != 0) {
                BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_FLASH_ERROR : BL_status;
            }
        } else if (parser_isMainAppDescriptor(&boot_data)) {
            if (newMainFirmwareHandler(&boot_data) != 0) {
                imgManager_eraseArea(boot_data.fap);
                BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_IMAGE_BAD_SIGNATURE : BL_status;
            }
        } else if (MF_boot_failed || mailEngine_updateAllowed()) {
            BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_GARBAGE_DETECTED : BL_status;
            LOG_WRN("Image with wrong descriptor stored in external flash!");
            imgManager_eraseArea(boot_data.fap);
            ret = BOOT_EBADIMAGE;
        }
    } else if (mailEngine_updateAllowed()) {
        /*reset progress for partial download?*/
        BL_status = BL_status == BOOTLOADER_OK ? BOOTLOADER_GARBAGE_DETECTED : BL_status;
        LOG_WRN("Nothing in external flash memory.");
        imgManager_eraseArea(boot_data.fap);
    }
    parser_deinitBootData(&boot_data);
    return ret;
}

/**
 * @brief Allows to initialize an update from secondary flash area via mcuboot.
 * @param boot_data Initialized bootable data from secondary flash area (@ref bootutil_data_t).
 * @return int. 0 on success, BOOT_E code otherwise (see bootutil_public.h).
 */
static int initiateUpdateViaMcuboot(bootutil_data_t *boot_data)
{
    if (boot_data == NULL || boot_data->fap == NULL) {
        return BOOT_EBADARGS;
    }
    imgManager_initiateUpdate(boot_data->fap);
    update_via_mcuboot_was_initiated = true;

#ifdef MCUBOOT_OVERWRITE_ONLY_FAST
    update_operation_progress[BOOTLOADER_UPDATE_OPERATION_COPY].operation_size =
        parser_getImageSize(boot_data);
    update_operation_progress[BOOTLOADER_UPDATE_OPERATION_ERASE_SECONDARY].operation_size =
        EXTERNAL_FLASH_SECTOR_SIZE * 2;
    for (int i = 0; i < BOOTLOADER_UPDATE_OPERATION_NUM; i++) {
        update_operation_progress[i].operation_progress = 0;
    }
#else
    #warning "Test required: copy/erase progress with full slot size."
#endif
    taskGui_sendEvent(TASK_GUI_UPDATE_STARTED_EVENT);
    return 0;
}

/**
 * @brief wrapper for universalizing versions
 * @return level
 */
static inline uint8_t get_current_level()
{
#if (HWLT_BOARD_REVISION_NUM == 1)
    #include "board_battMeter.h"
    return battMeter_getLevel();
#else
    return battControl_getLevel();
#endif
}

/**
 * @brief Verifies dependencies between the main firmware and the bootloader.
 *
 * This function checks and verifies whether the main firmware's required
 * dependencies are met based on the current bootloader configuration.
 *
 * @return true if the dependencies are verified and met, false otherwise.
 */
static bool verifyDepencency(void)
{
    bool verify_passed = false;
    bootutil_data_t boot_data = {0};
    bootutil_data_t bootloader = {0};
    do {
        int ret = parser_initBootData(&boot_data, FLASH_AREA_IMAGE_MAIN_APP_PRIMARY_ID);
        if (ret != 0) {
            LOG_WRN("Main Firmware: NaN");
            break;
        }

        if (parser_initBootData(&bootloader, FLASH_AREA_IMAGE_SELF_ID) == 0) {
            verify_passed = parser_verifyDependency(&bootloader, &boot_data);
        } else {
            LOG_ERR("No bootloader found in flash area!");
        }
    } while (0);
    parser_deinitBootData(&boot_data);
    parser_deinitBootData(&bootloader);
    return verify_passed;
}

char* bootloader_getPrimaryVersionOfMF(void)
{
    static char version[64] = {0};
    bootutil_data_t boot_data = {0};
    int ret = parser_initBootData(&boot_data, FLASH_AREA_IMAGE_MAIN_APP_PRIMARY_ID);
    if ((ret == BOOT_EBADIMAGE || ret == 0) && (boot_data.hdr.ih_ver.iv_major != 0xFFU)) {
        sprintf(version, "Main Firmware: %d.%d.%d.%ld", boot_data.hdr.ih_ver.iv_major,
                boot_data.hdr.ih_ver.iv_minor, boot_data.hdr.ih_ver.iv_revision,
                boot_data.hdr.ih_ver.iv_build_num);
    } else {
        strcpy(version, "Main Firmware: NaN");
    }
    parser_deinitBootData(&boot_data);
    return version;
}

char* bootloader_getSecondaryVersionOfMF(void)
{
    static char version[64] = {0};
    bootutil_data_t boot_data = {0};
    int ret = parser_initBootData(&boot_data, FLASH_AREA_IMAGE_SECONDARY_ID);
    if ((ret == BOOT_EBADIMAGE || ret == 0) && (boot_data.hdr.ih_ver.iv_major != 0xFFU)) {
        sprintf(version, "Release %d.%d.%d.%ld", boot_data.hdr.ih_ver.iv_major,
                boot_data.hdr.ih_ver.iv_minor, boot_data.hdr.ih_ver.iv_revision,
                boot_data.hdr.ih_ver.iv_build_num);
        if (boot_data.hdr.ih_load_addr == 0x8020000) {
            strcat(version, " (BL)");
        } else {
            strcat(version, " (MF)");
        }
    } else {
        strcpy(version, "Release NaN");
    }
    parser_deinitBootData(&boot_data);
    return version;
}

void bootloader_cleanSS(void)
{
    for(int i = INTERNAL_FLASH_SS_END_SECTOR; i >= INTERNAL_FLASH_SS_START_SECTOR; i--) {
        board_wdRefresh();
        internalMemory_eraseSector(i);
    }
}

void bootloader_cleanMF(void)
{
    for(int i = INTERNAL_FLASH_PRIMARY_END_SECTOR; i >= INTERNAL_FLASH_PRIMARY_START_SECTOR; i--) {
        board_wdRefresh();
        internalMemory_eraseSector(i);
    }
}