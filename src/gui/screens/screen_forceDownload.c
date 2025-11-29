#include "screens.h"

#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"
#include "gui_core.h"
#include "user_logic.h"
#include "task_gui.h"
#include "boot_app.h"
#include "mail_engine.h"
#include "board.h"

extern const bitmap_dsc_s img_logo;
extern const bitmap_dsc_s img_logo_only;
extern guiWidget_label_s widgetLabel_versionBl;
extern guiWidget_label_s widgetLabel_manufacture;

/** Timeout to tap the secret area of the screen in time. */
#define SCREEN_FORCE_DOWNLOAD_SWITCHER_TIMEOUT_MS (500)

/** Set to 1 to draw logo on force download screen */
#define DRAW_LOGO_ON_FORCE_DOWNLOAD 0

/** Press the secret area for at least 500ms */
#define CATCH_GO_BL_TIME_MS (500)

/** Set to zero to use logo without text. */
#define LOGO_WITH_TEXT 1

#define LOGO_AREA_X   (43 + 8)    /* The horizontal position of the start of the Logo */
#define LOGO_AREA_Y   (72 + 7)    /* The vertical position of the start of the Logo */
#define SECRET_AREA_X LOGO_AREA_X /* Touch horizontal position */
#define SECRET_AREA_Y LOGO_AREA_Y /* Touch vertical position */

static void forcedownloadButtonPressed(void);

/** Buttons on screen */
static guiWidget_button_s widget_go2BlButton = {
    .position.x = SECRET_AREA_X,
    .position.y = SECRET_AREA_Y,
    .width = 73,
    .height = 70,
    .color = 1,
    .text = NULL,
    .pressedCallback = forcedownloadButtonPressed,
    .pressed = false,
};

/** Logo image */
static const guiWidget_img_s widget_img_title = {
    .position.x = LOGO_AREA_X,
    .position.y = LOGO_AREA_Y,
    .img_dsc = LOGO_WITH_TEXT? &img_logo: &img_logo_only,
};

#if (LOGO_WITH_TEXT == 0)
/** Text on screen */
static guiWidget_label_s labelAnimation_header = {
    .text = "ERA",
    .position.x = 10,
    .position.y = 145,
    .font = HWLT_FONT_16_BOLD,
};
#endif /* LOGO_WITH_TEXT == 0 */

static uint32_t catching_time = 0;

void screenForceDownload_switch(void)
{
    setScreenTimeoutMs(SCREEN_FORCE_DOWNLOAD_SWITCHER_TIMEOUT_MS);

#if DRAW_LOGO_ON_FORCE_DOWNLOAD == 1
// #ifndef REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS
    paint_drawBitMap(widget_img_title.position.x, widget_img_title.position.y,
                     widget_img_title.img_dsc, PAINT_COLOR);
// #endif
#if (LOGO_WITH_TEXT == 0)
    paint_drawStringEn(labelAnimation_header.position.x, labelAnimation_header.position.y,
                       labelAnimation_header.text, labelAnimation_header.font, TEXT_ALIGN_CENTER,
                       PAINT_COLOR);
#endif /* LOGO_WITH_TEXT == 0 */
#endif /* DRAW_LOGO_ON_FORCE_DOWNLOAD == 1 */
    guiCore_processButton(&widget_go2BlButton); /*!< First check - whether the button is
                                                       pressed */
}

bool screenForceDownload_poll(void)
{
    bool update_required = false;
    guiCore_processButton(&widget_go2BlButton); /*!< First check - whether the
                                                                      button is pressed */
    if (widget_go2BlButton.pressed && (HAL_GetTick() - catching_time >= CATCH_GO_BL_TIME_MS)) {
        refreshScreenTime();
        bootApp_sendEvent(BOOT_APP_EVENT_GO_BL_CLICKED);
        widget_go2BlButton.pressed = false;
    }

    return update_required;
}

/**
 * @brief Handler when the secret area "go BL" has been released
 * @return None
 */
static void forcedownloadButtonPressed(void)
{
    catching_time = HAL_GetTick();
    refreshScreenTime();
}
