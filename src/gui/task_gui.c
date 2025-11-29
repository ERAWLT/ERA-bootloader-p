/**
 * @file task_gui.c
 * @brief Task GUI module.
 */
#include "task_gui.h"
#include "boot_app.h"

#include "board_partial_download.h"
#include "task_touchscreen.h"
#include "gui_core.h"
#include "board.h"

#include "cmsis_os.h"
#include "shell.h"
#include "elog.h"
#define LOG_MODULE_NAME "task_gui"
#define LOG_INF(...)    elog_i(LOG_MODULE_NAME, __VA_ARGS__)
#define LOG_ERR(...)    elog_e(LOG_MODULE_NAME, __VA_ARGS__)
#define LOG_WRN(...)    elog_w(LOG_MODULE_NAME, __VA_ARGS__)

#define GUI_CYCLE_TIME_MS      (100) /*!< Default timeout before next polling on screen. */
#define TASK_GUI_MAX_EVENT_LEN 10    /*!< Maximum event number for GUI task. */

/** Uncomment below to show the logo only under touch */
// #define GO_TO_BL_ABILITY_UNDER_TOUCH_ONLY

/** Queue type for GUI task. */
typedef struct {
    guiTask_event_e num; /*!< @ref guiTask_event_e */
} gui_queue_s;

static TaskHandle_t gui_task_handle = NULL;

/** Queue to save event for gui in bootloader */
static QueueHandle_t gui_task_queue = NULL;

static void gui_task(const void *);
static bool checkTouchs(void);

void guiTask_create(void)
{
    gui_task_queue = xQueueCreate(TASK_GUI_MAX_EVENT_LEN, sizeof(gui_queue_s));
    if (gui_task_queue == NULL) {
        LOG_ERR("create queue");
        Error_Handler();
    }

    if (xTaskCreate((TaskFunction_t)gui_task, "UI dialog", configMINIMAL_STACK_SIZE * 10, NULL,
                    osPriorityNormal, &gui_task_handle) != pdPASS) {
        LOG_ERR("create task");
        Error_Handler();
    }

    return;
}

void taskGui_sendEvent(guiTask_event_e num)
{
    gui_queue_s ui_event = {.num = num};
    xQueueSend(gui_task_queue, &ui_event, 1);
}

/**
 * @brief GUI task function.
 * @param arg Handle
 */
static void gui_task(const void *arg)
{
    LOG_INF("Started");
    gui_queue_s ui_event = {.num = TASK_GUI_EMPTY_EVENT};
    TickType_t wait_for_event_ticks = portMAX_DELAY;
    while (1) {
        if (xQueueReceive(gui_task_queue, &ui_event, wait_for_event_ticks) == pdPASS) {
            wait_for_event_ticks = pdMS_TO_TICKS(GUI_CYCLE_TIME_MS);
            /* Drawing should be in block mode. Pass down only when screen drawn. */
            switch (ui_event.num) {
                case TASK_GUI_WAIT_USER_EVENT:
                    LOG_INF("wait update screeen!");
                    TEST_WAIT_CONSOLE();
                    guiCore_switchScreen(SCREEN_ID_WAIT_USER);
#if (HWLT_BOARD_REVISION_NUM != 1)
                    /* TODO: check the operation of the flash at a low freq */
                    // pwr_McuLowPower();
#endif
                    break;

                case TASK_GUI_INFO:
                    LOG_INF("Info screen!");
                    TEST_WAIT_CONSOLE();
                    guiCore_switchScreen(SCREEN_ID_INFO);
                    break;

                case TASK_GUI_MORE_INFO:
                    LOG_INF("More info screen!");
                    TEST_WAIT_CONSOLE();
                    guiCore_switchScreen(SCREEN_ID_MORE_INFO);
                    break;

                case TASK_GUI_SHIPPING:
                    LOG_INF("Shipping screen!");
                    TEST_WAIT_CONSOLE();
                    guiCore_switchScreen(SCREEN_ID_SHIPPING);
                    break;

                case TASK_GUI_DOWNLOAD_EVENT:
                    LOG_INF("Download progress screen!");
                    guiCore_switchScreen(SCREEN_ID_DOWNLOAD);
                    break;

                case TASK_GUI_UPDATE_STARTED_EVENT:
                    LOG_INF("Update progress screen!");
                    guiCore_switchScreen(SCREEN_ID_UPDATING);
                    break;
                case TASK_GUI_UPDATE_PROCESS:
                    break;

                case TASK_GUI_LOW_BATTERY_EVENT:
                    LOG_INF("Low battery level screen!");
                    guiCore_switchScreen(SCREEN_ID_LOW_BATTERY_LEVEL);
                    break;

                case TASK_GUI_UPDATE_ERROR:
                    LOG_INF("Error screen");
                    guiCore_switchScreen(SCREEN_ID_ERROR);
                    break;

                case TASK_GUI_FORCE_DOWNLOAD_SWITCHER:
#ifdef GO_TO_BL_ABILITY_UNDER_TOUCH_ONLY
                    if (!checkTouchs()) {
                        bootApp_sendEvent(BOOT_APP_EVENT_BOOT_TO_MF);
                        break;
                    }
#endif /* GO_TO_BL_ABILITY_UNDER_TOUCH_ONLY */
                    LOG_INF("Wait for start force download!");
                    TEST_WAIT_CONSOLE();
                    guiCore_switchScreen(SCREEN_ID_FORCE_DOWNLOAD);
                    break;

                case TASK_GUI_START_MF_OK_EVENT:
                default:
                    break;
            }
        } else {
            /* Do not use touchs when polling the screen for the first time.
            So checkTouchs after 'else'. */
            checkTouchs();
        }
        guiCore_checkTimeout();
        guiCore_poll();
    }
}

/**
 * @brief Allows to check all touchs on screen and share them with user logic.
 * @return bool true if there is a new touch, false otherwise
 */
static bool checkTouchs(void)
{
    bool new_touch = false;
    static touchscreen_ev_s touch_event;
    static guiCore_touchEvent_s last_touch;

    if (last_touch.touch_state == TOUCH_STATE_RELEASED) {
        last_touch.touch_state = TOUCH_STATE_NONE;
    }
    while (touchscreen_readFromQueue(&touch_event) == 0) {
        new_touch = true;
        switch (touch_event.ev) {
            case TS_EV_DOWN:
            case TS_EV_CONTACT:
                LOG_INF("Tap process.");
                if (last_touch.touch_state != TOUCH_STATE_RELEASED) {
                    last_touch.touch_state = TOUCH_STATE_PRESSED;
                    last_touch.position.x = touch_event.x;
                    last_touch.position.y = touch_event.y;
                }
                break;

            case TS_EV_UP:
                last_touch.touch_state = TOUCH_STATE_RELEASED;
                last_touch.position.x = touch_event.x;
                last_touch.position.y = touch_event.y;
                break;
            default:
                break;
        }
    }
    /* Apply only last touch (released in priority) */
    guiCore_applyTouch(&last_touch);
    return new_touch;
}
