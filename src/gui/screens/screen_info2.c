#include "screens.h"
#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"

#include "board_partial_download.h"
#include "user_logic.h"
#include "gui_core.h"
#include "task_gui.h"

/** Switch to the download screen automatically when download starts */
// #define AUTO_SWITCH_TO_DOWNLOAD

extern const bitmap_dsc_s img_companionApp; 
extern const bitmap_dsc_s img_Close; 
extern const bitmap_dsc_s img_infoScreenBangs; 
extern bool drawBatteryLevel(bool full_update, bool invert);

static void CloseButtonReleased(void);

static guiWidget_label_s widgetLabel_title = {
    .position.x = 9,
    .position.y = 5,
    .font = HWLT_FONT_8,
    .text = "BL Info",
};

/** Info QR */
static const guiWidget_img_s imgWidget_qr = {
    .position.x = 76/2, // 31,
    .position.y = 60, // 75,
    .img_dsc = &img_companionApp,
};

/** Bangs for info screen */
static const guiWidget_img_s imgWidget_bangs = {
    .position.x = 0,
    .position.y = 0,
    .img_dsc = &img_infoScreenBangs,
};

/** Back img */
static const guiWidget_img_s imgWidget_close = {
    .position.x = 150,
    .position.y = 27,
    .img_dsc = &img_Close,
};

/** Back button */
static guiWidget_button_s button_goBack = {
    .position.x = 145,
    .position.y = 25,
    .width = 30,
    .height = 30,
    .color = 1,
    .text = NULL,
    .pressedCallback = CloseButtonReleased,
};

/** Text on screen */
static const guiWidget_label_s widgetList_label[] = {
    {
     .position.x = 8,
     .position.y = 181,
     .text_align = TEXT_ALIGN_CENTER,
     .text = "Download and install the companion mobile app for"
                "your hardware wallet.", .font = HWLT_FONT_8,
     },
};

/* Button to turn off */
static guiWidget_button_s widgetButton_close = {
    .position.x = 8,
    .position.y = 8+216,
    .width = 160,
    .height = 32,
    .color = 1,
    .text = "CLOSE",
    .releasedCallback = CloseButtonReleased,
};

void screenInfo2_switch(void)
{
    paint_drawBitMap(imgWidget_bangs.position.x, imgWidget_bangs.position.y,
                     imgWidget_bangs.img_dsc, PAINT_COLOR);
    paint_drawBitMap(imgWidget_close.position.x, imgWidget_close.position.y,
                     imgWidget_close.img_dsc, PAINT_COLOR);

    /* The timeout of the previous screen is used */

    paint_drawStringEn(widgetLabel_title.position.x, widgetLabel_title.position.y,
                       widgetLabel_title.text, widgetLabel_title.font, TEXT_ALIGN_LEFT,
                       PAINT_COLOR_INVERT);
    for (int i = 0; i < sizeof(widgetList_label) / sizeof(*widgetList_label); i++) {
        paint_drawStringEn(widgetList_label[i].position.x, widgetList_label[i].position.y,
                           widgetList_label[i].text, widgetList_label[i].font,
                           widgetList_label[i].text_align, PAINT_COLOR);
    }
    drawBatteryLevel(true, true);
    paint_drawButton(&widgetButton_close);

    paint_drawBitMap(imgWidget_qr.position.x, imgWidget_qr.position.y, imgWidget_qr.img_dsc,
                     PAINT_COLOR);
}

bool screenInfo2_poll(void)
{
    bool update_required = false;
    update_required |= drawBatteryLevel(false, true);
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
    if (guiCore_processButton(&widgetButton_close)) {
        paint_drawButton(&widgetButton_close);
        update_required |= true;
    }

    return update_required;
}

/**
 * @brief Handler when the "Close" button has been released
 * @return None
 */
static void CloseButtonReleased(void)
{
    taskGui_sendEvent(TASK_GUI_INFO);
}
