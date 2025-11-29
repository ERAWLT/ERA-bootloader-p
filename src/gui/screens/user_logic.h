/**
 * @file user_logic.h
 * @brief Header for general screen logic.
 */

#ifndef _USER_LOGIC_H_
#define _USER_LOGIC_H_

#include <stdint.h>
#include <stdbool.h>
#include "screens.h"

#define BATTERY_LEVEL_TO_DOWNLOAD 40 /*!< Minimum battery level to start downloading*/

/**
 * @brief Handling after current screen has been drawn.
 * @param scr_num Screen number that has been drawn @ref screen_id_e.
 */
void checkScreenTimeout(screen_id_e scr_num);

/**
 * @brief Get passed time in milliseconds
 * @param start_time_ms Start time in ms to count from.
 */
uint32_t getPassedTimeMs(uint32_t start_time_ms);

/**
 * @brief Set the Screen Timeout Ms
 * @param ms Maximum screen time in milliseconds
 */
void setScreenTimeoutMs(uint32_t ms);

/**
 * @brief Get the Screen Timeout Ms
 * @return uint32_t
 */
uint32_t getScreenTimeoutMs(void);

/**
 * @brief Get the Screen Time Ms
 * @return uint32_t Time of last event on screen in milliseconds
 */
uint32_t getScreenTimeMs(void);

/**
 * @brief Refresh screen time to current tick value
 * @return None
 */
void refreshScreenTime(void);

#endif /* _USER_LOGIC_H_ */
