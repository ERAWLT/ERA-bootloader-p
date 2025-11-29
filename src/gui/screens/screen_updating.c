#include "screens.h"
#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"

#include "bootloader.h"
#include "user_logic.h"
#include "gui_core.h"
#include "boot_app.h"

/** Uncomment to use a chip animation instead a box. */
// #define USE_CHIP_ANIMATION

/** Uncomment to show percentage */
// #define SHOW_PERCENT_LABLE

/** Uncomment to enable updating screen animation */
// #define ENABLE_UPDATING_SCREEN_ANIMATION

extern const bitmap_dsc_s img_chip_progress; 
extern bool drawBatteryLevel(bool full_update, bool invert);

static char *getProgressLevelString(void);
static int getProgressUpdating(void);

/** Text on screen */
static const guiWidget_label_s widgetList_label[] = {
    {
     .text = "Updating",
     .position.x = 9,
     .position.y = 5,
     .font = HWLT_FONT_8,
     .text_align = TEXT_ALIGN_LEFT,
     },
    {
     .text = "UPDATING",
     .position.x = 8,
     .position.y = 140,
     .font = HWLT_FONT_16_BOLD,
     .text_align = TEXT_ALIGN_CENTER,
     },
    {
     .position.x = 8,
     .position.y = 192,
     .font = HWLT_FONT_8,
     .text = "Your ERA Wallet will restart automatically once the process is complete.",
     .text_align = TEXT_ALIGN_CENTER,
     },
};

#ifdef SHOW_PERCENT_LABLE
static guiWidget_label_s label_updatingProgress = {
    .position.x = 9,
    .position.y = 157,
    .font = HWLT_FONT_16_BOLD,
    .text = NULL,
    .text_align = TEXT_ALIGN_CENTER,
};
#endif

/** The update progress */
static guiWidget_bar_s widget_progress = {
    .position.x = 8,
    .position.y = 170,
    .width = 160,
    .height = 8,
    .getProgress = getProgressUpdating,
};

#ifdef USE_CHIP_ANIMATION
/** Base image */
static const guiWidget_img_s widget_img_title = {
    .position.x = 8,
    .position.y = 50, // 60+5,
    .img_dsc = &img_chip_progress,
};
/** Image animation */
static guiWidget_imgAnimation_s widget_imgAnim = {
    .position.x = 65,
    .position.y = 50 + 12, // 60 + 17,
    .cur_num = 0,
    .img_num_in_sequence = CHIP_IMG_FILLED_EPISODED,
    .img_dsc_seq = NULL,
};
#else  /* USE_CHIP_ANIMATION */
extern const bitmap_dsc_s img_box_1;
extern const bitmap_dsc_s img_box_2;
extern const bitmap_dsc_s img_box_3;

/** Image animation sequence */
static const bitmap_dsc_s *img_anim_sequence[] = {
    &img_box_1,
#ifndef REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS
#ifdef ENABLE_UPDATING_SCREEN_ANIMATION
    &img_box_2,
    &img_box_3,
#endif /* ENABLE_UPDATING_SCREEN_ANIMATION */
#endif /* REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS */
};
/** Image animation */
static guiWidget_imgAnimation_s widget_imgAnim = {
    .position.x = 56,
    .position.y = 55,
    .cur_num = 0,
    .img_num_in_sequence = sizeof(img_anim_sequence) / sizeof(*img_anim_sequence),
    .img_dsc_seq = img_anim_sequence,
};
#endif /* USE_CHIP_ANIMATION */

/** Last polling for progress */
static int last_progress = 0;

void screenUpdating_switch(void)
{
    setScreenTimeoutMs(5 * 60 * 1000);
    last_progress = 101; /* Clearing progress */
#ifndef REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS
    #ifdef USE_CHIP_ANIMATION
    paint_drawBitMap(widget_img_title.position.x, widget_img_title.position.y,
                     widget_img_title.img_dsc, PAINT_COLOR);
    #else  /* USE_CHIP_ANIMATION */
    #endif /* USE_CHIP_ANIMATION */
#endif
    for (int i = 0; i < sizeof(widgetList_label) / sizeof(*widgetList_label); i++) {
        paint_drawStringEn(widgetList_label[i].position.x, widgetList_label[i].position.y,
                           widgetList_label[i].text, widgetList_label[i].font,
                           widgetList_label[i].text_align, PAINT_COLOR);
    }
    drawBatteryLevel(true, false);
}

