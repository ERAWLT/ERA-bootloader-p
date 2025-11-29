/**
 * @file flash_map_backend.c
 * @brief Flash map backend module.
 *  Allows to use read/write/erase operation for all areas of device's
 *  memory and storage mediums (internal flash, external NOR flash, eMMC, etc)
 */

#include <flash_map_backend/flash_map_backend.h>

#include <string.h>

#include <bootutil/bootutil.h>
#include <mcuboot_config/mcuboot_logging.h>
#include "bootutil_priv.h"

#include "sysflash/sysflash.h"

#include "board_partial_download.h"
#include "board_flash.h"
#include "board_internal_memory.h"
#include "bootloader.h"

#ifndef ARRAY_SIZE
    #define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))
#endif

#ifndef MIN
    #define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
    #define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifndef ALIGN_UP
    #define ALIGN_UP(num, align) (((num) + ((align) - 1)) & ~((align) - 1))
#endif

#ifndef ALIGN_DOWN
    #define ALIGN_DOWN(num, align) ((num) & ~((align) - 1))
#endif

#ifndef ALIGN_OFFSET
    #define ALIGN_OFFSET(num, align) ((num) & ((align) - 1))
#endif

#ifndef IS_ALIGNED
    #define IS_ALIGNED(num, align) (ALIGN_OFFSET((num), (align)) == 0)
#endif

/* Internal flash map below */

#define IMAGE1_SELF_START_ADDRESS INTERNAL_FLASH_GET_BASE(FLASH_SECTOR_1)
#define IMAGE1_SELF_SIZE          (FLASH_SECTOR_SIZE)

#define IMAGE0_PRIMARY_START_ADDRESS INTERNAL_FLASH_GET_BASE(INTERNAL_FLASH_PRIMARY_START_SECTOR)
#define APPLICATION_PRIMARY_SIZE                                                                   \
    (FLASH_SECTOR_SIZE *                                                                           \
     (INTERNAL_FLASH_PRIMARY_END_SECTOR - INTERNAL_FLASH_PRIMARY_START_SECTOR + 1))

/* External flash map below */

#define IMAGE0_SECONDARY_START_ADDRESS EXTERNAL_FLASH_BASEADDR_SECONDARY

/** We must have additional size at last equal WRAPPER_HEADER_SIZE, for public key replace case!
    We will move flash area offset on this size, when wrapper image arrived.
    So WRAPPER_HEADER_SIZE should be equal to external flash sector size. */
#define WRAPPER_HEADER_SIZE (EXTERNAL_FLASH_SECTOR_SIZE)

/** Available flash memory size for update */
#define APPLICATION_SECONDARY_AVAILABLE_SIZE                                                       \
    ((EXTERNAL_FLASH_HIGHADDR_SECONDARY - EXTERNAL_FLASH_BASEADDR_SECONDARY + 1) -                 \
     WRAPPER_HEADER_SIZE)

/* Align available size in secondary flash area to internal flash sector size. */
#define APPLICATION_SECONDARY_SIZE                                                                 \
    (APPLICATION_SECONDARY_AVAILABLE_SIZE -                                                        \
     (APPLICATION_SECONDARY_AVAILABLE_SIZE % FLASH_SECTOR_SIZE))

#define SCRATCH_0_START_ADDRESS EXTERNAL_FLASH_BASEADDR_SCRATCH
#define SCRATCH_SIZE            (EXTERNAL_FLASH_HIGHADDR_SCRATCH - EXTERNAL_FLASH_BASEADDR_SCRATCH + 1)

/** USER CONTROL MACROS FOR FLASH AREA (part below) */

/** Verify writed data */
// #define VALIDATE_PROGRAM_OP

