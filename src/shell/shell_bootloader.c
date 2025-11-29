/**
 * @file shell_bootloader.c
 * @brief Contains additional shell command handlers for bootloader test only.
 */

#include "shell_bootloader.h"
#include "board_partial_download.h"
#include "board_internal_memory.h"
#include "parser.h"
#include <bootutil/image.h>
#include "flash_map_backend/flash_map_backend.h"
#include "sysflash/sysflash.h"
#include "libse.h"

#include <stdio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h> /* strtol */

/** Console print for test functions */
#define LOG_SHELL(...)                                                                             \
    do {                                                                                           \
        printf("\t%s: ", __func__);                                                                \
        printf(__VA_ARGS__);                                                                       \
        printf("\n");                                                                            \
    } while (0)

/** Show for user which field was corrupted */
#define GET_CORRUPT_ID_STRING(id)                                                                  \
    (id == CORRUPT_IH_MAGIC              ? "ih_magic"                                              \
     : id == CORRUPT_IH_LOAD_ADDR        ? "ih_load_addr"                                          \
     : id == CORRUPT_IH_HDR_SIZE         ? "ih_hdr_size"                                           \
     : id == CORRUPT_IH_PROTECT_TLV_SIZE ? "ih_protect_tlv_size"                                   \
     : id == CORRUPT_IH_IMG_SIZE         ? "ih_img_size"                                           \
     : id == CORRUPT_IH_FLAGS            ? "ih_flags"                                              \
     : id == CORRUPT_IH_VER              ? "ih_ver"                                                \
     : id == CORRUPT_IMAGE               ? "image (sha256)"                                        \
     : id == CORRUPT_TRAILERINFOPROT     ? "trailer info prot"                                     \
     : id == CORRUPT_TRAILERINFO         ? "trailer info"                                          \
     : id == CORRUPT_SIGNATURE           ? "signature"                                             \
                                         : "image (sha256)")

#define PD_LOAD_LEN                                                                                \
    4096 /*!< Call load function with this lenght if possible. We can choose any length. */
#define CORRUPT_WHOLE_WORD 1    /*!< Corrupt all 32 bytes in flash memory word */

/** Index for corrupt fields */
typedef enum {
    CORRUPT_IH_MAGIC,
    CORRUPT_IH_LOAD_ADDR,
    CORRUPT_IH_HDR_SIZE,
    CORRUPT_IH_PROTECT_TLV_SIZE,
    CORRUPT_IH_IMG_SIZE,
    CORRUPT_IH_FLAGS,
    CORRUPT_IH_VER,
    CORRUPT_IMAGE,
    CORRUPT_TRAILERINFOPROT,
    CORRUPT_TRAILERINFO,
    CORRUPT_SIGNATURE,
    CORRUPT_NUM_FIELDS,
} corrupt_field_id_e;

/** Mcuboot image header structure type */
typedef struct image_header image_header_s;

static void corruptImage(int argc, char **argv);
static void cmdDownloadFromFlash(int argc, char **argv);
static void resetPublic(int argc, char **argv);

static uint32_t getImageSize(uint32_t addr);
static uint32_t getHeaderSize(uint32_t addr);
static uint32_t getImageSignedSize(uint32_t addr);
static uint32_t getImageFullSize(uint32_t addr);

void shellBootloader_init(void)
{
    shell_registerCommand("corruptMf", corruptImage);
    shell_registerCommand("pdLoad", cmdDownloadFromFlash);
    // shell_registerCommand("resetPub", resetPublic);
}

/**
 * @brief Handler of command "corruptMf"
 * @param args Pointer on arguments
 * @param arg_num Number of arguments
 * @return None
 */
