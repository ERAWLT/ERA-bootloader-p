/*
 * Copyright (c) 2018 Open Source Foundries Limited
 * Copyright (c) 2019-2020 Arm Limited
 * Copyright (c) 2019-2020 Linaro Limited
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __MCUBOOT_CONFIG_H__
#define __MCUBOOT_CONFIG_H__

#include "project_config.h"
#include "board_internal_memory.h"
#include "board_flash.h"
#include "board.h"

/*
 * Signature types
 *
 * You must choose exactly one signature type.
 */

/* Uncomment for RSA signature support */
// #define MCUBOOT_SIGN_RSA
#ifdef MCUBOOT_SIGN_RSA
    #define MCUBOOT_SIGN_RSA_LEN 2048
#endif

/* Uncomment for ECDSA signatures using curve P-256. */
#define MCUBOOT_SIGN_EC256
#if defined(MCUBOOT_SIGN_EC256)
    #if defined(MCUBOOT_USE_LIBSE)
        #define MCUBOOT_HW_KEY
    #else 
        #error "MCUBOOT_USE_LIBSE is required now. Used default mbedtls lib."
    #endif
#endif


/*
 * Upgrade mode
 *
 * The default is to support A/B image swapping with rollback.  Other modes
 * with simpler code path, which only supports overwriting the existing image
 * with the update image or running the newest image directly from its flash
 * partition, are also available.
 *
 * You can enable only one mode at a time from the list below to override
 * the default upgrade mode.
 */
// #define CONFIG_MCUBOOT_SWAP_USING_SCRATCH
#ifdef CONFIG_MCUBOOT_SWAP_USING_MOVE
    #define MCUBOOT_SWAP_USING_MOVE 1
#elif defined CONFIG_MCUBOOT_SWAP_USING_SCRATCH
    #define MCUBOOT_SWAP_USING_SCRATCH 1
#else
    /* Enable the overwrite-only code path. Default now. */
    #define MCUBOOT_OVERWRITE_ONLY
#endif

#define MCUBOOT_BOOT_MAX_ALIGN (INTERNAL_FLASH_PROGRAM_SIZE_BYTES)

/** During the software based downgrade prevention the image version numbers are
 * compared. In this case downgrade prevention is only available when the
 * overwrite-based image update strategy is used (i.e. `MCUBOOT_OVERWRITE_ONLY`
 * is set).
 */
#define MCUBOOT_DOWNGRADE_PREVENTION

#ifdef MCUBOOT_DOWNGRADE_PREVENTION
    /** Used with MCUBOOT_DOWNGRADE_PREVENTION during check_downgrade_prevention.
     *  Enable (set 1) to compare security counter of images.
     *  Disable (set 0) to compare version from image header.
     */
    #define MCUBOOT_DOWNGRADE_PREVENTION_SECURITY_COUNTER 1
#endif

/**
 * It is an optional step of the image validation process. When enabled, the target must
 * provide an implementation of the security counter interface defined in
 * `boot/bootutil/include/security_cnt.h`.
 * 
 * During the hardware based downgrade prevention (alias rollback protection) the
 * new image's security counter will be compared with the currently active security
 * counter value which must be stored in a non-volatile and trusted component of
 * the device. 
 */
// #define MCUBOOT_HW_ROLLBACK_PROT

/* Uncomment to enable the direct-xip code path. */
/* #define MCUBOOT_DIRECT_XIP */

/* Uncomment to enable the ram-load code path. When it commented then ih_load_addr is just a flag
 * and nothing more. */
/* #define MCUBOOT_RAM_LOAD */

#ifdef MCUBOOT_OVERWRITE_ONLY
    /** Uncomment to only erase and overwrite those primary slot sectors needed
     * to install the new image, rather than the entire image slot.
     */
    #define MCUBOOT_OVERWRITE_ONLY_FAST
    /** Uncomment to know that update was completed via mcuboot. */
    #define MCUBOOT_IMAGE_ACCESS_HOOKS
#endif


/*
 * Cryptographic settings
 *
 * You must choose between Mbed TLS and Tinycrypt as source of
 * cryptographic primitives. Other cryptographic settings are also
 * available.
 */

/* Uncomment to use Mbed TLS cryptographic primitives */
#if defined(CONFIG_WALLET_USE_MBEDTLS)
    #define MCUBOOT_USE_MBED_TLS
#else
    /* MCUboot requires the definition of a crypto lib,
    * using Tinycrypt as default */
    #define MCUBOOT_USE_TINYCRYPT
#endif


/**
 * Always check the signature of the image in the primary slot before booting,
 * even if no upgrade was performed. This is recommended if the boot
 * time penalty is acceptable.
 */
#define MCUBOOT_VALIDATE_PRIMARY_SLOT


/*
 * Flash abstraction
 */

/** Uncomment if your flash map API supports flash_area_get_sectors().
 * See the flash APIs for more details. 
 */
#define MCUBOOT_USE_FLASH_AREA_GET_SECTORS

/** Default maximum number of flash sectors per image slot; change
 * as desirable. 
 */
#define MCUBOOT_MAX_IMG_SECTORS ((EXTERNAL_FLASH_HIGHADDR_SECONDARY - EXTERNAL_FLASH_BASEADDR_SECONDARY)/EXTERNAL_FLASH_SECTOR_SIZE + 1) 

/** Default number of separately updateable images; change in case of
 * multiple images.
 */
#ifndef CONFIG_MCUBOOT_IMAGE_NUMBER
    #define MCUBOOT_IMAGE_NUMBER 1
#else
    #define MCUBOOT_IMAGE_NUMBER CONFIG_MCUBOOT_IMAGE_NUMBER
#endif

/*
 * Logging
 */

/*
 * If logging is enabled the following functions must be defined by the
 * platform:
 *
 *    MCUBOOT_LOG_MODULE_REGISTER(domain)
 *      Register a new log module and add the current C file to it.
 *
 *    MCUBOOT_LOG_MODULE_DECLARE(domain)
 *      Add the current C file to an existing log module.
 *
 *    MCUBOOT_LOG_ERR(...)
 *    MCUBOOT_LOG_WRN(...)
 *    MCUBOOT_LOG_INF(...)
 *    MCUBOOT_LOG_DBG(...)
 *
 * The function priority is:
 *
 *    MCUBOOT_LOG_ERR > MCUBOOT_LOG_WRN > MCUBOOT_LOG_INF > MCUBOOT_LOG_DBG
 */
#ifdef CONFIG_DEBUG_UART_ENABLED
    #define MCUBOOT_HAVE_LOGGING 1
#endif

/*
 * Watchdog feeding
 */

/* This macro might be implemented if the OS / HW watchdog is enabled while
 * doing a swap upgrade and the time it takes for a swapping is long enough
 * to cause an unwanted reset. If implementing this, the OS main.c must also
 * enable the watchdog (if required)!
 */
#define MCUBOOT_WATCHDOG_FEED()         \
    do {                                \
        board_wdRefresh();              \
    } while (0)



/* If a OS ports support single thread mode or is bare-metal then:
 * This macro implements call that switches CPU to an idle state, from which
 * the CPU may be woken up by, for example, UART transmission event.
 * 
 * Otherwise this macro should be no-op.
 */
#define MCUBOOT_CPU_IDLE()                 \
    do {                                   \
    } while (0)


#endif /* __MCUBOOT_CONFIG_H__ */