#if (APPLICATION_SECONDARY_SIZE < APPLICATION_PRIMARY_SIZE)
    /** FIT_NUMBER_OF_SECTORS is number of sectors required to fit full image size from secondary
     * slot to primary slot and back. Below calculation is possible only when internal
     * FLASH_SECTOR_SIZE the same for all sector (like for STM32H7).
     */
    #if (defined MCUBOOT_SWAP_USING_MOVE) &&                                                       \
        ((APPLICATION_SECONDARY_SIZE / FLASH_SECTOR_SIZE + 1) <                                    \
         (APPLICATION_PRIMARY_SIZE / FLASH_SECTOR_SIZE))
        #define FIT_NUMBER_OF_SECTORS                                                              \
            ((APPLICATION_SECONDARY_SIZE / FLASH_SECTOR_SIZE) + 1) /* == 10 */
    #else
        #define FIT_NUMBER_OF_SECTORS (APPLICATION_SECONDARY_SIZE / FLASH_SECTOR_SIZE) /* == 9 */
    #endif
    #warning "Bad memory map: SECONDARY less then PRIMARY slot!" /* in release: #error */
#elif (APPLICATION_SECONDARY_SIZE > APPLICATION_PRIMARY_SIZE)
    #define FIT_NUMBER_OF_SECTORS_SECONDARY (APPLICATION_PRIMARY_SIZE / FLASH_SECTOR_SIZE)
    #error "Test required: SECONDARY greater then PRIMARY slot!"
#elif (defined MCUBOOT_SWAP_USING_MOVE)
    #define APPLICATION_SECONDARY_SIZE (FLASH_SECTOR_SIZE * (MCUBOOT_MAX_IMG_SECTORS - 1))
#endif

#if ((defined FIT_NUMBER_OF_SECTORS) &&                                                            \
     ((FLASH_SECTOR_SIZE * FIT_NUMBER_OF_SECTORS) < APPLICATION_PRIMARY_SIZE))
    /* For primary slot only */
    #undef APPLICATION_PRIMARY_SIZE
    #define APPLICATION_PRIMARY_SIZE (FLASH_SECTOR_SIZE * FIT_NUMBER_OF_SECTORS)
    #warning "APPLICATION_PRIMARY_SIZE was fit to selected number of sectors!"
#endif

#if (FLASH_SECTOR_SIZE < EXTERNAL_FLASH_SECTOR_SIZE)
    #error "Test required: Current flash backend logic based on fact \
            that internal sector size greater then external!"
#elif (FLASH_SECTOR_SIZE % EXTERNAL_FLASH_SECTOR_SIZE != 0)
    #error "Sectors size in Flash devices is not compatible!"
#endif

/* This area never changed. */
static const struct flash_area bootloader_self_slot = {
    .fa_id = FLASH_AREA_IMAGE_SELF_ID,
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off = IMAGE1_SELF_START_ADDRESS,
    .fa_size = IMAGE1_SELF_SIZE,
};

/* This area never changed. */
static const struct flash_area primary_main_app = {
    .fa_id = FLASH_AREA_IMAGE_MAIN_APP_PRIMARY_ID,
    .fa_device_id = FLASH_DEVICE_INTERNAL_FLASH,
    .fa_off = IMAGE0_PRIMARY_START_ADDRESS,
    .fa_size = APPLICATION_PRIMARY_SIZE,
};

/*  This area can be changed, in case
    if update arrives with new public key in wrapper image
    if partial download will use storing with memory balancing algorithm. */
static struct flash_area secondary_img0 = {
    .fa_id = FLASH_AREA_IMAGE_SECONDARY_ID,
    .fa_device_id = FLASH_DEVICE_EXTERNAL_FLASH,
    .fa_off = IMAGE0_SECONDARY_START_ADDRESS,
    .fa_size = APPLICATION_SECONDARY_SIZE,
};

#ifdef MCUBOOT_SWAP_USING_SCRATCH
/*  This area can be changeable.  */
static struct flash_area scratch_img0 = {
    .fa_id = FLASH_AREA_IMAGE_SCRATCH_ID,
    .fa_device_id = FLASH_DEVICE_EXTERNAL_FLASH,
    .fa_off = SCRATCH_0_START_ADDRESS,
    .fa_size = SCRATCH_SIZE,
};
#endif /* MCUBOOT_SWAP_USING_SCRATCH */

