#include "screens.h"

#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"

#include "boot_app.h"
#include "gui_core.h"
#include "user_logic.h"
#include "task_gui.h"
#include "board_nfc.h"
#include "task_nfc_transport.h"
#include "board_batt_control.h"
#include "bootloader.h"
#include "common_time.h"

/** Uncomment to enable ability to erase secure storage and main-firmware. */
// #define ERASE_BUTTON_ENABLED

#ifdef ERASE_BUTTON_ENABLED
    /** Erase SS or MF after this delay while button under touch */
    #define ERASE_AFTER_TIMEOUT_MS 5000

extern void bootloader_cleanSS(void);
extern void bootloader_cleanMF(void);

static void eraseSSbuttonPressed(void);
static void eraseMFbuttonPressed(void);
static void switchContext(void);

static uint32_t catching_time_erase_SS = 0;
static uint32_t catching_time_erase_MF = 0;
#endif /* ERASE_BUTTON_ENABLED */

extern const bitmap_dsc_s img_firmware_error;
extern const bitmap_dsc_s img_chip;
extern guiWidget_label_s widgetLabel_deviceInfo[3];
extern bool drawBatteryLevel(bool full_update, bool invert);

static inline uint8_t get_current_level();
static void laterButtonReleased(void);
static void downloadButtonReleased(void);
static void turnOffButtonReleased(void);
extern char *bootloader_getPrimaryVersionOfMF(void);

const char error_title_label[] = "MF Error";

static guiWidget_label_s widgetLabel_title = {
    .position.x = 9,
    .position.y = 5,
    .font = HWLT_FONT_8,
    .text = "BL Menu"
};

/** Text on screen when MF is ok */
static const guiWidget_label_s widgetList_specLabel[] = {
    {
     .text = "BOOTLOADER",
     .position.x = 11,
     .position.y = 45,
     .font = HWLT_FONT_16_BOLD,
     },
};

/** Version of MF in primary slot */
static guiWidget_labelAnimation_s label_primaryVersionOfMF = {
    .getLabelText = bootloader_getPrimaryVersionOfMF,
    .position.x = 11,
    .position.y = 45 + 22,
    .font = HWLT_FONT_8,
};


/** Base image */
static guiWidget_img_s widget_img_title = {
    .position.x = 66,
    .position.y = 30,
    .img_dsc = &img_chip,
};

/** Text on screen */
static const guiWidget_label_s widgetList_label[] = {
    {
     .text = "FIRMWARE\nERROR",
     .position.x = 11,
     .position.y = 87,
     .font = HWLT_FONT_16_BOLD,
     },
    {
     .text = "An error occurred while starting the firmware. Please download and install the "
     "latest version.", .position.x = 16,
     .position.y = 87 + 42,
     .font = HWLT_FONT_8,
     },
};

/** Buttons on screen */
static guiWidget_button_s widgetButtons_NoMF[] = {
    {
     .position.x = 8,
     .position.y = 188, // 164 + 8,
     .width = 160,
     .height = 32,
     .color = 1,
     .text = "DOWNLOAD",
     .releasedCallback = downloadButtonReleased,
     },
    {
     .position.x = 8,
     .position.y = 8+216, // 164 + 52,
     .width = 160,
     .height = 32,
     .color = 0,
     .text = "TURN OFF",
     .releasedCallback = turnOffButtonReleased,
     },
};

/* Button on screen when MF is ok (start) */
static guiWidget_button_s widgetButtons_started[] = {
    {
     .position.x = 8,
     .position.y = 188, // 80 + 20 + 40*1,
     .width = 160,
     .height = 32,
     .color = 0,
     .text = "EXIT",
     .releasedCallback = laterButtonReleased,
     },
#ifdef ERASE_BUTTON_ENABLED
    {
     .position.x = 8,
     .position.y = 152, // 80 + 20 + 40*2,
     .width = 160,
     .height = 32,
     .color = 0,
     .text = "SPECIAL...",
     .releasedCallback = switchContext,
     },
#else /* ERASE_BUTTON_ENABLED */
    {
     .position.x = 8,
     .position.y = 152, // 80 + 20 + 40*2,
     .width = 160,
     .height = 32,
     .color = 1,
     .text = "DOWNLOAD",
     .releasedCallback = downloadButtonReleased,
     },
#endif /* ERASE_BUTTON_ENABLED */
    {
     .position.x = 8,
     .position.y = 8+216, //80 + 20 + 40*3,
     .width = 160,
     .height = 32,
     .color = 0,
     .text = "TURN OFF",
     .releasedCallback = turnOffButtonReleased,
     },
};

