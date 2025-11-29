/**
 * @file    img_manager.h
 * @brief HWLT-bootloader image manager header.
 */

#ifndef _IMG_MANAGER_H_
#define _IMG_MANAGER_H_

#include "stdbool.h"
#include "stdint.h"
#include "flash_map_backend/flash_map_backend.h"

/** Current status for image of main firmware in primary slot. */
typedef enum {
    IMG_MANAGER_MF_OK,               /*!< Classic boot without changes ('none' status type). */
    IMG_MANAGER_MF_TEST_IN_PROGRESS, /*!< Update was completed, but test required "first run MF". */
    IMG_MANAGER_MF_COPY_DONE_UNSET, /*!< Copy done flag is not set. Probably update was interrupted. */
    IMG_MANAGER_MF_MAGIC_UNSET,     /*!< Magic is not valid. Probably update was interrupted. */
    IMG_MANAGER_MF_READ_FAILED,      /*!< Flash read from primary slot was failed. */
} imgManager_status_MF_e;

/**
 * @brief This func is used to init update, if image of new MF exist in external flash.
 * @param fap @ref flash_area secondary slot.
 * @return None
 */
void imgManager_initiateUpdate(const struct flash_area *fap);

/**
 * @brief This func is used when mcuboot finishes copying update of MF to primary slot.
 * @param fap @ref flash_area primary slot.
 * @return None
 */
void imgManager_setCopyDone(const struct flash_area *fap);

/**
 * @brief This function marks the primary image as valid.
 * @return None
 */
void imgManager_setOkMF(void);

/**
 * @brief Read status from primary slot of MF.
 * @return @ref imgManager_status_MF_e.
 */
imgManager_status_MF_e imgManager_getStatusMF(void);

/**
 * @brief This function erases an existing MF image in the primary slot. 
 *  Call it with BootFailedHandler, when primary slot check has failed.
 * @note If slot is already erased, then erase operation will not be called.
 * @return None
 */
void imgManager_eraseBadMF(void);

/**
 * @brief This function erases an update of MF in the secondary slot. 
 * @note If slot is already erased, then erase operation will not be called.
 * @return None
 */
void imgManager_eraseUpdateMF(void);

/**
 * @brief This function erases an existing image in a slot. 
 * @note If slot is already erased, then erase operation will not be called.
 * @param fap @ref flash_area primary slot.
 * @return None
 */
void imgManager_eraseArea(const struct flash_area *fap);

/**
 * @brief This function is used to add magic value to image state.
 * @return None
 */
void imgManager_setMagicMF(void);

/**
 * @brief Required to jump to MF without "go to BL" question
 * @return Ture if update via mcuboot was passed successfully, false otherwise.
 */
bool imgManager_isUpdateMFpassed(void);

#endif /* _IMG_MANAGER_H_ */