static struct flash_area *s_flash_areas[FLASH_AREAS_NUM] = {
    (struct flash_area *)&primary_main_app,
    (struct flash_area *)&secondary_img0,
#ifdef MCUBOOT_SWAP_USING_SCRATCH
    (struct flash_area *)&scratch_img0,
#endif /* MCUBOOT_SWAP_USING_SCRATCH */
    (struct flash_area *)&bootloader_self_slot,
};

static const struct flash_area *prv_lookup_flash_area(uint8_t id)
{
    for (size_t i = 0; i < ARRAY_SIZE(s_flash_areas); i++) {
        const struct flash_area *area = (const struct flash_area *)s_flash_areas[i];
        if (id == area->fa_id) {
            return area;
        }
    }
    return NULL;
}

bool flash_area_move_start(const struct flash_area *area, uint32_t shift_in_bytes)
{
    if ((area != &secondary_img0) || (WRAPPER_HEADER_SIZE != shift_in_bytes)) {
        return false;
    }

    struct flash_area *new_secondary = (struct flash_area *)area;
    new_secondary->fa_off += shift_in_bytes;
    /* WRAPPER_HEADER_SIZE bytes were laid down at memory mapping stage, so size don't changed here.
     */

    return true;
}

void flash_area_reset_start(const struct flash_area *area)
{
    if (area != &secondary_img0) {
        return;
    }

    struct flash_area *new_secondary = (struct flash_area *)area;
    new_secondary->fa_off = IMAGE0_SECONDARY_START_ADDRESS;
}

int flash_area_open(uint8_t id, const struct flash_area **area_outp)
{
    int ret = -1;
    MCUBOOT_LOG_DBG("%s: ID=%d", __func__, (int)id);
    const struct flash_area *fa = prv_lookup_flash_area(id);
    if (fa != NULL) {
        *area_outp = fa;
        if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH) {
            ret = 0;
        } else if (fa->fa_device_id == FLASH_DEVICE_EXTERNAL_FLASH) {
            int status_flash = flash_getInitStatus();
            if (status_flash != 0) {
                status_flash = flash_init();
                if (status_flash != 0) {
                    MCUBOOT_LOG_ERR("flash_init Error %d", status_flash);
                }
            } else {
                ret = 0;
            }
        }
    }
    return ret;
}

void flash_area_close(const struct flash_area *area)
{
    /* Nothing to do */
}

int flash_area_read(const struct flash_area *fa, uint32_t off, void *dst, uint32_t len)
{
    if ((off >= fa->fa_size) || ((off + len) > fa->fa_size)) {
        MCUBOOT_LOG_ERR("%s: Out of Bounds (addr:0x%x, len: 0x%x)", __func__, fa->fa_off + off,
                        len);
        return -1;
    }

    if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH) {
        if (!internalMemory_read(dst, fa->fa_off + off, len)) {
            MCUBOOT_LOG_ERR("Integrity check failed!");
            return -1;
        }
    } else if (fa->fa_device_id == FLASH_DEVICE_EXTERNAL_FLASH) {

        /* Status for external flash */
        int status_flash;
        /* Address for read from external flash */
        uint32_t addr_to_read = fa->fa_off + off;
        /* destination data offset */
        uint32_t dst_off = 0;
        /* Only first read offset can be non zero */
        uint32_t read_off = addr_to_read % EXTERNAL_FLASH_PAGE_SIZE;
        /* Only first read_size should be aligned if aleary not. */
        uint32_t read_size = EXTERNAL_FLASH_PAGE_SIZE - read_off;

        while (len) {

            if (read_size > len) {
                read_size = len; /* last operation */
            }
            if ((status_flash = flash_read(((uint8_t *)dst + dst_off), addr_to_read, read_size)) !=
                0) {
                MCUBOOT_LOG_ERR("ext flash read (addr=%7.7X) ERROR %d!", addr_to_read,
                                status_flash);
                return -1;
            }
            dst_off += read_size;
            len -= read_size;
            addr_to_read += read_size;

            read_off = 0;
            read_size = EXTERNAL_FLASH_PAGE_SIZE;
        }
    } else {
        MCUBOOT_LOG_ERR("%s: unexisted area", __func__);
        return -1;
    }

    return 0;
}

