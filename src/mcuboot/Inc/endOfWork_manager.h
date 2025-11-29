/**
 * @file endOfWork_manager.h
 * @brief This header contains end of work functions for the bootloader.
 */

#ifndef _END_OF_WORK_MANAGER_
#define _END_OF_WORK_MANAGER_

#include "stdbool.h"
#include "stdint.h"

/**
 * @brief Turning off the device via the smart button.
 * @return Should never return
 */
void endOfWork_powerOff(void);

/**
 * @brief Going to shipping mode.
 * @return bool True for a successful request, False otherwise.
 */
bool endOfWork_shippingMode(void);

/**
 * @brief Going to standby mode.
 * @return Should never return
 */
void endOfWork_goSleep(void);

/**
 * @brief Reboot the MCU.
 * @return Should never return
 */
void endOfWork_reboot(void);

/**
 * @brief Jump to main-application or bootstrapper.
 * @param[in] address: start address to jump
 * @return Never return.
 */
void endOfWork_jumpTo(uint32_t address);

#endif /* _END_OF_WORK_MANAGER_ */
