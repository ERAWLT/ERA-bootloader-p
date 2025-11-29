/**
 * @file gui_core.c
 * @brief GUI core module for drawing screens
 */
#include "gui_core.h"
#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"

#include "string.h"
#include "screens.h"

#include "board.h"
#include "board_epaper.h"
#include "board_pwr.h"
#include "board_batt_control.h"
#include "user_logic.h"
#include "hwlt_version.h"
#include "mail_engine.h"

#include "elog.h"
#include "cmsis_os.h" /* To use delay for print */
#define LOG_MODULE_NAME "gui_core"
#define LOG_INF(...)    elog_i(LOG_MODULE_NAME, __VA_ARGS__)

#define DARK_MODE        0                          /*!< Set to '1' to enable dark color theme */
#define BACKGROUND_COLOR (DARK_MODE ? (0) : (0xFF)) /*!< Background color */
#define FOREGROUND_COLOR (DARK_MODE ? (0xFF) : (0)) /*!< Foreground color */

#define EPD_WIDTH  EPAPER_WIDTH                       /*!< Screen width */
#define EPD_HEIGHT EPAPER_HEIGHT                      /*!< Screen height */
#define EPD_ARRAY  (EPAPER_WIDTH * EPAPER_HEIGHT / 8) /*!< Screen framebuffer size */

#define PERIODIC_CLEAR_DISPLAY
#define FULL_UPDATE_PERIOD_TIMES      10 /*!< Full update every X times when switching the display */
#define PARTIAL_UPDATE_TIMES_TO_CLEAR 20 /*!< Fast update after X times of partial update */

#define HWLT_MANUFACTURE_LEN                                                                       \
    (HWLT_VERSION_MAX_VER_LEN * 2 + 10) /*!< Manufacturer string length,                           \
                                            additional 10 bytes for service info */

extern char *getBatteryLevelString(void);

/** Labels with version of bootloader, bootstrapper and serial number */
guiWidget_label_s widgetLabel_deviceInfo[] = {
    {
     .position.x = 9,
     .position.y = 45 + 22 + 12,
     .font = HWLT_FONT_8,
     .text = NULL,
     },
    {
     .position.x = 9,
     .position.y = 45 + 22 + 12 * 2,
     .font = HWLT_FONT_8,
     .text = NULL,
     },
    {
     .position.x = 9,
     .position.y = 45 + 20 + 12 * 3,
     .font = HWLT_FONT_8,
     .text = NULL,
     },
};

/** Label battery level */
guiWidget_label_s widgetLabel_battery = {
    .text = NULL,
    .position.x = 126,
    .position.y = 5,
    .font = HWLT_FONT_8,
};

/**
 * @brief Functions that allows to paint base image and static elements on screen.
 * @return None.
 */
static void (*screen_switch[SCREEN_NUM_ID])(void) = {
    [SCREEN_ID_WAIT_USER] = screenWaitUpdate_switch,
    [SCREEN_ID_LOW_BATTERY_LEVEL] = screenLowBattery_switch,
    [SCREEN_ID_INFO] = screenInfo_switch,
    [SCREEN_ID_SHIPPING] = screenShipping_switch,
    [SCREEN_ID_DOWNLOAD] = screenDownload_switch,
    [SCREEN_ID_UPDATING] = screenUpdating_switch,
    [SCREEN_ID_ERROR] = screenError_switch,
    [SCREEN_ID_FORCE_DOWNLOAD] = screenForceDownload_switch,
    [SCREEN_ID_MORE_INFO] = screenInfo2_switch,

};

/**
 * @brief Functions that allows to paint animation on screen.
 * @return bool true if the screen needs to be updated, false otherwise.
 */
static bool (*screen_poll[SCREEN_NUM_ID])(void) = {
    [SCREEN_ID_WAIT_USER] = screenWaitUpdate_poll,
    [SCREEN_ID_LOW_BATTERY_LEVEL] = screenLowBattery_poll,
    [SCREEN_ID_INFO] = screenInfo_poll,
    [SCREEN_ID_SHIPPING] = screenShipping_poll,
    [SCREEN_ID_DOWNLOAD] = screenDownload_poll,
    [SCREEN_ID_UPDATING] = screenUpdating_poll,
    [SCREEN_ID_ERROR] = screenError_poll,
    [SCREEN_ID_FORCE_DOWNLOAD] = screenForceDownload_poll,
    [SCREEN_ID_MORE_INFO] = screenInfo2_poll,
};

static guiCore_touchEvent_s *getCurrentTouch(void);
static void readVersions(void);
static void readSN(void);

/** Screen pixel buffer where used 1 bit per pixel */
static unsigned char guiCore_framebuffer[EPD_ARRAY];
/** Full update screen flag for epaper display */
static bool full_update_required = true;
/** First screen switch was completed */
static bool first_switch_completed = false;
/** Actual touch data */
static guiCore_touchEvent_s current_touch;
/** Actual screen number */
static screen_id_e current_screen_id = SCREEN_ID_WAIT_USER;

