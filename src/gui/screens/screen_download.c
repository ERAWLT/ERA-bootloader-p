/**
 * @file screen_download.c
 * @brief Objects definition on the download screen.
 */
#include "screens.h"
#include "gui_widgets.h"
#include "fonts.h"
#include "gui_paint.h"

#include "user_logic.h"
#include "task_gui.h" // To send new screen event
#include "boot_app.h"
#include "gui_core.h"
#include "board_partial_download.h"
#include "elog.h"
#include "board_flash.h"

#define LOG_TAG "DL_SCREEN"
#define SHIFT_Y     (20u)
#define PERCENT_STEP_TO_UPDATE_PAGE (10)
#define SHOW_ASSETS_VERSION 0 /*!< Show assets version on download screen or only assets string */

/** Uncomment to show progress label in percent */
// #define SHOW_PERCENT

extern const bitmap_dsc_s img_scan_phone;

extern char* bootloader_getPrimaryVersionOfMF(void);
extern char* bootloader_getSecondaryVersionOfMF(void);
extern bool drawBatteryLevel(bool full_update, bool invert);

/**
 * @brief Enumeration of download modes.
 */
typedef enum {
    DOWNLOAD_INIT,      /*!< Initial state */
    DOWNLOAD_NONE,      /*!< No download in progress */
    DOWNLOAD_ASSETS,    /*!< Downloading assets */
    DOWNLOAD_FIRMWARE   /*!< Downloading firmware */
} download_mode_e;

static char *getProgressLevelString(void);
static int getProgressDownload(void);
static void drawAnimation(download_mode_e dwld_type);
static void goBackPressed(void);
static char* getAssetsLabelText(void);
static int progressToPercent(uint32_t downloaded, uint32_t total, int last_prog);
static download_mode_e updateCurrentDownload(int *progress, bool *is_switched);

static size_t last_fw_downloaded = 0;     /*!< Last firmware downloaded size */
static size_t last_fw_total = 0;          /*!< Last firmware total size */
static size_t last_assets_downloaded = 0; /*!< Last assets downloaded size */
static size_t last_assets_total = 0;      /*!< Last assets total size */