int flash_area_write(const struct flash_area *fa, uint32_t off, const void *src, uint32_t len)
{
    if ((off >= fa->fa_size) || ((off + len) > fa->fa_size)) {
        MCUBOOT_LOG_ERR("%s: Out of Bounds (addr:0x%x, len: 0x%x)", __func__, fa->fa_off + off,
                        len);
        return -1;
    }

    if (len <= EXTERNAL_FLASH_PAGE_SIZE) {
        /* Print only update status operation */
        MCUBOOT_LOG_INF("Write: Addr: 0x%08x Length: %d", fa->fa_off + off, (int)len);
    }

    if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH) {

        const uint32_t addr = fa->fa_off + off;
        internalMemory_writeData(addr, (uint8_t *)src, len);
        bootloader_tryUpdateProgress(BOOTLOADER_UPDATE_OPERATION_COPY, len);

#ifdef VALIDATE_PROGRAM_OP
        if (memcmp((void *)addr, src, len) != 0) {
            MCUBOOT_LOG_ERR("%s: Program Failed", __func__);
            assert(0);
        }
#endif

    } else if (fa->fa_device_id == FLASH_DEVICE_EXTERNAL_FLASH) {

        const uint32_t start_addr = fa->fa_off + off;
        /* source data offset */
        uint32_t src_off = 0;
        /* next page to program */
        uint32_t page = start_addr / flash_getPageSize();
        /* for case when 'page_off' != 0? */
        uint32_t page_off = start_addr % flash_getPageSize();
        /* Only first prog_size should be aligned if aleary not. */
        uint32_t prog_size = EXTERNAL_FLASH_PAGE_SIZE - page_off;
        while (len) {
            if (prog_size > len) {
                prog_size = len; /* last operation */
            }
            int status_flash;
            if ((status_flash = flash_write(((uint8_t *)src + src_off), page, page_off,
                                            prog_size)) != 0) {
                MCUBOOT_LOG_ERR("ext flash write (page=%7.7X) ERROR %d!", page, status_flash);
                return -1;
            }
            len -= prog_size;
            src_off += prog_size;
            page += 1;

            page_off = 0;
            prog_size = EXTERNAL_FLASH_PAGE_SIZE;
        }
#ifdef VALIDATE_PROGRAM_OP
        /* Revert back len value */
        len = src_off;
        /* Clean src offset to check writed data from start */
        src_off = 0;
        /* Array to read data */
        uint8_t readed_bytes[EXTERNAL_FLASH_PAGE_SIZE];
        /* Status for external flash */
        int status_flash;
        /* Address for read from external flash */
        uint32_t addr_to_read = start_addr;
        /* Only first read offset can be non zero */
        uint32_t read_off = addr_to_read % EXTERNAL_FLASH_PAGE_SIZE;
        /* Only first read_size should be aligned if aleary not. */
        uint32_t read_size = EXTERNAL_FLASH_PAGE_SIZE - read_off;
        while (len) {

            if (read_size > len) {
                read_size = len; /* last operation */
            }
            if ((status_flash = flash_read(readed_bytes, addr_to_read, read_size)) != 0) {
                MCUBOOT_LOG_ERR("ext flash read (addr=%7.7X) ERROR %d!", addr_to_read,
                                status_flash);
                return -1;
            } else if (memcmp((void *)readed_bytes, (void *)((uint8_t *)src + src_off),
                              read_size) != 0) {
                MCUBOOT_LOG_ERR("%s: Program Failed", __func__);
                return -1;
            }
            src_off += read_size;
            len -= read_size;
            addr_to_read += read_size;

            read_off = 0;
            read_size = EXTERNAL_FLASH_PAGE_SIZE;
        }
#endif

    } else {
        MCUBOOT_LOG_ERR("%s: unexisted area", __func__);
        return -1;
    }

    return 0;
}