static void corruptImage(int argc, char **argv)
{
    uint32_t manual_offset = 0;
    corrupt_field_id_e corrupt_id = CORRUPT_IMAGE;

    if (argc > 1) {
        corrupt_id = strtol((const char *)argv[1], NULL, 10);
    }
    if (argc > 2) {
        manual_offset = strtol((const char *)argv[2], NULL, 10);
    }

    uint32_t base_addr = INTERNAL_FLASH_GET_BASE(INTERNAL_FLASH_PRIMARY_START_SECTOR);
    uint32_t offset = 0;
    switch (corrupt_id) {
        case CORRUPT_IH_MAGIC:
            offset = offsetof(struct image_header, ih_magic);
            break;
        case CORRUPT_IH_LOAD_ADDR:
            offset = offsetof(struct image_header, ih_load_addr);
            break;
        case CORRUPT_IH_HDR_SIZE:
            offset = offsetof(struct image_header, ih_hdr_size);
            break;
        case CORRUPT_IH_PROTECT_TLV_SIZE:
            offset = offsetof(struct image_header, ih_protect_tlv_size);
            break;
        case CORRUPT_IH_IMG_SIZE:
            offset = offsetof(struct image_header, ih_img_size);
            break;
        case CORRUPT_IH_FLAGS:
            offset = offsetof(struct image_header, ih_flags);
            break;
        case CORRUPT_IH_VER:
            offset = offsetof(struct image_header, ih_ver);
            break;
        case CORRUPT_TRAILERINFOPROT:
            /* corrupt for tlv_tot length */
            offset = getHeaderSize(base_addr) + getImageSize(base_addr) + 2;
            break;
        case CORRUPT_TRAILERINFO:
            /* corrupt for tlv_tot protected length */
            offset = getImageSignedSize(base_addr) + 2;
            break;
        case CORRUPT_SIGNATURE:
            offset = getImageSignedSize(base_addr) + sizeof(struct image_tlv_info) +
                     sizeof(struct image_tlv) + 40;
            break;

        case CORRUPT_IMAGE:
        default:
            offset = getHeaderSize(base_addr) - 40;
            break;
    }
    offset += manual_offset;
    base_addr += offset;
    if (!internalMemory_verifyIntegrity(base_addr, 1)) {
        LOG_SHELL("Already corrupted!");
        return;
    }
#if (CORRUPT_WHOLE_WORD == 0)
    if (*((uint8_t *)(base_addr)) == 0) {
        LOG_SHELL("Nothing to currupt");
        return;
    }
#endif

    static uint8_t data[INTERNAL_FLASH_PROGRAM_SIZE_BYTES] = {0};
    uint32_t aligned_addr = base_addr & ~(INTERNAL_FLASH_PROGRAM_SIZE_BYTES - 1);
    for (int i = 0; i < INTERNAL_FLASH_PROGRAM_SIZE_BYTES; i++) {
        if (i == (base_addr - aligned_addr)) {
            data[i] = 0; // corrupting
        } else {
            data[i] = CORRUPT_WHOLE_WORD? 0x00: 0xFF;
        }
    }

    if (internalMemory_writeData(aligned_addr, (uint8_t *)data, sizeof(data))) {
        if (*((uint8_t *)(base_addr)) == 0) {
            LOG_SHELL("Ok");
        }
    } else {
        LOG_SHELL("Fail");
    }

    return;
}

/**
 * @brief Partial download from internal flash memory address.
 * @param[in] argc Passes arg count from shell. Can be 1, 2, 3, 4.
 * @param[in] argv Passes argument list.
 * [0] cmd name.
 * [1] hex address to load from, as default 0x08060000 (FLASH_SECTOR #3).
 * [2] boolean flag reset progress, as default False.
 * [3] max length to load, as default unlimited.
 * @return None
 */
