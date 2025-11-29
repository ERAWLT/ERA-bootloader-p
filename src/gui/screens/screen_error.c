#include "screens.h"
#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"

#include "gui_core.h"
#include "user_logic.h"
#include "task_gui.h"
#include "boot_app.h"
#include "bootloader.h"

/** Uncomment to exit after the timeout in seconds has expired */
// #define AUTO_EXIT_AT_TIMEOUT_ENABLED

/** Timeout in seconds to exit from screen, when AUTO_EXIT_AT_TIMEOUT_ENABLED */
#define SCREEN_ERROR_TIMEOUT_SEC (30)

extern const bitmap_dsc_s img_download_error; 
extern bool drawBatteryLevel(bool full_update, bool invert);

static char *getTitleString(void);
static void drawAnimation(void);
static void continueReleased(void);
#ifdef AUTO_EXIT_AT_TIMEOUT_ENABLED
static char *getButtonText(void);
#endif

/** Base image */
static const guiWidget_img_s widget_img_title = {
    .position.x = 66,
    .position.y = 54+8,
    .img_dsc = &img_download_error,
};

/** Text on screen */
static guiWidget_labelAnimation_s labelAnimation_title = {
    .getLabelText = getTitleString,
    .position.x = 9,
    .position.y = 5,
    .font = HWLT_FONT_8,
};

/** Text on screen */
static guiWidget_label_s labelAnimation_header = {
    .text = "DOWNLOAD\nERROR",
    .position.x = 11,
    .position.y = 110,
    .font = HWLT_FONT_16_BOLD,
};

/** Text on the screen */
static guiWidget_label_s widgetList_label[] = {
    {
     .text = "An error occurred during\nthe download process. Please\ntry again.",
     .position.x = 10,
     .position.y = 42 + 110,
     .font = HWLT_FONT_8,
     },
};

/** Additional text on the screen when switching to sleep mode */
static guiWidget_label_s widgetList_sleep[] = {
    {
     .position.x = 8,
     .position.y = 216,
     .text_align = TEXT_ALIGN_CENTER,
     .text = "Press the button"
                " to continue.", .font = HWLT_FONT_8_BOLD,
     },
};

/* Button to continue */
static guiWidget_button_s widgetButton_continue = {
    .position.x = 8,
#ifdef AUTO_EXIT_AT_TIMEOUT_ENABLED
    .position.y = 164+8,
#else
    .position.y = 8+216, // 164+52,
#endif
    .width = 160,
#ifdef AUTO_EXIT_AT_TIMEOUT_ENABLED
    .height = 40+44,
#else
    .height = 32,
#endif
    .color = 1,
    .text = "CONTINUE",
    .releasedCallback = continueReleased,
};

static uint32_t last_passed_sec = 0;

void screenError_switch(void)
{
    last_passed_sec = 0;
    setScreenTimeoutMs(60*1000);
#ifndef REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS
    paint_drawBitMap(widget_img_title.position.x, widget_img_title.position.y,
                     widget_img_title.img_dsc, PAINT_COLOR);
#endif
    paint_drawStringEn(labelAnimation_title.position.x, labelAnimation_title.position.y,
                       labelAnimation_title.getLabelText(), labelAnimation_title.font, TEXT_ALIGN_LEFT,
                       PAINT_COLOR);

    paint_drawStringEn(labelAnimation_header.position.x, labelAnimation_header.position.y,
                       labelAnimation_header.text, labelAnimation_header.font, TEXT_ALIGN_CENTER,
                       PAINT_COLOR);

    for (int i = 0; i < sizeof(widgetList_label) / sizeof(*widgetList_label); i++) {
        paint_drawStringEn(widgetList_label[i].position.x, widgetList_label[i].position.y,
                           widgetList_label[i].text, widgetList_label[i].font, TEXT_ALIGN_CENTER,
                           PAINT_COLOR);
    }
    drawBatteryLevel(true, false);
    drawAnimation();
}

bool screenError_poll(void)
{
    bool going_to_sleep = false;
    bool update_required = false;

    uint32_t passed_sec = getPassedTimeMs(getScreenTimeMs()) / 1000;
    if (last_passed_sec != passed_sec && last_passed_sec < SCREEN_ERROR_TIMEOUT_SEC) {
        if (passed_sec >= SCREEN_ERROR_TIMEOUT_SEC) {
            passed_sec = SCREEN_ERROR_TIMEOUT_SEC;
#ifdef AUTO_EXIT_AT_TIMEOUT_ENABLED
            continueReleased();
#else
            setScreenTimeoutMs(0);
            going_to_sleep = true;
#endif /* AUTO_EXIT_AT_TIMEOUT_ENABLED */
        }
        last_passed_sec = passed_sec;
#ifdef AUTO_EXIT_AT_TIMEOUT_ENABLED
        update_required = true;
#endif /* AUTO_EXIT_AT_TIMEOUT_ENABLED */
    }

    update_required |= guiCore_processButton(&widgetButton_continue);
    update_required |= drawBatteryLevel(false, false);
    
    if (going_to_sleep) {
        paint_clearWindow(widgetButton_continue.position.x, widgetButton_continue.position.y,
                          widgetButton_continue.width, widgetButton_continue.height);
        for (int i = 0; i < sizeof(widgetList_sleep) / sizeof(*widgetList_sleep); i++) {
            paint_drawStringEn(widgetList_sleep[i].position.x, widgetList_sleep[i].position.y,
                               widgetList_sleep[i].text, widgetList_sleep[i].font,
                               TEXT_ALIGN_CENTER, PAINT_COLOR);
        }
        update_required = true;
    } else if (update_required) {
        drawAnimation();
    }

    return update_required;
}

static void drawAnimation(void)
{
#ifdef AUTO_EXIT_AT_TIMEOUT_ENABLED
    widgetButton_continue.text = getButtonText();
#else
    
#endif
    paint_drawButton(&widgetButton_continue);
}

/**
 * @brief Get the error string.
 * @return char* Pointer to text to display.
 */
static char *getTitleString(void)
{
    static char timeline[sizeof("ERROR #XXX")];
    snprintf(timeline, sizeof(timeline), "Error %3.1d", bootloader_getDownloadStatus()%100);
    return timeline;
}

#ifdef AUTO_EXIT_AT_TIMEOUT_ENABLED
/**
 * @brief Get the text on the 'continue' button including the passed time.
 * @return char* Pointer to text to display.
 */
static char *getButtonText(void)
{
    static char timeline[sizeof("CONTINUE xx:xx")];
    sprintf(timeline, "CONTINUE\n\n0:%2.2lu", SCREEN_ERROR_TIMEOUT_SEC - last_passed_sec);
    return timeline;
}
#endif /* AUTO_EXIT_AT_TIMEOUT_ENABLED */

/**
 * @brief Exit from screen.
 * @return None.
 */
static void continueReleased(void)
{
    bootApp_sendEvent(BOOT_APP_EVENT_BOOT_TO_MF);
}
