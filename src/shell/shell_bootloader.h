/**
 * @file shell_bootloader.h
 * @brief 
 */

#ifndef _SHELL_BOOTLOADER_H_
#define _SHELL_BOOTLOADER_H_

#include "shell_cmd_list.h"
#include "shell.h"
#include "project_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef SHELL_ENABLED
    #define REGISTER_SHELL_FOR_BOOTLOADER shellBootloader_init()
#else
    #define REGISTER_SHELL_FOR_BOOTLOADER
#endif

/**
 * @brief Register shell commands for bootloader
 * @return none
 */
void shellBootloader_init(void);

#ifdef __cplusplus
}
#endif

#endif /* _SHELL_BOOTLOADER_H_ */