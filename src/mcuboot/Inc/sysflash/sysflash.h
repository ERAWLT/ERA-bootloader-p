/**
 * @file sysflash.h
 * @brief System flash header. Flash area(s), mapping macros and image indices are declared here.
 */

#ifndef _SYSFLASH_H_
#define _SYSFLASH_H_

#include <mcuboot_config/mcuboot_config.h>

/** An arbitrarily high slot ID we will use to indicate that
 *  there is not slot
 */
#define FLASH_SLOT_DOES_NOT_EXIST 255

/** A user-defined identifier for different storage mediums
 *  (i.e internal flash, external NOR flash, eMMC, etc)
 */
typedef enum {
    FLASH_DEVICE_INTERNAL_FLASH = 0, /*!< Index of internal flash memory in the device */
    FLASH_DEVICE_EXTERNAL_FLASH = 1, /*!< Index of external flash memory in the device */
    FLASH_DEVICES_NUM                /*!< Number of available flash memory chips in the device */
} flash_device_id_e;

/** A user-defined identifier for different memory areas,
 *  which was allocated in one from flash devices (@ref flash_device_id_e).
 */
typedef enum {
    FLASH_AREA_IMAGE_MAIN_APP_PRIMARY_ID, /*!< Flash area containing main-firmware */
    FLASH_AREA_IMAGE_SECONDARY_ID,        /*!< Flash area containing update for BL or MF */
#ifdef MCUBOOT_SWAP_USING_SCRATCH
    FLASH_AREA_IMAGE_SCRATCH_ID, /*!< Temporary flash area that is used via mcuboot  \
                                     during update MF */
#endif
    FLASH_AREA_IMAGE_SELF_ID,        /*!< Flash area containing current bootloader */
    FLASH_AREAS_NUM /*!< Number of available flash areas for mcuboot in the device */
} flash_area_id_e;

/** A user-defined identifier for different images that can be booted via mcuboot. */
typedef enum {
    IMG_ID_MAIN_APP = 0, /*!< Main firmware image id that can be booted via mcuboot. */
    NUM_IMG_ID           /*!< Number of images available for mcuboot. */
} mcuboot_img_id_e;

#if (MCUBOOT_IMAGE_NUMBER == 1)
    /** Primary slot for main firmware, nothing else can be stored here. */
    #define FLASH_AREA_IMAGE_PRIMARY(x)    (((x) == IMG_ID_MAIN_APP) ?                  \
                                            FLASH_AREA_IMAGE_MAIN_APP_PRIMARY_ID :      \
                                            FLASH_SLOT_DOES_NOT_EXIST)
    /** Note that update for bootloader and main firmware arrived in the same secondary slot now.
     *  But update for BL will be sent to BS without any involvement from mcuboot.
     */
    #define FLASH_AREA_IMAGE_SECONDARY(x)  (((x) == IMG_ID_MAIN_APP) ?                  \
                                            FLASH_AREA_IMAGE_SECONDARY_ID :             \
                                            FLASH_SLOT_DOES_NOT_EXIST)
#else
    #error "MCUBOOT_IMAGE_NUMBER is currently not supported."
#endif

#ifdef MCUBOOT_SWAP_USING_SCRATCH
    #define FLASH_AREA_IMAGE_SCRATCH        FLASH_AREA_IMAGE_SCRATCH_ID                                
#endif


#endif /* _SYSFLASH_H_ */