static void cmdDownloadFromFlash(int argc, char **argv)
{
    size_t addr_from = FLASH_BASE + INTERNAL_FLASH_SS_START_SECTOR * FLASH_SECTOR_SIZE;
    size_t bytes_to_load_before_exit = -1;
    bool reset_progress = false;
    switch (argc) {
        case 4:
            bytes_to_load_before_exit = strtol((const char *)argv[3], NULL, 10);
        case 3:
            reset_progress = strtol((const char *)argv[2], NULL, 10) ? true : false;
        case 2:
            addr_from = strtol((const char *)argv[1], NULL, 16);
        default:
            break;
    }

    /* prepare */
    size_t image_size = getImageFullSize(addr_from);
    LOG_SHELL("image info: isize=%d, from=%X", image_size, addr_from);

    /* get progress */
    size_t total_size;
    size_t downloaded;
    size_t offset = 0;
    partialDownload_getProgress(&total_size, &downloaded, PARTIAL_DOWNLOAD_MODE_FIRMWARE);

    if (total_size != image_size || reset_progress) {
        LOG_SHELL("start download again for %d bytes.", total_size);
        offset = 0;
        /* total_size = image_size; */
    } else if (downloaded == image_size) {
        LOG_SHELL("done -> downloaded=%d", downloaded);
        return;
    } else {
        offset = downloaded;
    }

    LOG_SHELL("continue: already downloaded=%d", downloaded);
    uint32_t loadedLen_session = 0;
    uint32_t next_len = PD_LOAD_LEN;

    if (bytes_to_load_before_exit == 0 && offset == 0) {
        const struct flash_area *fap;
        int rc = flash_area_open(FLASH_AREA_IMAGE_SECONDARY_ID, &fap);
        if (rc != 0) {
            LOG_SHELL("Err: flash_area_open = %d", rc);
            return;
        }

        size_t bytes_to_erase = downloaded? downloaded : EXTERNAL_FLASH_SECTOR_SIZE;
        rc = flash_area_erase(fap, 0, bytes_to_erase);
        if (rc != 0) {
            LOG_SHELL("Err: flash_area_erase = %d", rc);
            return;
        }

        /* Clear all progress */
        partialDownload_loadImage(NULL, 0, 0, 0, PARTIAL_DOWNLOAD_MODE_FIRMWARE);
        LOG_SHELL("%d bytes has been erased from external flash memory.", bytes_to_erase);
    }

    while (loadedLen_session < bytes_to_load_before_exit) {

        if (offset + next_len > image_size) {
            next_len = image_size - offset;
        }
        if (loadedLen_session + next_len > bytes_to_load_before_exit) {
            next_len = bytes_to_load_before_exit - loadedLen_session;
        }
        /* load new image */
        board_wdRefresh();
        if (partialDownload_loadImage((uint8_t *)(addr_from + offset), next_len, image_size,
                                      offset, PARTIAL_DOWNLOAD_MODE_FIRMWARE) != PARTIAL_DOWNLOAD_COMMAND_COMPLETED) {
            return;
        }
        offset += next_len;
        loadedLen_session += next_len;
        if (offset >= image_size) {
            break;
        }
    }
    
    LOG_SHELL("ready: downloaded=%ld", loadedLen_session);
}

/**
 * @brief This function used to obtain header size, full image should be placed in address.
 * @param   addr The memory area where placed start of image_header struct.
 * @return  uint32_t size of header.
 */
static uint32_t getHeaderSize(uint32_t addr)
{
    image_header_s *image_header = (image_header_s *)(addr);
    return image_header->ih_hdr_size;
}

/**
 * @brief This function used to obtain payload of image 
 * @param addr The memory area address where placed start of image_header struct.
 * @return uint32_t size of image
 */
static uint32_t getImageSize(uint32_t addr)
{
    image_header_s *image_header = (image_header_s *)(addr);
    return image_header->ih_img_size;
}

/**
 * @brief This function used to obtain FULL image size with header and with trailers, full image
 *        should be placed in address.
 * @param addr The memory area address where placed start of image_header struct.
 * @return uint32_t size of image
 */
static uint32_t getImageFullSize(uint32_t addr)
{
    image_header_s *image_header = (image_header_s *)(addr);
    struct image_tlv_info *trailer = (struct image_tlv_info *)(addr + image_header->ih_hdr_size +
                                                               image_header->ih_img_size +
                                                               image_header->ih_protect_tlv_size);

    return image_header->ih_hdr_size + image_header->ih_img_size +
           image_header->ih_protect_tlv_size + trailer->it_tlv_tot;
}

/**
 * @brief This function used to obtain size of signed image part, full image should be placed
 *        in address.
 * @param addr The memory area address where placed start of image_header struct.
 * @return None
 */
static uint32_t getImageSignedSize(uint32_t addr)
{
    image_header_s *image_header = (image_header_s *)(addr);
    return image_header->ih_hdr_size + image_header->ih_img_size +
           image_header->ih_protect_tlv_size;
}

/**
 * @brief Set default public key for bootloader
 * @param arg Pointer on arguments
 * @param arg_num Number of arguments
 * @return None
 */
static void resetPublic(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    libse_restoreDefaultPublicKeyMF();
}
