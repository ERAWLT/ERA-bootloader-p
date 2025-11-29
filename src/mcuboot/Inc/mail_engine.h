/**
 * @file    mail_engine.h
 * @brief HWLT-bootloader mailbox engine header.
 */

#ifndef _MAILBOX_ENGINE_H_
#define _MAILBOX_ENGINE_H_

#include "stdbool.h"
#include "stdint.h"
#include "flash_map_backend/flash_map_backend.h"
#include "ramMbox.h"

typedef enum {
    MAIL_ENGINE_OK = 0,                 /*!< All work completed successfully */
    MAIL_ENGINE_BAD_PARAMETERS = 1,     /*!< Unexpected function parameters */
    MAIL_ENGINE_FLASH_ERR = 2,          /*!< Flash engine error */
    MAIL_ENGINE_RAM_ERR = 3,            /*!< RamMbox error */
} mailEngine_err_e;

/**
 * @brief Get status first start.
 * @return bool. True if msg about first start was found.
 *               False otherwise.
 */
bool mailEngine_firstStartBL(void);

/**
 * @brief "Go to BL" button has been released (with a working application).
 * @return None
 */
void mailEngine_forceDownloadOccured(void);

/**
 * @brief Get status of the flag "update allowed".
 * @return bool. True if update allowed.
 *               False otherwise.
 */
bool mailEngine_updateAllowed(void);

/**
 * @brief Check - is image of new BL required for BS again.
 * @return bool. True if send new image required.
 *               False otherwise.
 */
bool mailEngine_shareNewBLRequired(void);

/**
 * @brief Check - is msg from MF about first successful start.
 * @return bool. True if exist mailbox msg with "ok" from MF.
 *               False otherwise.
 */
bool mailEngine_successFirstStartMF(void);

/**
 * @brief Get status of the flag "failed to udpate BL".
 * @return bool. True if received mail with "failed to update BL" msg from BS.
 *               False otherwise.
 */
bool mailEngine_previousBLupdateFailed(void);

/**
 * @brief Get status of the flag "Update BL completed successfully".
 * @return bool. true if flag set, false otherwise.
 */
bool mailEngine_isUpdateCompletedBL(void);

/**
 * @brief Share new BL image to the bootstrapper via the mailbox. 
 *  Bootstrapper will handle the BL update.
 * @param fa_new_BL flash area pointer to new BL.
 * @param total_size_new_BL total size of new BL.
 * @return @ref mailEngine_err_e. 
 */
mailEngine_err_e mailEngine_shareNewBL(const struct flash_area *fa_new_BL, uint32_t total_size_new_BL);

/**
 * @brief Send event code to bootstrapper via mailbox message.
 * @param event @ref ram_mbox_fw_event_e. Event number.
 * @return None
 */
void mailEngine_sendEventToBS(ram_mbox_fw_event_e event);

/**
 * @brief Send event code to main-firmware via mailbox message.
 * @param event @ref ram_mbox_fw_event_e. Event number.
 * @return None
 */
void mailEngine_sendEventToMF(ram_mbox_fw_event_e event);

/**
 * @brief Extract all useful emails to BL.
 * @return None
 */
void mailEngine_readMsgToBL(void);

/**
 * @brief Extract all unreachable emails to BL.
 * @return None
 */
void mailEngine_removeMsgToBL(void);

/**
 * @brief When MF fails, extract all emails to him.
 * @return None
 */
void mailEngine_removeMsgToMF(void);

/**
 * @brief Get the chosen manufacutrer parameter string from BS.
 * @param code @ref ram_mbox_fw_manufacture_e. Type code of manufacture parameter.
 * @param[inout] dst_buf Pointer to destination buffer to write manufacture data.
 * @param dst_buf_size Destination buffer size in bytes.
 * @return bool. True if manufacture parameter exist in mailbox. False otherwise.
 */
bool mailEngine_getManufactureString(ram_mbox_fw_manufacture_e code, char *dst_buf, uint32_t dst_buf_size);

/**
 * @brief Deleting old messages from bootloader.
 * @return None.
 */
void mailEngine_removeMsgFromBL(void);

/**
 * @brief Search for IOP message from bootstrapper to get iop bytes.
 * @param[out] iop_bytes Received secret bytes for use.
 * @param iop_len Required length in bytes.
 * @return true on success, false otherwise.
 */
bool mailEngine_getIOP(uint8_t *iop_bytes, uint32_t iop_len);

/**
 * @brief Share IOP bytes with bootloader via mailbox message.
 * @param[in] iop_bytes Secret bytes to share.
 * @param iop_len Length in bytes.
 * @return true on success, false otherwise.
 */
void mailEngine_shareIOPtoMF(const uint8_t *iop_bytes, uint32_t iop_len);

#endif /* _MAILBOX_ENGINE_H_ */