int flash_area_erase(const struct flash_area *fa, uint32_t off, uint32_t len)
{
    if ((off > fa->fa_size) || (off + len > fa->fa_size)) {
        MCUBOOT_LOG_ERR("%s: Out of Bounds (addr:0x%x, len: 0x%x)", __func__, fa->fa_off + off,
                        len);
        return -1;
    }

    if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH) {

        if ((len % FLASH_SECTOR_SIZE) != 0 || (off % FLASH_SECTOR_SIZE) != 0) {
            MCUBOOT_LOG_ERR("%s: Not aligned on sector Offset: 0x%x Length: 0x%x", __func__,
                            (int)off, (int)len);
            return -1;
        }

        const uint32_t start_addr = fa->fa_off + off;
        const uint32_t start_sector = INTERNAL_FLASH_GET_SECTOR(start_addr);
        const uint32_t end_sector = INTERNAL_FLASH_GET_SECTOR(start_addr + len);
        MCUBOOT_LOG_INF("%s: Addr: 0x%08x Length: %d", __func__, (int)start_addr, (int)len);

        for (uint32_t si = start_sector; si < end_sector; si += 1) {
            internalMemory_eraseSector(si);
            board_wdRefresh();
            bootloader_tryUpdateProgress(BOOTLOADER_UPDATE_OPERATION_ERASE_PRIMARY,
                                         FLASH_SECTOR_SIZE);
        }

    } else if (fa->fa_device_id == FLASH_DEVICE_EXTERNAL_FLASH) {

        const uint32_t start_addr = fa->fa_off + off;
        const uint32_t start_sector = start_addr / flash_getSectorSize();
        const uint32_t end_sector = (start_addr + len) / flash_getSectorSize();
        MCUBOOT_LOG_INF("%s: Addr: 0x%08x Length: %d", __func__, (int)start_addr, (int)len);
        if (flash_getSectorCount() < end_sector) {
            MCUBOOT_LOG_ERR("%s: Out of Bounds (end_sector:0x%x)", __func__, end_sector);
            return -1;
        }

        int status_flash;
        for (uint32_t si = start_sector; si < end_sector; si += 1) {
            if ((status_flash = flash_erase_sector(si)) != 0) {
                MCUBOOT_LOG_ERR("Erase sector %d - Error %d", si, status_flash);
                return -1;
            }
            board_wdRefresh();
            bootloader_tryUpdateProgress(BOOTLOADER_UPDATE_OPERATION_ERASE_SECONDARY,
                                         flash_getSectorSize());
        }

        /* Clear all progress */
        partialDownload_loadImage(NULL, 0, 0, 0, PARTIAL_DOWNLOAD_MODE_FIRMWARE);

    } else {
        return -1;
    }

    return 0;
}

uint32_t flash_area_align(const struct flash_area *fa)
{
    if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH) {
        return INTERNAL_FLASH_PROGRAM_SIZE_BYTES;
    } else if (fa->fa_device_id == FLASH_DEVICE_EXTERNAL_FLASH) {
        return MIN(INTERNAL_FLASH_PROGRAM_SIZE_BYTES, EXTERNAL_FLASH_PAGE_SIZE);
    } else {
        return -1;
    }
}

uint8_t flash_area_erased_val(const struct flash_area *fa)
{
    return EMPTY_FLASH_BYTE;
}

