/**
 * @file user_logic.c
 * @brief General logic for screens.
 */
#include "user_logic.h"
#include "gui_core.h"

#include "endOfWork_manager.h"
#include "board.h"
#include "board_pwr.h"
#include "board_nfc.h"

#include "boot_app.h"
#include "task_gui.h" /*to switch screen*/
#include "board_partial_download.h"

#include "elog.h"
#include "cmsis_os.h" /* To use delay for print */
#define LOG_MODULE_NAME "user_logic"
#define LOG_INF(...)    elog_i(LOG_MODULE_NAME, __VA_ARGS__)
#define LOG_ERR(...)    elog_e(LOG_MODULE_NAME, __VA_ARGS__)

/** Maximum screen time in milliseconds */
static uint32_t time_before_standby_ms = 5 * 60 * 1000;

/** Time of the last event on the current screen */
static uint32_t screen_time_ms = 0;

void checkScreenTimeout(screen_id_e scr_num)
{
    if (getPassedTimeMs(getScreenTimeMs()) >= getScreenTimeoutMs()) {
        switch (scr_num) {
            case SCREEN_ID_WAIT_USER:
                taskGui_sendEvent(TASK_GUI_SHIPPING);
                break;
            case SCREEN_ID_LOW_BATTERY_LEVEL:
                taskGui_sendEvent(TASK_GUI_SHIPPING);
                break;
            case SCREEN_ID_INFO:
                taskGui_sendEvent(TASK_GUI_SHIPPING);
                break;
            case SCREEN_ID_SHIPPING:
                break;
            case SCREEN_ID_DOWNLOAD:
                bootApp_sendEvent(BOOT_APP_EVENT_UPDATE_DOWNLOADED);
                break;
            case SCREEN_ID_UPDATING:
                /* Unexpected case! */
                LOG_ERR("Timeout expired while updating...");
                bootApp_sendEvent(BOOT_APP_EVENT_UPDATE_DOWNLOADED);
                break;
            case SCREEN_ID_ERROR:
                /* Save the error code on screen until User sees */
                endOfWork_goSleep(); // bootApp_sendEvent(BOOT_APP_EVENT_BOOT_TO_MF);
                break;
            case SCREEN_ID_FORCE_DOWNLOAD:
                bootApp_sendEvent(BOOT_APP_EVENT_BOOT_TO_MF);
                break;
            default:
                break;
        }
        refreshScreenTime();
    }
}

void setScreenTimeoutMs(uint32_t ms)
{
    time_before_standby_ms = ms;
}

uint32_t getScreenTimeoutMs(void)
{
    return time_before_standby_ms;
}

void refreshScreenTime(void)
{
    screen_time_ms = HAL_GetTick();
}

uint32_t getScreenTimeMs(void)
{
    return screen_time_ms;
}

uint32_t getPassedTimeMs(uint32_t start_time_ms)
{
    uint32_t cnt_ms = HAL_GetTick();
    if (cnt_ms < start_time_ms) {
        return (0xFFFFFFFFU - start_time_ms + cnt_ms);
    } else {
        return (cnt_ms - start_time_ms);
    }
}