bool screenUpdating_poll(void)
{
    bool update_required = false;

    int progress = bootloader_getUpdateProgress();
    if (progress != last_progress) {
        last_progress = progress;
        update_required = true;
        refreshScreenTime();
    } else if (last_progress >= 100) {
        bootApp_sendEvent(BOOT_APP_EVENT_SCREEN_DRAWN);
    } else if (last_progress < 0) {
        bootApp_sendEvent(BOOT_APP_EVENT_SCREEN_DRAWN);
    }

    update_required |= drawBatteryLevel(false, false);

    if (update_required) {
        if (last_progress >= 0) {
#ifdef SHOW_PERCENT_LABLE
            paint_clearWindow(label_updatingProgress.position.x, label_updatingProgress.position.y,
                              160, 20);
            paint_drawStringEn(label_updatingProgress.position.x, label_updatingProgress.position.y,
                               getProgressLevelString(), label_updatingProgress.font,
                               label_updatingProgress.text_align, PAINT_COLOR);
#endif /* SHOW_PERCENT_LABLE */

#ifdef USE_CHIP_ANIMATION
            widget_imgAnim.cur_num =
                (uint8_t)((float)(last_progress * (widget_imgAnim.img_num_in_sequence - 1)) /
                          100.F); /* Episode in range from 0 to 7 */
            paint_drawChip(widget_imgAnim.position, widget_imgAnim.cur_num);
            /* Drawing progress only for updating the main-firmware */
#else  /* USE_CHIP_ANIMATION */

#if ENABLE_UPDATING_SCREEN_ANIMATION
            paint_drawBitMap(widget_imgAnim.position.x, widget_imgAnim.position.y,
                             widget_imgAnim.img_dsc_seq[widget_imgAnim.cur_num],
                             PAINT_BACKGROUND | PAINT_COLOR);
            widget_imgAnim.cur_num = (widget_imgAnim.cur_num + 1 <
                                      widget_imgAnim.img_num_in_sequence)
                                         ? widget_imgAnim.cur_num + 1
                                         : 0;

#else
            /* Starting update via bootstrapper, no animation possible. */
            paint_drawBitMap(widget_imgAnim.position.x, widget_imgAnim.position.y,
                widget_imgAnim.img_dsc_seq[0], PAINT_BACKGROUND | PAINT_COLOR);
#endif


#endif /* USE_CHIP_ANIMATION */
            paint_drawProgressBar(&widget_progress, widget_progress.getProgress());
        } else {
#ifdef USE_CHIP_ANIMATION
            paint_drawChip(widget_imgAnim.position, widget_imgAnim.img_num_in_sequence - 1);
#else  /* USE_CHIP_ANIMATION */
            /* Starting update via bootstrapper, no animation possible. */
            paint_drawBitMap(widget_imgAnim.position.x, widget_imgAnim.position.y,
                             widget_imgAnim.img_dsc_seq[0], PAINT_BACKGROUND | PAINT_COLOR);
#endif /* USE_CHIP_ANIMATION */
        }
    }

    return update_required;
}

/**
 * @brief Get the Updating Progress
 * @return int Value in range from 0 to 100.
 */
static int getProgressUpdating(void)
{
    return (last_progress > 0) && (last_progress <= 100)? last_progress : 0;
}

/**
 * @brief Get the progress Level string.
 * @return char* Pointer to text to display.
 */
static char *getProgressLevelString(void)
{
    static char progress_line[sizeof("100%%")];
    snprintf(progress_line, sizeof(progress_line), "%3.1d%%", getProgressUpdating() > 100 ? 100 : getProgressUpdating());
    return progress_line;
}