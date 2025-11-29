/**
 * @file    bootloader.h
 * @brief HWLT-bootloader header.
 */

#ifndef _HWLT_BOOTLOADER_H_
#define _HWLT_BOOTLOADER_H_

#include <stdint.h>
#include <stdbool.h>

/** Boot status */
typedef enum {
    BOOTLOADER_OK,                     /*!< Boot passed successfully. */
    BOOTLOADER_TOO_LOW_BATTERY_LEVEL,  /*!< Too low battery level to continue required operation */
    BOOTLOADER_GARBAGE_DETECTED,       /*!< Garbage detected in external flash. */
    BOOTLOADER_UPDATE_TO_SAME_VERSION, /*!< Update to the same version is forbidden. */
    BOOTLOADER_SECURE_COUNTER_LESS,    /*!< The update has a lower security counter. */
    BOOTLOADER_IMAGE_BAD_SIGNATURE,    /*!< Update check failed. */
    BOOTLOADER_FLASH_ERROR,            /*!< Error in external flash when copying an update. */
    BOOTLOADER_FOUND_NOTHING,          /*!< Nothing to boot found */
    BOOTLOADER_READY_TO_JUMP_TO_MF,    /*!< All Checked and could go to MainApp */
    BOOTLOADER_JUMP_TO_BS_REQUIRED,    /*!< Jump to BS without confirmation in BL */
    BOOTLOADER_PREVIOUS_UPDATE_FAILED, /*!< "failed to update BL" msg from BS */
    BOOTLOADER_MF_DID_NOT_RESPOND,     /*!< First start was unsuccessful, the application did not
                                          respond. */
    BOOTLOADER_SE_FAILED,               /*!< The secure element has failed. */
    BOOTLOADER_DEPENDENCY_ERROR,        /*!< Incompatible version of the app (too old). */
} bootloader_status_e;

/** Type for update operation indexes */
typedef enum {
    BOOTLOADER_UPDATE_OPERATION_VERIFY,        /*!< Verification is first update operation. */
    BOOTLOADER_UPDATE_OPERATION_ERASE_PRIMARY, /*!< Erase primaty slot is second update operation.
                                                */
    BOOTLOADER_UPDATE_OPERATION_COPY, /*!< Copying from secondary slot to primary is third update
                                         operation. */
    BOOTLOADER_UPDATE_OPERATION_ERASE_SECONDARY, /*!< Erase secondary slot is fourth update
                                                    operation. */
    BOOTLOADER_UPDATE_OPERATION_NUM,             /*!< Number of update operations.  */
} bootloader_updateOperation_id_e;

/**
 * @brief Initialize data before boot.
 * @return None
 */
bootloader_status_e bootloader_init(void);

/**
 * @brief Bootloader main function.
 *  Performs updates for bootloader and main-firmware if allowed by user.
 *  Also continue updates if it was interrupted in previous start.
 *  Boot to main firmware as default.
 * @return None
 * @note Call it also when MF is not present but user want to update downloaded image.
 */
bootloader_status_e bootloader_letStart(void);

/**
 * @brief Get update initiation flag.
 * @return true if update was initiated via mcuboot, false otherwise.
 */
bool bootloader_updateInitiated(void);

/**
 * @brief Used when updating in progress. Allows to update the current update progress.
 * @param id @ref bootloader_updateOperation_id_e.
 * @param next_chunk Next memory chunk size for read/write/erase operation.
 * Negative value when full size.
 * @return None
 */
void bootloader_tryUpdateProgress(bootloader_updateOperation_id_e id, uint32_t next_chunk);

/**
 * @brief Used to show current update progress percent.
 * @return int Negative value when updating the bootloader, positive percentage value from 0 to 100
 * if an updating is in progress for main-firmware.
 */
int bootloader_getUpdateProgress(void);

/**
 * @brief Used to get actual status of update.
 * @return @ref bootloader_status_e.
 */
bootloader_status_e bootloader_getDownloadStatus(void);

/**
 * @brief Get current status of boot.
 * @return true MF is valid, false no.
 */
bool bootloader_isThereValidMF(void);

/**
 * @brief jump to MF
 * @return false if we havenot jumped
 */
bool bootloader_jumpToMF(void);

/**
 * @brief Jump to bootstrapper.
 * @return Never return.
 */
void bootloader_jumpToBS(void);

#endif /* _HWLT_BOOTLOADER_H_ */
