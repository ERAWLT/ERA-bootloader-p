/**
 * @file boot_app.c
 * @brief Main module for hwlt-mcuboot application.
 */

#include "boot_app.h"
#include "version_project.h"

#include "stdint.h"
#include "cmsis_os.h"
#include "board.h"
#include "board_nfc.h"
#include "board_partial_download.h"
#include "task_nfc_transport.h"
#include "bootloader.h"
#include "task_gui.h"
#include "mail_engine.h"
#include "img_manager.h"

#include "shell_bootloader.h"
#include "elog.h"
#define LOG_MODULE_NAME "boot_app"
#define LOG_ERR(...)    elog_e(LOG_MODULE_NAME, __VA_ARGS__)

#define FORCE_DOWNLOAD_ENABLED /*!< Enable the "Force to bootloader" button to skip MF startup */
#define SHOW_ERROR_ALWAYS      /*!< Enable error screen in BL when a valid MF exists */

#define TICK_TO_WAIT_IN_BLOCK   pdMS_TO_TICKS(1000)
#define MAIN_TASK_MAX_EVENT_LEN 1

#define VERIFY_TIMEOUT_MS    5000 /*!< Required time to verify firmware after download */
#define CLOSE_PROTOCOL_DELAY 1000 /*!< Timeout to close session after download */

#define MOTOR_VIBRATION_TIME_MS 100 /*!< Time to vibrate the motor during boot */

static void turnOffNfc(int boot_num);
static void bootTask(void);
static inline void readyToJumpToMFhandler(void);

static TaskHandle_t boot_app_handle = NULL;
static QueueHandle_t boot_queue = NULL;
static bool force_download_clicked = false;
static int boot_num = 0;

void app_main(void)
{
    motor_control(true);
    vTaskDelay(pdMS_TO_TICKS(MOTOR_VIBRATION_TIME_MS));
    motor_control(false);

    if (xTaskCreate((TaskFunction_t)bootTask, "boot application", configMINIMAL_STACK_SIZE * 10,
                    NULL, osPriorityNormal, &boot_app_handle) != pdPASS) {
        LOG_ERR("create task");
        Error_Handler();
    }
}

bool bootApp_peekForEvent(bootApp_event_e event_num, uint32_t timeout_ms)
{
    bootApp_event_e boot_event = 0;
    if (xQueuePeek(boot_queue, &boot_event, pdMS_TO_TICKS(timeout_ms)) == pdPASS) {
        if (boot_event == event_num) {
            return true;
        }
    }
    return false;
}

bool bootApp_isForceDownloadClicked(void)
{
    return force_download_clicked;
}

void bootApp_sendEvent(bootApp_event_e boot_event)
{
    xQueueSend(boot_queue, &boot_event, TICK_TO_WAIT_IN_BLOCK);
}

/**
 * @brief Main task for bootloader
 */