int flash_area_get_sector(const struct flash_area *fa, uint32_t addr, struct flash_sector *sector)
{
    if (addr >= fa->fa_size) {
        MCUBOOT_LOG_ERR("%s: unexisted address=%lx request", __func__, addr);
        return -1;
    }

    size_t sector_size;
    if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH) {
        sector_size = FLASH_SECTOR_SIZE;
    } else if (fa->fa_device_id == FLASH_DEVICE_EXTERNAL_FLASH) {
#ifdef MCUBOOT_SWAP_USING_MOVE
        sector_size = MAX(FLASH_SECTOR_SIZE, EXTERNAL_FLASH_SECTOR_SIZE);
#else
        sector_size = EXTERNAL_FLASH_SECTOR_SIZE;
#endif
    } else {
        MCUBOOT_LOG_ERR("%s: unexisted area", __func__);
        return -1;
    }

    if (sector != NULL) {
        for (size_t off = 0; off < fa->fa_size; off += sector_size) {
            if (off > addr) {
                break;
            }
            sector->fs_off = off;
            sector->fs_size = sector_size;
        }
        uint32_t delta_bytes_first_sector = (fa->fa_off % sector_size);
        if ((addr < sector_size) && delta_bytes_first_sector) { /* Wrapper image or moved image in
                                                                   external flash */
            MCUBOOT_LOG_INF("current sector: delta_bytes_first_sector=%d",
                            delta_bytes_first_sector);
            sector->fs_size -= delta_bytes_first_sector; /* Only first sector will be moved */
        }
    }
    return 0;
}

int flash_area_get_sectors(int fa_id, uint32_t *count, struct flash_sector *sectors)
{
    const struct flash_area *fa = prv_lookup_flash_area(fa_id);
    size_t sector_size;
    uint32_t total_count = 0;

    if (fa->fa_device_id == FLASH_DEVICE_INTERNAL_FLASH) {
        /** All sectors for the STM32H7 are the same size */
#ifdef MCUBOOT_SWAP_USING_MOVE
        sector_size = MAX(FLASH_SECTOR_SIZE, EXTERNAL_FLASH_SECTOR_SIZE);
#else
        sector_size = FLASH_SECTOR_SIZE;
#endif
    } else if (fa->fa_device_id == FLASH_DEVICE_EXTERNAL_FLASH) {
        /* Sector size for external memory less or equal internal sector size */
#ifdef MCUBOOT_SWAP_USING_MOVE
        sector_size = MAX(FLASH_SECTOR_SIZE, EXTERNAL_FLASH_SECTOR_SIZE);
#else
        sector_size = EXTERNAL_FLASH_SECTOR_SIZE;
#endif
    } else {
        MCUBOOT_LOG_ERR("%s: unexisted area", __func__);
        return -1;
    }

    if (sectors != NULL) {
        for (size_t off = 0; off < fa->fa_size; off += sector_size) {
            /* Note: Offset here is relative to flash area, not device */
            sectors[total_count].fs_off = off;
            sectors[total_count].fs_size = sector_size;
            total_count++;

            if (total_count >= BOOT_MAX_IMG_SECTORS) {
                break;
            }
        }
        uint32_t delta_bytes_first_sector = (fa->fa_off % sector_size);
        if (delta_bytes_first_sector) { /* Wrapper image or moved image in external flash */
            MCUBOOT_LOG_INF("sectors: delta_bytes_first_sector=%d", delta_bytes_first_sector);
            sectors[0].fs_size -= delta_bytes_first_sector; /* Only first sector will be moved */
        }
    }
    if (count != NULL) {
        *count = total_count;
    }

    return 0;
}

int flash_area_id_from_multi_image_slot(int image_index, int slot)
{
    if (image_index < MCUBOOT_IMAGE_NUMBER) {
        switch (slot) {
            case BOOT_PRIMARY_SLOT:
                return FLASH_AREA_IMAGE_PRIMARY(image_index);
                break;
            case BOOT_SECONDARY_SLOT:
                return FLASH_AREA_IMAGE_SECONDARY(image_index);
                break;
        }
    }

    MCUBOOT_LOG_ERR("Unexpected Request: image_index=%d, slot=%d", image_index, slot);
    return -1;
}

int flash_area_id_from_image_slot(int slot)
{
    return flash_area_id_from_multi_image_slot(0, slot);
}