/** Text on screen */
static const guiWidget_label_s widgetList_label[] = {
    {
     .text = "NFC",
     .position.x = 9,
     .position.y = 5,
     .font = HWLT_FONT_8,
     .text_align = TEXT_ALIGN_LEFT,
     },
     {
     .text = "DOWNLOAD",
     .position.x = 25,
     .position.y = 28,
     .font = HWLT_FONT_8_BOLD,
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

static guiWidget_label_s label_downloadProgress = {
    .position.x = 40,
    .position.y = 20 + 5, // 157,
    .font = HWLT_FONT_16_BOLD,
    .text = NULL,
    .text_align = TEXT_ALIGN_CENTER,
};

/** Version of MF in primary slot */
static guiWidget_labelAnimation_s label_primaryVersionOfMF = {
    .getLabelText = bootloader_getPrimaryVersionOfMF,
    .position.x = 11,
    .position.y = 58 + 10 + 10, // 157 + 42,
    .font = HWLT_FONT_8_BOLD,
};

/** Version of MF in secondary slot */
static guiWidget_labelAnimation_s label_secondaryVersionOfMF = {
    .getLabelText = bootloader_getSecondaryVersionOfMF,
    .position.x = 8,
    .position.y = 74,
    .font = HWLT_FONT_8_BOLD,
};

/** Version of assets in flash area */
static guiWidget_labelAnimation_s label_VersionOfAssetsMF = {
    .getLabelText = getAssetsLabelText,
    .position.x = 8,
    .position.y = 74,
    .font = HWLT_FONT_8_BOLD,
};

/** Download progress */
static guiWidget_bar_s widget_progress = {
    .position.x = 8,
    .position.y = 52,
    .width = 160,
    .height = 8,
    .getProgress = getProgressDownload,
};

/** Base image */
static const guiWidget_img_s widget_img_title = {
    .position.x = 47,
    .position.y = 105,
    .img_dsc = &img_scan_phone,
};

/** Last polling for progress */
static int last_progress = 0;

void screenDownload_switch(void)
{
    setScreenTimeoutMs(5 * 60 * 1000);
    last_progress = 0; /* Clearing progress */
#ifndef REDUCE_FLASH_USAGE_BY_REMOVING_ANIMATIONS
    paint_drawBitMap(widget_img_title.position.x, widget_img_title.position.y,
                     widget_img_title.img_dsc, PAINT_COLOR);
#endif
    for (int i = 0; i < sizeof(widgetList_label) / sizeof(*widgetList_label); i++) {
        paint_drawStringEn(widgetList_label[i].position.x, widgetList_label[i].position.y,
                           widgetList_label[i].text, widgetList_label[i].font,
                           widgetList_label[i].text_align, PAINT_COLOR);
    }
    // paint_drawStringEn(label_primaryVersionOfMF.position.x, label_primaryVersionOfMF.position.y,
    //                    label_primaryVersionOfMF.getLabelText(), label_primaryVersionOfMF.font, TEXT_ALIGN_CENTER,
    //                    PAINT_COLOR);
    drawBatteryLevel(true, false);
    /* by default show MF label */
    drawAnimation(DOWNLOAD_FIRMWARE);
}

bool screenDownload_poll(void)
{
    bool update_required = false;
    int progress = 0;
    bool is_mode_switched = false;
    download_mode_e dwld_type = updateCurrentDownload(&progress, &is_mode_switched);

    if ((progress != last_progress) || is_mode_switched)
    {
        log_i("update progress: %d%%, type=%d", progress, dwld_type);
        last_progress = progress;
        update_required = true;
        refreshScreenTime();
        if (last_progress >= 100) {
            last_progress = 100;
            if (dwld_type == DOWNLOAD_FIRMWARE) {
                /* TODO: support complete update cmd from mobile protocol  */
                bootApp_sendEvent(BOOT_APP_EVENT_UPDATE_DOWNLOADED);
            } else if (dwld_type == DOWNLOAD_ASSETS) {
                /* TODO: force save to flash */
            }
        }
    }
    update_required |= guiCore_processButton(&button_goBack);
    update_required |= drawBatteryLevel(false, false);

    if (update_required) {
        drawAnimation(dwld_type);
    }
    return update_required;
}

static void drawAnimation(download_mode_e dwld_type)
{
    guiWidget_labelAnimation_s *progress_label;
    /* By default progress label is MF */
    progress_label = (dwld_type == DOWNLOAD_ASSETS) ? &label_VersionOfAssetsMF : &label_secondaryVersionOfMF;


#ifdef SHOW_PERCENT
    paint_clearWindow(label_downloadProgress.position.x, label_downloadProgress.position.y,
                      100, 22);
    paint_drawStringEn(label_downloadProgress.position.x, label_downloadProgress.position.y,
                       getProgressLevelString(), label_downloadProgress.font, 
                       label_downloadProgress.text_align,
                       PAINT_COLOR);

#endif /* SHOW_PERCENT */
    paint_clearWindow(progress_label->position.x, progress_label->position.y,
                      160, 22);
    paint_drawStringEn(progress_label->position.x, progress_label->position.y,
                       progress_label->getLabelText(), progress_label->font, TEXT_ALIGN_CENTER,
                       PAINT_COLOR);

    // paint_drawChip(widget_imgAnim.position, widget_imgAnim.cur_num);
    // widget_imgAnim.cur_num = (widget_imgAnim.cur_num + 1 < widget_imgAnim.img_num_in_sequence)
    //                              ? widget_imgAnim.cur_num + 1
    //                              : 0;

    paint_drawProgressBar(&widget_progress, widget_progress.getProgress());
}

/**
 * @brief Get the Download Progress
 * @return int Value in range from 0 to 100.
 */
static int getProgressDownload(void)
{
    return (last_progress > 0) && (last_progress <= 100) ? last_progress : 0;
}

/**
 * @brief Get the progress Level string.
 * @return char* Pointer to text to display.
 */
static char *getProgressLevelString(void)
{
    static char progress_line[sizeof("100%%")];
    snprintf(progress_line, sizeof(progress_line), "%3.1d%%", getProgressDownload() > 100 ? 100 : getProgressDownload());
    return progress_line;
}

/**
 * @brief Handler when the back button has been pressed.
 * @return None
 */
static void goBackPressed(void)
{
    taskGui_sendEvent(TASK_GUI_INFO);
}

/** 
 * @brief Update and filter of steps progress percentage
 * @param downloaded Number of bytes downloaded.
 * @param total Total number of bytes to download.
 * @param last_prog Last known progress percentage.
 * @return New progress percentage, or last known if no update is needed.
 */
static int progressToPercent(uint32_t downloaded, uint32_t total, int last_prog)
{
    int new_progress = downloaded * 100 / total;
    if (new_progress < last_prog || new_progress >= 100 ||
        new_progress >= last_prog + PERCENT_STEP_TO_UPDATE_PAGE) {
        return new_progress >= 100 ? 100 : new_progress;
    } else {
        return last_prog;
    }
}

/**
 * @brief Get fresh info what is currently downloading.
 * 
 * @param[out] progress Pointer to store the current download progress percentage.
 * @param[out] is_switched Pointer to store if the download type has switched.
 * @return The current download mode as an enum value.
 */
static download_mode_e updateCurrentDownload(int *progress, bool *is_switched)
{
    static download_mode_e current_download = DOWNLOAD_INIT;
    size_t fw_total_size, fw_downloaded_size;
    size_t assets_total_size, assets_downloaded_size;
    bool is_switched_local = false;

    partialDownload_getProgress(&fw_total_size, &fw_downloaded_size,
                                PARTIAL_DOWNLOAD_MODE_FIRMWARE);
    partialDownload_getProgress(&assets_total_size, &assets_downloaded_size,
                                PARTIAL_DOWNLOAD_MODE_ASSETS);
    /* Initialize the download state */
    if (current_download == DOWNLOAD_INIT) {
        last_assets_downloaded = assets_downloaded_size;
        last_assets_total = assets_total_size;
        last_fw_downloaded = fw_downloaded_size;
        last_fw_total = fw_total_size;
        current_download = DOWNLOAD_NONE;
    }

    if (last_fw_downloaded != fw_downloaded_size || last_fw_total != fw_total_size) {
        if (current_download != DOWNLOAD_FIRMWARE) {
            current_download = DOWNLOAD_FIRMWARE;
            is_switched_local = true;
            log_i("switching to firmware download: %u/%u", fw_downloaded_size, fw_total_size);
        }
        last_fw_total = fw_total_size;
        last_fw_downloaded = fw_downloaded_size;
        log_d("last firmware: %u/%u", last_fw_downloaded, last_fw_total);
    }

    if (last_assets_downloaded != assets_downloaded_size || last_assets_total != assets_total_size) {
        if (current_download != DOWNLOAD_ASSETS) {
            current_download = DOWNLOAD_ASSETS;
            is_switched_local = true;
            log_i("switching to assets download: %u/%u", assets_downloaded_size, assets_total_size);
        }
        last_assets_downloaded = assets_downloaded_size;
        last_assets_total = assets_total_size;
        log_d("last assets: %u/%u", last_assets_downloaded, last_assets_total);
    }

    if (progress) {
        if (current_download == DOWNLOAD_FIRMWARE) {
            *progress = progressToPercent(fw_downloaded_size, fw_total_size, last_progress);
        } else if (current_download == DOWNLOAD_ASSETS) {
            *progress = progressToPercent(assets_downloaded_size, assets_total_size, last_progress);
        } else {
            *progress = last_progress; // No download in progress
        }
    }

    if (is_switched) {
        *is_switched = is_switched_local;
    }
    
    return current_download;
}

/**
 * @brief Get the assets label text.
 * 
 * This function retrieves the assets version from external flash memory
 * and formats it into a string for display.
 * 
 * @return char* Pointer to the formatted assets label text.
 */
static char* getAssetsLabelText(void)
{
#if SHOW_ASSETS_VERSION
    /* TODO: Implement assets version retrieval in correct place (framework) */
    uint8_t buffer[24];
    flash_read(buffer, EXTERNAL_FLASH_BASEADDR_ASSETS_UPD, sizeof(buffer));

    uint32_t magic = *((uint32_t *)buffer);
    uint32_t assets_ver = (uint32_t *)(buffer + 4);
    char *assets_ver_mf = (char *)(buffer + 8);

    log_i("magic = 0x%08X, assets_ver = 0x%08X, assets_ver_mf = %.16s", magic, assets_ver,
          assets_ver_mf);

    int len = snprintf(version, sizeof(version), "Release: Assets");
    if (magic == 0x1DB2A40C) {
        snprintf(version + len, sizeof(version) - len, "%.16s", assets_ver_mf);
    } else {
        snprintf(version + len, sizeof(version) - len, "NaN");
    }

    log_i("getAssetsLabelText(): %s", version);
    return version;
#else
    return "Release: Assets (MF)";
#endif
}
