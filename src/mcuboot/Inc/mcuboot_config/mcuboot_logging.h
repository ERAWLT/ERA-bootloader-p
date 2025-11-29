/*
 * Copyright (c) 2018 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MCUBOOT_LOGGING_H__
#define __MCUBOOT_LOGGING_H__

#include <mcuboot_config/mcuboot_config.h>

#include "bootutil/ignore.h"
#include "elog.h"
#define LOG_MODULE_NAME     "Mcuboot"

#define MCUBOOT_LOG_LEVEL_OFF      0
#define MCUBOOT_LOG_LEVEL_ERROR    1
#define MCUBOOT_LOG_LEVEL_WARNING  2
#define MCUBOOT_LOG_LEVEL_INFO     3
#define MCUBOOT_LOG_LEVEL_DEBUG    4

/*
* The compiled log level determines the maximum level that can be
* printed.
*/
#ifndef MCUBOOT_LOG_LEVEL
    #ifdef MCUBOOT_HAVE_LOGGING
            #define MCUBOOT_LOG_LEVEL MCUBOOT_LOG_LEVEL_DEBUG
    #else
            #define MCUBOOT_LOG_LEVEL MCUBOOT_LOG_LEVEL_OFF
    #endif /* MCUBOOT_HAVE_LOGGING */
#endif

#define MCUBOOT_LOG_MODULE_DECLARE(domain)  /* ignore */
#define MCUBOOT_LOG_MODULE_REGISTER(domain) /* ignore */

#if MCUBOOT_LOG_LEVEL >= MCUBOOT_LOG_LEVEL_ERROR
    #define MCUBOOT_LOG_ERR(...) elog_e(LOG_MODULE_NAME, __VA_ARGS__)
#else
    #define MCUBOOT_LOG_ERR(...) IGNORE(__VA_ARGS__)
#endif

#if MCUBOOT_LOG_LEVEL >= MCUBOOT_LOG_LEVEL_WARNING
    #define MCUBOOT_LOG_WRN(...) elog_w(LOG_MODULE_NAME, __VA_ARGS__)
#else
    #define MCUBOOT_LOG_WRN(...) IGNORE(__VA_ARGS__)
#endif

#if MCUBOOT_LOG_LEVEL >= MCUBOOT_LOG_LEVEL_INFO
    #define MCUBOOT_LOG_INF(...) elog_i(LOG_MODULE_NAME, __VA_ARGS__)
#else
    #define MCUBOOT_LOG_INF(...) IGNORE(__VA_ARGS__)
#endif

#if MCUBOOT_LOG_LEVEL >= MCUBOOT_LOG_LEVEL_DEBUG
    #define MCUBOOT_LOG_DBG(...) elog_d(LOG_MODULE_NAME, __VA_ARGS__)
#else
    #define MCUBOOT_LOG_DBG(...) IGNORE(__VA_ARGS__)
#endif

#endif /* __MCUBOOT_LOGGING_H__ */