/** Flags to control whether each screen should draw/initialize display */
static bool drawScreenFlag[SCREEN_NUM_ID] = {
    [SCREEN_ID_WAIT_USER] = true,         /*!< User waiting screen - needs display */
    [SCREEN_ID_LOW_BATTERY_LEVEL] = true, /*!< Low battery level screen - needs display */
    [SCREEN_ID_INFO] = true,              /*!< Info screen - needs display */
    [SCREEN_ID_SHIPPING] = true,          /*!< Shipping screen - needs display */
    [SCREEN_ID_DOWNLOAD] = true,          /*!< Download screen - needs display */
    [SCREEN_ID_UPDATING] = true,          /*!< Update screen - needs display */
    [SCREEN_ID_ERROR] = true,             /*!< Error screen - needs display */
    [SCREEN_ID_FORCE_DOWNLOAD] = false,   /*!< Force download screen - no display needed */
    [SCREEN_ID_MORE_INFO] = true,         /*!< Additional info screen - needs display */
};

void guiCore_switchScreen(screen_id_e screen_id)
{
    /* Initialize display only when we need to show visual feedback to the user.
     * For SCREEN_ID_FORCE_DOWNLOAD, we don't initialize display since no visual feedback is needed. */
    if (!first_switch_completed && screen_id < SCREEN_NUM_ID && drawScreenFlag[screen_id]) {
        epaper_init();
        epaper_setCurrentTheme(DARK_MODE);
        readVersions();
        readSN();
        first_switch_completed = true;
        paint_newImage(guiCore_framebuffer, EPD_WIDTH, EPD_HEIGHT, ROTATE_0, FOREGROUND_COLOR);
    }
    
    if (first_switch_completed && screen_id < SCREEN_NUM_ID && drawScreenFlag[screen_id]) {
        guiCore_applyTouch(NULL); /* Clear touchs before, except the first screen (required for go BL).
                                   */
    }
    
    /* Always set current screen ID and refresh time for timeout handling */
    if (screen_id < SCREEN_NUM_ID) {
        current_screen_id = screen_id;
    }
    refreshScreenTime();
    
    /* Only proceed with visual screen drawing if display was initialized */
    if (first_switch_completed && screen_id < SCREEN_NUM_ID && drawScreenFlag[screen_id]) {
        full_update_required = true; /* Only for first screen */
        paint_clear(BACKGROUND_COLOR);
    }

    /* Always call screen switch function for logic (even without display) */
    screen_switch[current_screen_id]();
}

void guiCore_poll(void)
{
    if (current_screen_id >= SCREEN_NUM_ID) {
        return;
    }
    
    /* For force download screen without display init, still need to poll for touch events */
    if (!first_switch_completed && !drawScreenFlag[current_screen_id]) {
        screen_poll[current_screen_id]();
        return;
    }
    
    if (!first_switch_completed) {
        return;
    }

    if (screen_poll[current_screen_id]() || full_update_required) {
        /* Release below */
        epaper_update_e update_type = full_update_required ? EPAPER_UPDATE_FAST
                                                           : EPAPER_UPDATE_PARTIAL;
#ifdef PERIODIC_CLEAR_DISPLAY
        static uint8_t swtich_display_cnt = FULL_UPDATE_PERIOD_TIMES;
        static uint8_t partial_update_cnt = 0;
        if (full_update_required) {
            if ((current_screen_id == SCREEN_ID_SHIPPING) ||
                (current_screen_id == SCREEN_ID_ERROR)) {
                update_type = EPAPER_UPDATE_FULL;
                swtich_display_cnt = 0;
            } else if (swtich_display_cnt++ >= FULL_UPDATE_PERIOD_TIMES) {
                update_type = EPAPER_UPDATE_FAST; // EPAPER_UPDATE_FULL;
                swtich_display_cnt = 0;
            }
        }
        if (update_type == EPAPER_UPDATE_PARTIAL) {
            if (partial_update_cnt++ >= PARTIAL_UPDATE_TIMES_TO_CLEAR) {
                partial_update_cnt = 0;
                update_type = EPAPER_UPDATE_FAST;
            }
        } else {
            partial_update_cnt = 0;
        }
#endif /* PERIODIC_CLEAR_DISPLAY */
        epaper_drawFramebuffer((const uint8_t *)guiCore_framebuffer, update_type, true);
        full_update_required = false;
    }
}

void guiCore_checkTimeout(void)
{
        
    if ((!first_switch_completed || full_update_required) && current_screen_id != SCREEN_ID_FORCE_DOWNLOAD) {
        return;
    }
    checkScreenTimeout(current_screen_id);
}

void guiCore_applyTouch(guiCore_touchEvent_s *new_touch)
{
    if (new_touch != NULL) {
        current_touch.touch_state = new_touch->touch_state;
        current_touch.position.x = new_touch->position.x;
        current_touch.position.y = new_touch->position.y;
    } else {
        current_touch.touch_state = TOUCH_STATE_NONE;
    }
}

