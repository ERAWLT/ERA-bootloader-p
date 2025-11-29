#include "screens.h"
#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"
#include "task_gui.h"

#include "board_pwr.h"
#include "board_batt_control.h"
#include "gui_core.h"
#include "user_logic.h"
#include "boot_app.h"

/* Macros thas allows to convert constant values to string */
#define STRINGIFY(s) TO_STRING(s)
#define TO_STRING(s) #s

#define LOW_BATTERY_TIMEOUT_MS (30 * 1000) /*!< Screen timeout in milliseconds */
#define SHIFT_Y     (20u)

extern const bitmap_dsc_s img_battery;
extern const bitmap_dsc_s img_attention;
extern bool drawBatteryLevel(bool full_update, bool invert);

static char *getBatteryLevelString(void);
static inline uint8_t get_current_level();
static void turnOffButtonReleased(void);
static void goBackPressed(void);

/** Base image */
static const guiWidget_img_s widget_img_title = {
    .position.x = 28 + 8,
    .position.y = 55,
    .img_dsc = &img_battery,
};

/** Part during discharging */
static const guiWidget_img_s imgWidget_attentionPart = {
    .position.x = 28 + 8 + 52,
    .position.y = 55 + 15,
    .img_dsc = &img_attention,
};

/** Part during charging */
static const guiWidget_img_s imgWidget_lightingPart = {
    .position.x = 8 + 74,
    .position.y = 55 + 12,
    .img_dsc = NULL, /*!< Included in base image */
};

/** Back button */
static guiWidget_button_s button_goBack = {
    .position.x = 8,
    .position.y = SHIFT_Y,
    .width = 30,
    .height = 24,
    .color = 1,
    .text = NULL,
    .pressedCallback = goBackPressed,
};

const char low_battery_title[] = "Low battery";
const char charging_title[] = "Charging";

const char low_battery_progress[] = "LOW BATTERY";
const char charging_progress[] = "[CHARGING]";

/** Text on screen */
static guiWidget_label_s widgetList_label[] = {
    {
     .position.x = 9,
     .position.y = 5,
     .font = HWLT_FONT_8,
     .text = low_battery_title,
     .text_align = TEXT_ALIGN_LEFT,
     },
    {
     .text = low_battery_progress,
     .position.x = 15,
     .position.y = 135 + 20,
     .font = HWLT_FONT_8_BOLD,
     .text_align = TEXT_ALIGN_CENTER,
     },
    {
     .text = "Charge the device to at least " STRINGIFY(
     BATTERY_LEVEL_TO_DOWNLOAD) "% before starting the update.",
     .position.x = 15,
     .position.y = 135 + 20 + 12,
     .font = HWLT_FONT_8,
     .text_align = TEXT_ALIGN_CENTER,
     },
    {
     .text = "<",
     .position.x = 8,
     .position.y = SHIFT_Y + 4,
     .text_align = TEXT_ALIGN_LEFT,
     .font = HWLT_FONT_16_BOLD,
     },
};

/* Button to turn off */
static guiWidget_button_s widgetButton_turnOff = {
    .position.x = 8,
    .position.y = 8+216, // 164 + 52,
    .width = 160,
    .height = 32,
    .color = 1,
    .text = "TURN OFF",
    .releasedCallback = turnOffButtonReleased,
};

static guiWidget_labelAnimation_s widget_labelAnimation = {
    .getLabelText = getBatteryLevelString,
    .position.x = 15,
    .position.y = 130,
    .font = HWLT_FONT_16_BOLD,
};

static uint8_t last_battery_level = 100;

void screenLowBattery_switch(void)
{
    last_battery_level = 100;
    setScreenTimeoutMs(LOW_BATTERY_TIMEOUT_MS);

#ifndef REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS
    paint_drawBitMap(widget_img_title.position.x, widget_img_title.position.y,
                     widget_img_title.img_dsc, PAINT_COLOR);
#endif
    for (int i = 0; i < sizeof(widgetList_label) / sizeof(*widgetList_label); i++) {
        paint_drawStringEn(widgetList_label[i].position.x, widgetList_label[i].position.y,
                           widgetList_label[i].text, widgetList_label[i].font,
                           widgetList_label[i].text_align, PAINT_COLOR);
    }
    drawBatteryLevel(true, false);
    paint_drawButton(&widgetButton_turnOff);
}