#ifdef ERASE_BUTTON_ENABLED
/* Button on screen when MF is ok (special) */
static guiWidget_button_s widgetButtons_specDownload[] = {
    {
     .position.x = 8,
     .position.y = 152 - 32 - 4,
     .width = 160,
     .height = 32,
     .color = 0,
     .text = "ERASE SS!",
     .pressedCallback = eraseSSbuttonPressed,
     },
     {
     .position.x = 8,
     .position.y = 152,
     .width = 160,
     .height = 32,
     .color = 0,
     .text = "ERASE MF!",
     .pressedCallback = eraseMFbuttonPressed,
     },
    {
     .position.x = 8,
     .position.y = 188,
     .width = 160,
     .height = 32,
     .color = 0,
     .text = "DOWNLOAD",
     .releasedCallback = downloadButtonReleased,
     },
    {
     .position.x = 8,
     .position.y = 8+216,
     .width = 160,
     .height = 32,
     .color = 0,
     .text = "BACK...",
     .releasedCallback = switchContext,
     },
};
#endif /* ERASE_BUTTON_ENABLED */

static bool specContextOn = false;

void screenWaitUpdate_switch(void)
{
    specContextOn = false;
    setScreenTimeoutMs(5 * 60 * 1000);
    
    if (bootloader_isThereValidMF()) {
        for (int i = 0; i < sizeof(widgetList_specLabel) / sizeof(*widgetList_specLabel); i++) {
            paint_drawStringEn(widgetList_specLabel[i].position.x,
                               widgetList_specLabel[i].position.y, widgetList_specLabel[i].text,
                               widgetList_specLabel[i].font, TEXT_ALIGN_CENTER, PAINT_COLOR);
        }
        for (int i = 0; i < sizeof(widgetButtons_started) / sizeof(*widgetButtons_started); i++) {
            paint_drawButton(&widgetButtons_started[i]);
        }
        paint_drawStringEn(label_primaryVersionOfMF.position.x, label_primaryVersionOfMF.position.y,
                           label_primaryVersionOfMF.getLabelText(), label_primaryVersionOfMF.font,
                           TEXT_ALIGN_CENTER, PAINT_COLOR);
        for (int i = 0; i < 3; i++) {
            paint_drawStringEn(widgetLabel_deviceInfo[i].position.x,
                               widgetLabel_deviceInfo[i].position.y, widgetLabel_deviceInfo[i].text,
                               widgetLabel_deviceInfo[i].font, TEXT_ALIGN_CENTER, PAINT_COLOR);
        }
    } else {
        widgetLabel_title.text = error_title_label;
        widget_img_title.img_dsc = &img_firmware_error;
        for (int i = 0; i < sizeof(widgetList_label) / sizeof(*widgetList_label); i++) {
            paint_drawStringEn(widgetList_label[i].position.x, widgetList_label[i].position.y,
                               widgetList_label[i].text, widgetList_label[i].font,
                               TEXT_ALIGN_CENTER, PAINT_COLOR);
        }
        for (int i = 0; i < sizeof(widgetButtons_NoMF) / sizeof(*widgetButtons_NoMF); i++) {
            paint_drawButton(&widgetButtons_NoMF[i]);
        }
#ifndef REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS
        paint_drawBitMap(widget_img_title.position.x, widget_img_title.position.y,
                         widget_img_title.img_dsc, PAINT_COLOR);
#endif
    }

    paint_drawStringEn(widgetLabel_title.position.x, widgetLabel_title.position.y,
                       widgetLabel_title.text, widgetLabel_title.font, TEXT_ALIGN_LEFT,
                       PAINT_COLOR);
    drawBatteryLevel(true, false);
}

