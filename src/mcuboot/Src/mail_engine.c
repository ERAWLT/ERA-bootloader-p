/**
 * @file    mail_engine.c
 * @brief HWLT-bootloader mailbox engine module. 
 *  This module allows to manage mailbox messages with BS (bootstrapper) and MF (main-firmware).
 */

#include "mail_engine.h"
#include <string.h>
#include <stdio.h>

#include "board_internal_memory.h"
#include "board_partial_download.h"

#ifdef DEBUG_UPDATE_OK_ON_TEST
    #include "img_manager.h"
#endif

/** See @ref mailEngine_firstStartBL. */
static bool is_first_run_BL = false;
/** See @ref mailEngine_shareNewBLRequired. */
static bool send_image_again_to_BS_required = false;
/** See @ref mailEngine_updateAllowed. */
volatile static bool is_update_allowed = false;
/** Used to set primary slot as good. See @ref mailEngine_successFirstStartMF. */
static bool is_update_MF_successful = false;
/** @ref mailEngine_previousBLupdateFailed. */
static bool failed_to_update_BL = false;
/** Used to remove existed update in external flash. */
static bool is_update_BL_successful = false;

mailEngine_err_e mailEngine_shareNewBL(const struct flash_area *fa_new_BL, 
                                       uint32_t total_size_new_BL)
{
    if ((total_size_new_BL > FLASH_SECTOR_SIZE) || (fa_new_BL == NULL)) {
        return MAIL_ENGINE_BAD_PARAMETERS;
    }

    ram_mbox_msg_t *msg = NULL;
    if (ramMbox_putRaw((uint8_t)RAM_MBOX_ID_BL, (uint8_t)RAM_MBOX_ID_BS, \
                       RAM_MBOX_MSG_TYPE_FIRMWARE, &msg, total_size_new_BL)) {
        mailEngine_removeMsgFromBL();
        return MAIL_ENGINE_RAM_ERR;
    }

    uint8_t *dst = RAM_MBOX_GET_PAYLOAD(msg);
    if (flash_area_read(fa_new_BL, 0, dst, total_size_new_BL)) {
        return MAIL_ENGINE_FLASH_ERR;
    }
    
    if (ramMbox_payloadIsComplete(msg)) {
        return MAIL_ENGINE_RAM_ERR;
    }

    return MAIL_ENGINE_OK;
}

void mailEngine_sendEventToBS(ram_mbox_fw_event_e event)
{
    ram_mbox_fw_event_e *data = &event;
    ramMbox_put((uint8_t)RAM_MBOX_ID_BL, (uint8_t)RAM_MBOX_ID_BS, RAM_MBOX_MSG_TYPE_EVENT, \
                (uint8_t *)data, sizeof(*data));
}

void mailEngine_sendEventToMF(ram_mbox_fw_event_e event)
{
    ram_mbox_fw_event_e *data = &event;
    ramMbox_put((uint8_t)RAM_MBOX_ID_BL, (uint8_t)RAM_MBOX_ID_APP, RAM_MBOX_MSG_TYPE_EVENT, \
                (uint8_t *)data, sizeof(*data));
}

bool mailEngine_isUpdateCompletedBL(void)
{
    return is_update_BL_successful;
}

bool mailEngine_firstStartBL(void)
{
    return is_first_run_BL;
}

bool mailEngine_shareNewBLRequired(void)
{
    return send_image_again_to_BS_required;
}

bool mailEngine_updateAllowed(void)
{
    return is_update_allowed;
}

bool mailEngine_successFirstStartMF(void)
{
    return is_update_MF_successful;
}

void mailEngine_forceDownloadOccured(void)
{
    is_update_allowed = true;
}

bool mailEngine_previousBLupdateFailed(void)
{
    return failed_to_update_BL;
}

bool mailEngine_getManufactureString(ram_mbox_fw_manufacture_e code, char *dst_buf,
                                     uint32_t dst_buf_size)
{
    bool is_valid = false;
    ram_mbox_msg_t *msg = ramMbox_getMsgTo(RAM_MBOX_ID_BL, &is_valid);
    while (msg != NULL) {
        if (!is_valid) {
            ramMbox_delete(msg);
        } else if (msg->from == RAM_MBOX_ID_BS) {
            if (msg->type == RAM_MBOX_MSG_TYPE_MANUFACTURE) {
                if (sizeof(ram_mbox_fw_manufacture_e) < msg->len) {
                    ram_mbox_fw_manufacture_e *manufacture_type = RAM_MBOX_GET_PAYLOAD(msg);
                    if (code == *manufacture_type) {
                        char *manufacuture_string = (char *)manufacture_type +
                                                    sizeof(ram_mbox_fw_manufacture_e);
                        uint32_t string_len = msg->len - sizeof(ram_mbox_fw_manufacture_e);
                        memcpy(dst_buf, manufacuture_string,
                               string_len > dst_buf_size ? dst_buf_size : string_len);
                        ramMbox_delete(msg);
                        return true;
                    }
                }
            }
        }
        msg = ramMbox_seekMsgNextTo(msg, RAM_MBOX_ID_BL, &is_valid);
    }
    return false;
}