bool screenLowBattery_poll(void)
{
    static bool is_charger_connected = true;
    bool update_required = false;

    uint8_t level = get_current_level();
    if (last_battery_level != level) {
        last_battery_level = level;
        update_required = true;

        static bool boot_event = false;
        if (level > BATTERY_LEVEL_TO_DOWNLOAD) {
            if (!boot_event) {
                boot_event = true;
                bootApp_sendEvent(BOOT_APP_EVENT_START_AGAIN);
            }
        } else {
            boot_event = false;
        }
    }

    if (is_charger_connected != pwr_isChargerConnected()) {
        is_charger_connected = !is_charger_connected;
        if (is_charger_connected){
            widgetList_label[0].text = charging_title;
            widgetList_label[1].text = charging_progress;
        } else {
            widgetList_label[0].text = low_battery_title;
            widgetList_label[1].text = low_battery_progress;
        }
        update_required = true;

        for (int i = 0; i < 2; i++) {
            paint_clearWindow(widgetList_label[i].position.x, widgetList_label[i].position.y, 120,
                              FONT_GET_HEIGHT(widgetList_label[i].font));
            paint_drawStringEn(widgetList_label[i].position.x, widgetList_label[i].position.y,
                               widgetList_label[i].text, widgetList_label[i].font,
                               widgetList_label[i].text_align, PAINT_COLOR);
        }

#ifndef REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS
        if (is_charger_connected) {
            paint_drawBitMap(widget_img_title.position.x, widget_img_title.position.y,
                             widget_img_title.img_dsc, PAINT_COLOR | PAINT_BACKGROUND);
        } else {
            paint_clearWindow(imgWidget_lightingPart.position.x,
                              imgWidget_lightingPart.position.y, 24, 32);
            paint_drawBitMap(imgWidget_attentionPart.position.x, imgWidget_attentionPart.position.y,
                             imgWidget_attentionPart.img_dsc, PAINT_COLOR | PAINT_BACKGROUND);
        }
#endif /* REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS */
    }

    if (update_required) {
        /* Show big letters with the battery level */
        paint_clearWindow(widget_labelAnimation.position.x, widget_labelAnimation.position.y, 140,
                          20);
        paint_drawStringEn(widget_labelAnimation.position.x, widget_labelAnimation.position.y,
                           widget_labelAnimation.getLabelText(), widget_labelAnimation.font,
                           TEXT_ALIGN_CENTER, PAINT_BACKGROUND | PAINT_COLOR);
    }

    if (is_charger_connected) {
        refreshScreenTime();
    }
    
    update_required |= guiCore_processButton(&button_goBack);
    if (guiCore_processButton(&widgetButton_turnOff)) {
        paint_drawButton(&widgetButton_turnOff);
        update_required |= true;
    }
    update_required |= drawBatteryLevel(false, false);

    return update_required;
}

/**
 * @brief Get the Battery Level string.
 * @return char* Pointer to text to display.
 */
static char *getBatteryLevelString(void)
{
    static char battery_line[sizeof("100%%")];
    if (pwr_isChargerConnected()) {
        sprintf(battery_line, "%3.1d%%", last_battery_level > 100 ? 100 : last_battery_level);
    } else {
        sprintf(battery_line, "%3.1d%%", last_battery_level > 100 ? 100 : last_battery_level);
    }
    return battery_line;
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
 * @brief Handler when the "Turn off" button has been released
 * @return None
 */
static void turnOffButtonReleased(void)
{
    taskGui_sendEvent(TASK_GUI_SHIPPING);
}

/**
 * @brief Handler when the back button has been pressed.
 * @return None
 */
static void goBackPressed(void)
{
    taskGui_sendEvent(TASK_GUI_WAIT_USER_EVENT);
}
