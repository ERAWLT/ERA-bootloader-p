/**
 * @file task_gui.h
 * @brief Task for GUI.
 */
#ifndef _TASK_GUI_
#define _TASK_GUI_

#include <stdint.h>

/** Maximum time in milliseconds to draw one screen */
#define TASK_GUI_MAX_SCREEN_DRAWING_TIME_MS (2000)

/** Events in the graphical user interface */
typedef enum {
    TASK_GUI_EMPTY_EVENT,             /* Nothing happened */
    TASK_GUI_WAIT_USER_EVENT,         /* General. Boot failed. */
    TASK_GUI_INFO,                    /* General. Show info screen. */
    TASK_GUI_SHIPPING,                /* Show shipping screen. */
    TASK_GUI_DOWNLOAD_EVENT,          /* Download has started. */
    TASK_GUI_UPDATE_STARTED_EVENT,    /* Bootloader has started an update process. */
    TASK_GUI_UPDATE_PROCESS,          /* New stage during update. */
    TASK_GUI_LOW_BATTERY_EVENT,       /* Show low battery screen. */
    TASK_GUI_UPDATE_ERROR,            /* Failed to check and update. Show error screen. */
    TASK_GUI_START_MF_OK_EVENT,       /* Update completed successfully event.*/
    TASK_GUI_FORCE_DOWNLOAD_SWITCHER, /* Wait until user start force download*/
    TASK_GUI_MORE_INFO,               /* Additional info page */
} guiTask_event_e;

/**
 * @brief Create an GUI task.
 * @return None
 */
void guiTask_create(void);

/**
 * @brief Send event to GUI task.
 * @param event @ref guiTask_event_e.
 */
void taskGui_sendEvent(guiTask_event_e event);

#endif /*_TASK_GUI_*/
