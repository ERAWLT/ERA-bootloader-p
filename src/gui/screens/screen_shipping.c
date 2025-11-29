#include "screens.h"
#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"

#include "endOfWork_manager.h"
#include "board_pwr.h"
#include "user_logic.h"
#include "gui_core.h"
#include "task_gui.h"
#include "common_time.h"
#if HWLT_BOARD_REVISION_NUM != 1
    #include "board_smart_button.h"
#endif

/** Check battery level before entering to shipping mode */
// #define SHOW_LOW_BATTERY_LEVEL

/** Use era logo and text below like initial firmware */
#define USE_POWER_OFF_SCREEN_SAME_AS_IF 1

#define BATT_REQUEST_TIMEOUT_SEC 10   /*!< We should go sleep by screen timeout */
#define ATTEMPT_TIMEOUT_MS       1000 /*!< Waiting time before next attempt */
#define ATTEMPTS_BEFORE_TURN_OFF 2    /*!< Attempts for shipping mode before just turn off */

#define LOGO_AREA_X   (43 + 8)    /* The horizontal position of the start of the Logo */
#define LOGO_AREA_Y   (72 + 7)    /* The vertical position of the start of the Logo */

extern const bitmap_dsc_s img_turn_off;
extern const bitmap_dsc_s img_logo;

/** We should jump to the low battery screen only once (at fuel meter failure). */
static int attepts_num = 0;
static uint32_t time_to_attempt_ms = 0;


#if (USE_POWER_OFF_SCREEN_SAME_AS_IF)
/* Use old BL screen with a chip */
/** Base image (Era logo) */
static const guiWidget_img_s widget_img_title = {
    .position.x = LOGO_AREA_X, // 64
    .position.y = LOGO_AREA_Y, // 88
    .img_dsc = &img_logo,
};

/** Text on screen */
static const guiWidget_label_s widgetList_label[] = {
    {
        .position.x = 0,
        .position.y = 235, //
        .text_align = TEXT_ALIGN_CENTER,
        .text = "Place the device on the\n" 
                "charging pad "
#if (HWLT_BOARD_REVISION_NUM == 2)
                "and press the button "
#endif
                "to turn it on", 
        .font = HWLT_FONT_8,
    },
};

#else /* Use old BL screen with a chip */
/** Base image (chip) */
static const guiWidget_img_s widget_img_title = {
    .position.x = 8, // 64
    .position.y = 80, // 88
    .img_dsc = &img_turn_off,
};
/** Text on screen */
static const guiWidget_label_s widgetList_label[] = {
    {
     .position.x = 18,
     .position.y = 143,
     .text_align = TEXT_ALIGN_CENTER,
     .text = "TURN ON",
     .font = HWLT_FONT_16_BOLD,
     },
     {
     .position.x = 18,
     .position.y = 143 + 23,
     .text_align = TEXT_ALIGN_CENTER,
     .text = "Place the device on"
                " the charging pad"
#if (HWLT_BOARD_REVISION_NUM == 2)
                " and press the button"
#endif
                " to turn it on.", 
    .font = HWLT_FONT_8_BOLD,
     },
};
#endif /* USE_POWER_OFF_SCREEN_SAME_AS_IF == 1 */

void screenShipping_switch(void)
{
    setScreenTimeoutMs(BATT_REQUEST_TIMEOUT_SEC * 1000);

#ifndef REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS
    paint_drawBitMap(widget_img_title.position.x, widget_img_title.position.y,
                     widget_img_title.img_dsc, PAINT_COLOR);
#endif
    for (int i = 0; i < sizeof(widgetList_label) / sizeof(*widgetList_label); i++) {
        paint_drawStringEn(widgetList_label[i].position.x, widgetList_label[i].position.y,
                            widgetList_label[i].text, widgetList_label[i].font,
                            widgetList_label[i].text_align, PAINT_COLOR);
    }
    time_to_attempt_ms = sysClock_getCurrentTime();
}

bool screenShipping_poll(void)
{
#if (HWLT_BOARD_REVISION_NUM != 1)
    do {
        if (!sysClock_IsTimePassed(time_to_attempt_ms, ATTEMPT_TIMEOUT_MS)) {
            break;
        }
        time_to_attempt_ms = sysClock_getCurrentTime();
        
        if (pwr_isChargerConnected()) {
            printf("Charging!\n");
            break;
        }
        
        if (endOfWork_shippingMode() == false) {
            printf("Request for shipping mode failed!\n");
#ifdef SHOW_LOW_BATTERY_LEVEL
            if (attepts_num == 0) {
                /* First failure time */
                taskGui_sendEvent(TASK_GUI_LOW_BATTERY_EVENT);
            }
#endif
            /*  If we hit the screen for the second time, we go to just sleep
                by screen timeout (after some addtional attempts). */
            attepts_num++;
            if (attepts_num > ATTEMPTS_BEFORE_TURN_OFF) {
                printf("Request for power off started...\n");
                endOfWork_powerOff();
                /* After 1 sec, the power should be turned off. */
            }
        }
    } while (0);
#endif
    return false;
}