bool mailEngine_getIOP(uint8_t *iop_bytes, uint32_t iop_len)
{
    bool status = false;
    bool is_valid = false;
    ram_mbox_msg_t *msg = ramMbox_getMsgTo(RAM_MBOX_ID_BL, &is_valid);
    while (msg != NULL && iop_bytes != NULL) {
        if (!is_valid) {
            ramMbox_delete(msg);
        } else if (msg->from == RAM_MBOX_ID_BS) {
            if (msg->type == RAM_MBOX_MSG_TYPE_IOP) {
                if (iop_len == msg->len) {
                    uint8_t *iop_fromBS = RAM_MBOX_GET_PAYLOAD(msg);
                    memcpy(iop_bytes, iop_fromBS, msg->len);
                    status = true;
                }
                ramMbox_delete(msg);
            }
        }
        msg = ramMbox_seekMsgNextTo(msg, RAM_MBOX_ID_BL, &is_valid);
    }
    return status;
}

void mailEngine_readMsgToBL(void)
{
    bool is_valid = false;
    ram_mbox_msg_t *msg = ramMbox_getMsgTo(RAM_MBOX_ID_BL, &is_valid);
    while (msg != NULL) {
        if (!is_valid) {
            ramMbox_delete(msg);
        } else if (msg->from == RAM_MBOX_ID_BS) {
            if (msg->type == RAM_MBOX_MSG_TYPE_EVENT) {
                if (sizeof(ram_mbox_fw_event_e) == msg->len) {
                    ram_mbox_fw_event_e *code = RAM_MBOX_GET_PAYLOAD(msg);
                    switch (*code) {
                        case RAM_MBOX_FW_EVENT_BL_FIRST_RUN_PUBLIC_KEY:
                            send_image_again_to_BS_required = true;
                        case RAM_MBOX_FW_EVENT_BL_FIRST_RUN:
                            is_first_run_BL = true;
                            break;

                        case RAM_MBOX_FW_EVENT_BL_UPDATE_SUCCESS:
                            is_update_BL_successful = true;
                            break;

                        case RAM_MBOX_FW_EVENT_BL_FAILED_TO_UPDATE:
                        case RAM_MBOX_FW_EVENT_UPDATE_TOO_LOW_SECURE_CNT:
                        case RAM_MBOX_FW_EVENT_UPDATE_VERSION_EQUAL:
                        case RAM_MBOX_FW_EVENT_UPDATE_WRONG_PUBLIC_KEY:
                            failed_to_update_BL = true;
                            break;

                        case RAM_MBOX_FW_EVENT_ERR_RCV_SLOT_0:
                        case RAM_MBOX_FW_EVENT_ERR_RCV_SLOT_1:
                        case RAM_MBOX_FW_EVENT_ERR_UPD_SLOT_1:
                        default:
                            break;
                    }
                }
                ramMbox_delete(msg);
            }
        } else if (msg->from == RAM_MBOX_ID_APP) {
            if (msg->type == RAM_MBOX_MSG_TYPE_EVENT) {
                if (sizeof(ram_mbox_fw_event_e) == msg->len) {
                    ram_mbox_fw_event_e *code = RAM_MBOX_GET_PAYLOAD(msg);
                    if (*code == RAM_MBOX_FW_EVENT_MF_FIRST_RUN_SUCCESS) {
                        is_update_MF_successful = true;
                    } else if (*code == RAM_MBOX_FW_EVENT_ALLOW_UPDATE) {
                        is_update_allowed = true;
                    } 
                }
                ramMbox_delete(msg);
            }
        }
        msg = ramMbox_seekMsgNextTo(msg, RAM_MBOX_ID_BL, &is_valid);
    }
}

void mailEngine_removeMsgToBL(void)
{
    uint32_t msg_cnt = ramMbox_getMsgCntTo((uint8_t)RAM_MBOX_ID_BL);

    while (msg_cnt--) {
        bool is_valid = false;
        ram_mbox_msg_t *msg = ramMbox_getMsgTo((uint8_t)RAM_MBOX_ID_BL, &is_valid);
        if (msg != NULL) {
            ramMbox_delete(msg);
        }
    }
}

void mailEngine_shareIOPtoMF(const uint8_t *iop_bytes, uint32_t iop_len)
{
    ramMbox_put((uint8_t)RAM_MBOX_ID_BL, (uint8_t)RAM_MBOX_ID_APP, RAM_MBOX_MSG_TYPE_IOP,
                (uint8_t *)iop_bytes, iop_len);
}

void mailEngine_removeMsgToMF(void)
{
    uint32_t msg_cnt = ramMbox_getMsgCntTo((uint8_t)RAM_MBOX_ID_APP);

    while (msg_cnt--) {
        bool is_valid = false;
        ram_mbox_msg_t *msg = ramMbox_getMsgTo((uint8_t)RAM_MBOX_ID_APP, &is_valid);
        if (msg != NULL) {
            ramMbox_delete(msg);
        }
    }
}

void mailEngine_removeMsgFromBL(void)
{
    bool            is_valid = false;
    ram_mbox_msg_t *msg      = ramMbox_getMsgFrom(RAM_MBOX_ID_BL, &is_valid);

    while (msg != NULL) {
        if (!is_valid) {
            printf("msg at %p non valid\n", msg);
            ramMbox_delete(msg);
        } else if (msg->from == RAM_MBOX_ID_BL) {
            printf("BL garbage collection at %p\n", msg);
            ramMbox_delete(msg);
        }
        msg = ramMbox_getMsgFrom(RAM_MBOX_ID_BL, &is_valid);
    }
}
