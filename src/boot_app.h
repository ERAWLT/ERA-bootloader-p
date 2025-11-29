/**
 * @file boot_app.h
 * @brief Main header for hwlt-mcuboot.
 */

#ifndef _BOOT_APP_H_
#define _BOOT_APP_H_

#include <stdbool.h>
#include <stdint.h>

/** Boot event type */
typedef enum {
    BOOT_APP_EVENT_NONE,              /*!< Nothing happened. */
    BOOT_APP_EVENT_SHOW_ERROR,        /*!< Error occurred during the previous update. */
    BOOT_APP_EVENT_BOOT_TO_MF,        /*!< Let's try to boot to the main firmware. */
    BOOT_APP_EVENT_START_AGAIN,       /*!< Let's try to continue updating. Reset boot
                                            counter to zero in this case. */
    BOOT_APP_EVENT_UPDATE_DOWNLOADED, /*!< Update has been downloaded. Let's update. */
    BOOT_APP_EVENT_SCREEN_DRAWN,      /*!< Just notification that a screen has been drawn. */
    BOOT_APP_EVENT_GO_BL_CLICKED,     /*!< the "GO to BL" button has been clicked. */
    BOOT_APP_EVENT_LATER_CLICKED,     /*!< the "Later" button has been clicked. */
} bootApp_event_e;

/**
 * @brief Sends an event to bootloader
 * @param main_event @ref bootApp_event_e
 */
void bootApp_sendEvent(bootApp_event_e main_event);

/**
 * @brief Wait for event number in bootloader queue.
 * @param event_num Event @ref bootApp_event_e.
 * @param timeout_ms Event wait time.
 * @return true If event exist in bootloader queue.
 * @return false If timeout has expired and event still doesn't exist in bootloader queue.
 * @note Event peek without removing from queue.
 */
bool bootApp_peekForEvent(bootApp_event_e event_num, uint32_t timeout_ms);

/**
 * @brief The "GO to BL" button has been clicked.
 * @return true if clicked and we are ready to jump to MF.
 * @return false if there is no valid main firmware.
 */
bool bootApp_isForceDownloadClicked(void);

#endif /* _BOOT_APP_H_ */
