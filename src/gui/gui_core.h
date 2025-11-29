/**
 * @file gui_core.h
 * @brief GUI core header
 */
#ifndef _GUI_CORE_H_
#define _GUI_CORE_H_

#include "gui_widgets.h"
#include "screens.h"

/** Touch states */
typedef enum {
    TOUCH_STATE_NONE,     /*!< Nothing happened */
    TOUCH_STATE_PRESSED,  /*!< Pressed */
    TOUCH_STATE_RELEASED, /*!< Released */
} touch_state_e;

/** Touch event type */
typedef struct {
    coordinate_s position;     /*!< Current touch position */
    touch_state_e touch_state; /*!< @ref touch_state_e */
} guiCore_touchEvent_s;

/**
 * @brief Apply new touch event to gui.
 * @param new_touch Pointer to new touch event, will be copied to gui variable.
 * @return None
 */
void guiCore_applyTouch(guiCore_touchEvent_s *new_touch);

/**
 * @brief Check whether the button is currently under touch event. See @ref guiCore_touchEvent_s.
 * @param btn Button to check.
 * @return bool true if there is a new touch on the button, false otherwise
 */
bool guiCore_processButton(guiWidget_button_s *btn);

/**
 * @brief Allows to switch current screen on display.
 * @param screen_id Screen identificator @ref screen_id_e.
 * @return None
 */
void guiCore_switchScreen(screen_id_e screen_id);

/**
 * @brief Allows to draw actual data on current screen. Also sends the framebuffer to epaper to
 * display. Blocked until epaper is ready.
 * @return None
 */
void guiCore_poll(void);

/**
 * @brief Allows to check the waiting time of the last screen to avoid freezing.
 * @return None
 */
void guiCore_checkTimeout(void);

#endif /* _GUI_CORE_H_ */