bool guiCore_processButton(guiWidget_button_s *btn)
{
    guiCore_touchEvent_s *touch = getCurrentTouch();
    bool in_range = true;
    if (touch->position.x < btn->position.x || touch->position.x > btn->position.x + btn->width) {
        /* Out of Ox range */
        in_range = false;
    }
    if (touch->position.y < btn->position.y || touch->position.y > btn->position.y + btn->height) {
        /* Out of Oy range */
        in_range = false;
    }

    bool previous_btn_state = btn->pressed;

    switch (touch->touch_state) {
        case TOUCH_STATE_PRESSED:
            if (in_range) {
                if ((!btn->pressed) && (btn->pressedCallback != NULL)) {
                    btn->pressedCallback();
                }
                btn->pressed = true;
            } else {
                btn->pressed = false;
            }
            break;

        case TOUCH_STATE_RELEASED:
            if (in_range) {
                btn->pressed = true;
                if (btn->releasedCallback != NULL) {
                    btn->releasedCallback();
                }
            }
            break;
        case TOUCH_STATE_NONE:
        default:
            btn->pressed = false;
            break;
    }
    
    return previous_btn_state != btn->pressed;
}

/**
 * @brief Allows to get current touch event (@ref guiCore_touchEvent_s).
 * @return Pointer to @ref guiCore_touchEvent_s.
 */
static guiCore_touchEvent_s *getCurrentTouch(void)
{
    return &current_touch;
}

/**
 * @brief Allows to initialize the version label with actual text.
 * @return None
 */
static void readVersions(void)
{
    static char ver_string_bl[HWLT_VERSION_MAX_VER_LEN + 20] = {0};
    hwlt_version_s *ver = hwltVersion_getCurrent();
    if (ver->cnt > 0) {
        snprintf(ver_string_bl, HWLT_VERSION_MAX_VER_LEN + 20, "Bootloader: %s", (ver->records)[0].ver);
    }
    widgetLabel_deviceInfo[0].text = ver_string_bl;

    static char ver_string_bs[HWLT_VERSION_MAX_VER_LEN + 20] = {0};
    strncpy(ver_string_bs, "Bootstrapper: ", HWLT_VERSION_MAX_VER_LEN);
    char *bs_ver = (char *)ver_string_bs + strlen(ver_string_bs);
    if (!mailEngine_getManufactureString(RAM_MBOX_FW_MANUFACTURE_VERSION_BS, bs_ver,
                                         HWLT_VERSION_MAX_VER_LEN)) {
        return;
    }
    widgetLabel_deviceInfo[1].text = ver_string_bs;
}

static void readSN(void)
{
    static char serial_string[HWLT_VERSION_MAX_VER_LEN + 20] = {0};
    sprintf(serial_string, "SN: ");
    char *ver = (char *)serial_string + strlen(serial_string);
    if (!mailEngine_getManufactureString(RAM_MBOX_FW_MANUFACTURE_SERIAL_NUMBER, ver,
                                         HWLT_VERSION_MAX_VER_LEN)) {
        return;
    }
    widgetLabel_deviceInfo[2].text = serial_string;
}

/**
 * @brief Show battery level
 * @param full_update Draw a battery indicator anyway if true.
 * @param invert Invert main color (@ref color_t).
 * @return true if screen update reqired, false nothing changed.
 */
bool drawBatteryLevel(bool full_update, bool invert)
{
    static bool charging = false;
    static uint8_t last_battery_level = 100;
    static char battery_line[sizeof("100%%")];

    extern const bitmap_dsc_s img_lightning;

    /** Lightning image */
    static const guiWidget_img_s widgetImg_lightning = {
        .position.x = 161,
        .position.y = 4,
        .img_dsc = &img_lightning,
    };

    widgetLabel_battery.text = battery_line;
    
    bool update_req = false;
    if ((battControl_getLevel() != last_battery_level) || (charging != pwr_isChargerConnected()) ||
        full_update) {
        update_req = true;
        charging = pwr_isChargerConnected();
        last_battery_level = battControl_getLevel();

        snprintf(battery_line, sizeof(battery_line), "%3.1u%%",
                 last_battery_level > 100 ? 100 : last_battery_level);

        if (invert) paint_selectColor(BACKGROUND_COLOR); /* Invert battery area */
        paint_clearWindow(widgetLabel_battery.position.x, widgetImg_lightning.position.y, 55, 12);

        paint_drawStringEn(widgetLabel_battery.position.x, widgetLabel_battery.position.y,
                           widgetLabel_battery.text, widgetLabel_battery.font, TEXT_ALIGN_LEFT,
                           PAINT_COLOR);
        paint_drawBattery(157, 5, last_battery_level);
        if (charging) {
            paint_drawBitMap(widgetImg_lightning.position.x, widgetImg_lightning.position.y,
                             widgetImg_lightning.img_dsc, PAINT_BACKGROUND | PAINT_COLOR);
        }
        if (invert) paint_selectColor(FOREGROUND_COLOR); /* Revert back */
    }
    return update_req;
}
