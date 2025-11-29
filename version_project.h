/**
 * @file version_project.h
 * @brief Version of all project modules. Store in flash.
 */

#ifndef _VERSION_PROJECT_H_
#define _VERSION_PROJECT_H_

#include "hwlt_version.h"
#include "version_hwlt_version.h"
#include "version_ramMbox.h"
#include "version_framework.h"

#ifdef __cplusplus
extern "C" {
#endif

/** name of project version */
#define VERSION_PROJECT_NAME         "BOOTLOADER"

/** Major project base */
#define VERSION_PROJECT_BASE        "0.9."
/** Buildnum of project version. Updates automatically, don't change it */
#define VERSION_PROJECT_BUILDNUM     "74"


/** Count of submodules exclude project version */
#define RECORDS_COUNT (4U)

/** Global structure describing the version. Locates in FLASH in section "hwlt_versions" in .ld  */
hwlt_version_s version_project  __attribute__((section(".hwlt_versions")))  =  {
    .cnt     = RECORDS_COUNT,
    .cnt_xor = (uint8_t)~RECORDS_COUNT,

    .records = {
        { VERSION_PROJECT_NAME,      VERSION_PROJECT_BASE  VERSION_PROJECT_BUILDNUM },
        { VERSION_FRAMEWORK_NAME,    VERSION_FRAMEWORK_BUILDNUM    },
        { VERSION_RAM_MBOX_NAME,     VERSION_RAM_MBOX_BUILDNUM     },
        { VERSION_HWLT_VERSION_NAME, VERSION_HWLT_VERSION_BUILDNUM },
    }
};

#ifdef __cplusplus
}
#endif

#endif /* _VERSION_PROJECT_H_ */