bool screenWaitUpdate_poll(void)
{
    bool update_required = false;
    guiWidget_button_s *screenButtons = widgetButtons_NoMF;
    int num_of_buttons = sizeof(widgetButtons_NoMF) / sizeof(*widgetButtons_NoMF);
    if (bootloader_isThereValidMF()) {
        screenButtons = widgetButtons_started;
        num_of_buttons = sizeof(widgetButtons_started) / sizeof(*widgetButtons_started);
#ifdef ERASE_BUTTON_ENABLED
        static bool contextBefore = false;
        if (specContextOn) {
            screenButtons = widgetButtons_specDownload;
            num_of_buttons = sizeof(widgetButtons_specDownload) /
                             sizeof(*widgetButtons_specDownload);
        }
        if (contextBefore != specContextOn) {
            update_required = true;
            contextBefore = specContextOn;
            paint_clearWindow(widgetList_specLabel[0].position.x,
                              widgetList_specLabel[0].position.y, 160, 20);
            paint_clearWindow(widgetButtons_specDownload[0].position.x,
                              widgetButtons_specDownload[0].position.y,
                              widgetButtons_specDownload[0].width,
                              widgetButtons_specDownload[0].height);
            if (specContextOn) {
                paint_drawStringEn(label_primaryVersionOfMF.position.x,
                                   label_primaryVersionOfMF.position.y,
                                   label_primaryVersionOfMF.getLabelText(),
                                   label_primaryVersionOfMF.font, TEXT_ALIGN_CENTER, PAINT_COLOR);
            } else {
                paint_drawStringEn(widgetList_specLabel[0].position.x,
                                   widgetList_specLabel[0].position.y, widgetList_specLabel[0].text,
                                   widgetList_specLabel[0].font, TEXT_ALIGN_CENTER, PAINT_COLOR);
            }
        }
#endif /* ERASE_BUTTON_ENABLED */
    }

    for (int i = 0; i < num_of_buttons; i++) {
        update_required |= guiCore_processButton(&screenButtons[i]); /*!< First check - whether
                                                                            the button is pressed */
        if (update_required) {
            paint_drawButton(&screenButtons[i]);
        }
    }
    update_required |= drawBatteryLevel(false, false);

#ifdef ERASE_BUTTON_ENABLED
    if (specContextOn) {
        static int touch_progress, remaining_time_ms;
        static bool reaction_done = false;
        guiWidget_button_s *eraseButton = NULL;
        void (*eraseHandler)(void) = NULL;
        if (widgetButtons_specDownload[0].pressed) {
            remaining_time_ms = ERASE_AFTER_TIMEOUT_MS -
                                sysClock_getPassedTime(catching_time_erase_SS);
            eraseButton = &widgetButtons_specDownload[0];
            eraseHandler = bootloader_cleanSS;
        } else if (widgetButtons_specDownload[1].pressed) {
            remaining_time_ms = ERASE_AFTER_TIMEOUT_MS -
                                sysClock_getPassedTime(catching_time_erase_MF);
            eraseButton = &widgetButtons_specDownload[1];
            eraseHandler = bootloader_cleanMF;
        } else {
            touch_progress = 0;
            reaction_done = false;
        }
        if (eraseButton != NULL && eraseHandler != NULL) {
            update_required = true;
            if (touch_progress == 100) {
                static char *successString = "DONE";
                eraseButton->text = successString;
                paint_drawButton(eraseButton);
                if (!reaction_done) {
                    reaction_done = true;
                    eraseHandler();
                    eraseButton->color = !eraseButton->color;
                }
            } else {
                touch_progress = remaining_time_ms > 0
                                     ? 100 - remaining_time_ms * 100 / ERASE_AFTER_TIMEOUT_MS
                                     : 100;

                guiWidget_bar_s bar = {
                    .height = eraseButton->height,
                    .width = eraseButton->width,
                    .position.x = eraseButton->position.x,
                    .position.y = eraseButton->position.y,
                };
                paint_drawProgressBar(&bar, touch_progress);
            }
        }
    }
#endif /* ERASE_BUTTON_ENABLED */

    return update_required;
}

#ifdef ERASE_BUTTON_ENABLED
static void eraseSSbuttonPressed(void)
{
    catching_time_erase_SS = sysClock_getCurrentTime();
}

static void eraseMFbuttonPressed(void)
{
    catching_time_erase_MF = sysClock_getCurrentTime();
}

/**
 * @brief Switch the context on screen.
 * @return None.
 */
static void switchContext(void) {
    specContextOn = !specContextOn;
}
#endif /* ERASE_BUTTON_ENABLED */

/**
 * @brief Handler when the "download" button has been released
 * @return None
 */
static void downloadButtonReleased(void)
{
    if (get_current_level() >= BATTERY_LEVEL_TO_DOWNLOAD) {
        taskGui_sendEvent(TASK_GUI_INFO);
#if (HWLT_BOARD_REVISION_NUM == 1)
        nfc_enableRfConnection();
#else
        nfcTransport_run(900);
#endif
    } else {
        taskGui_sendEvent(TASK_GUI_LOW_BATTERY_EVENT);
    }
}

/**
 * @brief Handler when the "later" button has been released
 * @return None
 */
static void laterButtonReleased(void)
{
    bootApp_sendEvent(BOOT_APP_EVENT_LATER_CLICKED);
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