static void bootTask(void)
{
    boot_queue = xQueueCreate(MAIN_TASK_MAX_EVENT_LEN, sizeof(bootApp_event_e));
    bootApp_event_e boot_event = BOOT_APP_EVENT_START_AGAIN;
    guiTask_create();
    switch (bootloader_init()) {
        case BOOTLOADER_PREVIOUS_UPDATE_FAILED:
            boot_event = BOOT_APP_EVENT_SHOW_ERROR;
            break;
        case BOOTLOADER_JUMP_TO_BS_REQUIRED:
            bootloader_jumpToBS();
            break;
        default:
            break;
    }
    xQueueSend(boot_queue, &boot_event, TICK_TO_WAIT_IN_BLOCK);
    
    frontLight_enable(true);

    while (1) {
        osDelay(10);
        board_wdRefresh();
        if (xQueueReceive(boot_queue, &boot_event, TICK_TO_WAIT_IN_BLOCK) == pdPASS) {
            switch (boot_event) {
                case BOOT_APP_EVENT_START_AGAIN:
                    boot_num = 0;
                case BOOT_APP_EVENT_UPDATE_DOWNLOADED:
                    turnOffNfc(boot_num);
                    switch (bootloader_letStart()) {
                        case BOOTLOADER_TOO_LOW_BATTERY_LEVEL:
                            taskGui_sendEvent(TASK_GUI_LOW_BATTERY_EVENT);
                            break;

                        case BOOTLOADER_UPDATE_TO_SAME_VERSION:
                        case BOOTLOADER_SECURE_COUNTER_LESS:
                        case BOOTLOADER_IMAGE_BAD_SIGNATURE:
                        case BOOTLOADER_FLASH_ERROR:
                        case BOOTLOADER_MF_DID_NOT_RESPOND:
                        case BOOTLOADER_DEPENDENCY_ERROR:
                            taskGui_sendEvent(TASK_GUI_UPDATE_ERROR);
                            break;

                        case BOOTLOADER_JUMP_TO_BS_REQUIRED:
                            taskGui_sendEvent(TASK_GUI_UPDATE_STARTED_EVENT);
                            if (!bootApp_peekForEvent(BOOT_APP_EVENT_SCREEN_DRAWN,
                                                      TASK_GUI_MAX_SCREEN_DRAWING_TIME_MS)) {
                                LOG_ERR("GUI is too slow!");
                            }
                            bootloader_jumpToBS();
                            break;
                        case BOOTLOADER_READY_TO_JUMP_TO_MF:
                            readyToJumpToMFhandler();
                            break;

                        case BOOTLOADER_GARBAGE_DETECTED:
                        case BOOTLOADER_FOUND_NOTHING:
                        default:
                            taskGui_sendEvent(TASK_GUI_WAIT_USER_EVENT);
                            break;
                    }
                    boot_num += 1;
                    break;
                case BOOT_APP_EVENT_SHOW_ERROR:
                    frontLight_setBrightness(10);
                    taskGui_sendEvent(TASK_GUI_UPDATE_ERROR);
                    break;
                
                /* Force donwload logic start */
                case BOOT_APP_EVENT_GO_BL_CLICKED:
                    force_download_clicked = true;
                    mailEngine_forceDownloadOccured();
                    taskGui_sendEvent(TASK_GUI_WAIT_USER_EVENT);
                    frontLight_setBrightness(10);
                    break;
                case BOOT_APP_EVENT_LATER_CLICKED:
                    if (force_download_clicked) {
                        bootloader_jumpToMF();
                    }
                    taskGui_sendEvent(TASK_GUI_WAIT_USER_EVENT);
                    break;
                /* Force donwload logic end */

                case BOOT_APP_EVENT_BOOT_TO_MF:
                    if (boot_num == 0){
                        /* previous update BL failed, check MF */
                        boot_event = BOOT_APP_EVENT_START_AGAIN;
                        xQueueSend(boot_queue, &boot_event, TICK_TO_WAIT_IN_BLOCK);
                    } else if (!force_download_clicked) {
                        /* Usually Bl start */
                        if (!bootloader_jumpToMF()) {
                            taskGui_sendEvent(TASK_GUI_WAIT_USER_EVENT);
                        }
                    } else {
                        /* User trying to update via BL, so stays here */
                        taskGui_sendEvent(TASK_GUI_WAIT_USER_EVENT);
                    }
                    break;
                
                default:
                    break;
            }
        }
    }
}

/**
 * @brief Perform actions before jump to MF or just jump to MF, when bootloader ready to jump to
 * main firmware.
 * @return None.
 */
static inline void readyToJumpToMFhandler(void)
{
    if (imgManager_isUpdateMFpassed()) {
        if (!bootApp_peekForEvent(BOOT_APP_EVENT_SCREEN_DRAWN,
                                  TASK_GUI_MAX_SCREEN_DRAWING_TIME_MS)) {
            LOG_ERR("GUI is too slow!");
        }
        /* Jump without confirmation */
        if (!bootloader_jumpToMF()) {
            taskGui_sendEvent(TASK_GUI_UPDATE_ERROR);
        }
    } else {
        if (mailEngine_updateAllowed()) {
            if (!force_download_clicked) {
#ifndef SHOW_ERROR_ALWAYS
                /* update was allowed in MF, error screen should be there */
                bootloader_jumpToMF();
#endif /* not SHOW_ERROR_ALWAYS */
            }
            if (bootloader_getDownloadStatus() == BOOTLOADER_TOO_LOW_BATTERY_LEVEL ||
                (boot_num == 0 && force_download_clicked)) {
                /* Exit from low battery level screen */
                taskGui_sendEvent(TASK_GUI_WAIT_USER_EVENT);
            } else {
                taskGui_sendEvent(TASK_GUI_UPDATE_ERROR);
            }
        } else {
#ifdef FORCE_DOWNLOAD_ENABLED
            frontLight_setBrightness(0);
            taskGui_sendEvent(TASK_GUI_FORCE_DOWNLOAD_SWITCHER);
#else
            /* Jump without confirmation */
            if (!bootloader_jumpToMF()) {
                taskGui_sendEvent(TASK_GUI_WAIT_USER_EVENT);
            }
#endif /* FORCE_DOWNLOAD_ENABLED */
        }
    }
}

/**
 * @brief Turn off NFC exchange to before starting boot.
 * @param boot_num int 0 after restarting the device, boot number otherwise.
 * @return None
 */
static void turnOffNfc(int boot_num)
{
    if (boot_num != 0) {
        uint32_t timeout_ms = VERIFY_TIMEOUT_MS;
        while (timeout_ms) {
            if (partialDownload_isBusy() == 0) {
                osDelay(CLOSE_PROTOCOL_DELAY);
                board_wdRefresh();
                break;
            }
            timeout_ms = timeout_ms < 100 ? 0 : timeout_ms - 100;
            osDelay(100);
            board_wdRefresh();
        }
    }
#if (HWLT_BOARD_REVISION_NUM == 1)
    nfc_disableRfConnection();
#else
    nfcTransport_sleep();
#endif
}
