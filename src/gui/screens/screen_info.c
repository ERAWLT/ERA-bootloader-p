#include "screens.h"
#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"

#include "board_partial_download.h"
#include "user_logic.h"
#include "gui_core.h"
#include "task_gui.h"

/** Uncomment to show additional info page by info button */
#define ADDITIONAL_INFO_ENABLED

/** Switch to the download screen automatically when download starts */
// #define AUTO_SWITCH_TO_DOWNLOAD

#define SHIFT_Y     (20u)

extern const bitmap_dsc_s img_Info16x16;
extern bool drawBatteryLevel(bool full_update, bool invert);

static void goInfoPressed(void);
static void goBackPressed(void);
static void NextButtonReleased(void);

static guiWidget_label_s widgetLabel_title = {
    .position.x = 9,
    .position.y = 5,
    .font = HWLT_FONT_8,
    .text = "BL Info",
};

#ifdef ADDITIONAL_INFO_ENABLED
/** Info img of info button */
static const guiWidget_img_s imgWidget_info = {
    .position.x = 148,
    .position.y = 25,
    .img_dsc = &img_Info16x16,
};

/** Show info button */
static guiWidget_button_s button_goInfo = {
    .position.x = 140,
    .position.y = 20,
    .width = 30,
    .height = 30,
    .color = 1,
    .text = NULL,
    .pressedCallback = goInfoPressed,
};
#endif /* ADDITIONAL_INFO_ENABLED */

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

/** Text on screen */
static const guiWidget_label_s widgetList_label[] = {
    {
     .text = "<",
     .position.x = 8,
     .position.y = SHIFT_Y + 4,
     .text_align = TEXT_ALIGN_LEFT,
     .font = HWLT_FONT_16_BOLD,
     },
    {
     .text = "DOWNLOAD UPDATE",
     .position.x = 32,
     .position.y = SHIFT_Y + 8,
     .text_align = TEXT_ALIGN_LEFT,
     .font = HWLT_FONT_8_BOLD,
     },
    {
     .text = "1. Open the Mobile App",
     .position.x = 10,
     .position.y = 54,
     .text_align = TEXT_ALIGN_LEFT,
     .font = HWLT_FONT_8_BOLD,
     },
    {
     .position.x = 10,
     .position.y = 54 + 10 * 1 + 1,
     .text_align = TEXT_ALIGN_LEFT,
     .text = "Scan the QR code in the manual to download the ERA wallet companion app, then "
     "open it.", .font = HWLT_FONT_8,
     },
    {
     .text = "2. Download the Firmware",
     .position.x = 10,
     .position.y = 54 + 10 * 5 + 6 + 1,
     .text_align = TEXT_ALIGN_LEFT,
     .font = HWLT_FONT_8_BOLD,
     },
    {
     .position.x = 10,
     .position.y = 54 + 10 * 5 + 6 + 10 * 1 + 2,
     .text_align = TEXT_ALIGN_LEFT,
     .text = "Use the app to download the latest ERA wallet firmware.",
     .font = HWLT_FONT_8,
     },
    {
     .text = "3. Update the Wallet",
     .position.x = 10,
     .position.y = 54 + 10 * 5 + 6 + 10 * 4 + 8 + 2,
     .text_align = TEXT_ALIGN_LEFT,
     .font = HWLT_FONT_8_BOLD,
     },
    {
     .position.x = 10,
     .position.y = 54 + 10 * 5 + 6 + 10 * 4 + 8 + 10 * 1 + 3,
     .text_align = TEXT_ALIGN_LEFT,
     .text =
     "Follow the instructions in the app to complete the firmware update for your hardware "
     "wallet.", .font = HWLT_FONT_8,
     },
};

/* Button to next screen */
static guiWidget_button_s widgetButton_next = {
    .position.x = 8,
    .position.y = 8+216, // 164 + 52,
    .width = 160,
    .height = 32,
    .color = 1,
    .text = "NEXT",
    .releasedCallback = NextButtonReleased,
};

void screenInfo_switch(void)
{
    /* The timeout of the previous screen is used */

    paint_drawStringEn(widgetLabel_title.position.x, widgetLabel_title.position.y,
                       widgetLabel_title.text, widgetLabel_title.font, TEXT_ALIGN_LEFT,
                       PAINT_COLOR);
    for (int i = 0; i < sizeof(widgetList_label) / sizeof(*widgetList_label); i++) {
        paint_drawStringEn(widgetList_label[i].position.x, widgetList_label[i].position.y,
                           widgetList_label[i].text, widgetList_label[i].font,
                           widgetList_label[i].text_align, PAINT_COLOR);
    }
    drawBatteryLevel(true, false);
    paint_drawButton(&widgetButton_next);
#ifdef ADDITIONAL_INFO_ENABLED
    paint_drawBitMap(imgWidget_info.position.x, imgWidget_info.position.y, imgWidget_info.img_dsc,
                     PAINT_COLOR);
#endif
}

bool screenInfo_poll(void)
{
    bool update_required = false;
    update_required |= drawBatteryLevel(false, false);
#ifdef AUTO_SWITCH_TO_DOWNLOAD
    size_t total_size, downloaded_size;
    if (partialDownload_getProgress(&total_size, &downloaded_size) ==
        PARTIAL_DOWNLOAD_COMMAND_COMPLETED) {
        if (downloaded_size > 0 && total_size != 0) {
            taskGui_sendEvent(TASK_GUI_DOWNLOAD_EVENT); /* when nfc receive data */
        }
    }
#endif /* AUTO_SWITCH_TO_DOWNLOAD */
    update_required |= guiCore_processButton(&button_goBack);
#ifdef ADDITIONAL_INFO_ENABLED
    update_required |= guiCore_processButton(&button_goInfo);
#endif
    if (guiCore_processButton(&widgetButton_next)) {
        paint_drawButton(&widgetButton_next);
        update_required |= true;
    }

    return update_required;
}

/**
 * @brief Handler when the back button has been pressed.
 * @return None
 */
static void goBackPressed(void)
{
    taskGui_sendEvent(TASK_GUI_WAIT_USER_EVENT);
}

/**
 * @brief Handler when the info button has been pressed.
 * @return None
 */
static void goInfoPressed(void)
{
    taskGui_sendEvent(TASK_GUI_MORE_INFO);
}
/**
 * @brief Handler when the "Next" button has been released
 * @return None
 */
static void NextButtonReleased(void)
{
    taskGui_sendEvent(TASK_GUI_DOWNLOAD_EVENT);
}
