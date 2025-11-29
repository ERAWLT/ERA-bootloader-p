/**
 * @file  secure.c
 * @brief This file contains security functions.
 */

#include "secure.h"

#include "mail_engine.h"
#include "flash_map_backend/flash_map_backend.h"
#include "sysflash/sysflash.h"
#include "board_crypto.h"
#include "parser.h"

#include "libse.h"
#include "board_backup_sram.h"
#include "board_rnd.h"
#include "stdio.h"

/** Uncomment to use software storage for secure element.
 *  If hardware SE enabled it's not necessary.
 */
#define USE_SOFT_STORAGE_FOR_SE

/** Uncommend to use max available MF size instead image size of MF */
#define USE_FULL_FLASH_SIZE_FOR_IOPX_MF    

/** Uncommend to use max available BL size instead image size of BL */
// #define USE_FULL_FLASH_SIZE_FOR_IOPX_BL    

/** Unique bytes type */
typedef struct {
    uint8_t *bytes; /*!< Pointer to allocated or static memory address */
    uint32_t len;   /*!< Available length in bytes */
} unique_bytes_s;

static void fillZeroes(volatile uint8_t *data, uint8_t len);
static void computeIOPX(uint8_t IOPX[DIGEST_SIZE]);
static void computeIOPXtoMF(uint8_t IOPX[DIGEST_SIZE]);
static void getBli(unique_bytes_s *Bli);
static void getMfi(unique_bytes_s *Mfi);
#ifdef USE_SOFT_STORAGE_FOR_SE
static bool secure_readSlot(uint8_t slot, uint8_t *data, size_t len);
static bool secure_writeSlot(uint8_t slot, uint8_t *data, size_t len);
#endif /* USE_SOFT_STORAGE_FOR_SE */

/** Memory for storing IOP bytes */
static uint8_t IOPVX[DIGEST_SIZE];

/**
 * @brief Show data buffer in log
 * @param buf data to show
 * @param len length of data in bytes
 */
static void print_buf(uint8_t *buf, int len, const char *msg)
{
    if (msg != NULL) {
        printf(msg);
    }
    for (int i = 0; i < len; i++) {
        printf("%02X", buf[i]);
    }
    printf("\n");
}

void secure_init(void)
{
    /* Receive data from mailbox */
    mailEngine_getIOP(IOPVX, DIGEST_SIZE);
    print_buf(IOPVX, 32, "IOPVX: ");

#ifdef USE_SOFT_STORAGE_FOR_SE
    static libse_soft_api_s libse_software_support = {
        .read = secure_readSlot,
        .write = secure_writeSlot,
        .get_iop = secure_getIOP,
        .is_initialized = false,
    };
    if (!backupSram_enable()) {
        printf("backup SRAM is not enabled!\n");
    }
#else /* USE_SOFT_STORAGE_FOR_SE */
    static libse_soft_api_s libse_software_support = {
        .get_iop = secure_getIOP,
        .is_initialized = false,
    };
#endif /* USE_SOFT_STORAGE_FOR_SE */
    /* Initialize secure element */
    int status = libse_init(&libse_software_support);
    if (status) {
        printf("Failed to init SE: %d.\n", status);
    }
}

void secure_getIOP(uint8_t iopv[DIGEST_SIZE])
{
    if (iopv == NULL) {
        return;
    }

    uint8_t IOPX[DIGEST_SIZE] = {0};
    computeIOPX(IOPX);
    print_buf(IOPX, 32, "IOPX get: ");
    for (uint32_t i = 0; i < DIGEST_SIZE; i++) {
        iopv[i] = IOPVX[i] ^ IOPX[i];
    }
    print_buf(iopv, 32, "IOPV: ");

    fillZeroes((volatile uint8_t *)IOPX, DIGEST_SIZE);
}

void secure_sendIOPtoMF(void)
{
    uint8_t IOPV[DIGEST_SIZE] = {0};
    secure_getIOP(IOPV);

    uint8_t IOPX_MF[DIGEST_SIZE] = {0};
    uint8_t IOPVX_MF[DIGEST_SIZE] = {0};
    computeIOPXtoMF(IOPX_MF);
    for (uint32_t i = 0; i < DIGEST_SIZE; i++) {
        IOPVX_MF[i] = IOPV[i] ^ IOPX_MF[i];
    }
    mailEngine_shareIOPtoMF(IOPVX_MF, DIGEST_SIZE);

    print_buf(IOPV, 32, "IOPV to mf: ");
    print_buf(IOPX_MF, 32, "IOPX to mf: ");
    print_buf(IOPVX_MF, 32, "IOPVX to mf: ");

    fillZeroes((volatile uint8_t *)IOPV, DIGEST_SIZE);
    fillZeroes((volatile uint8_t *)IOPX_MF, DIGEST_SIZE);
}

void secure_deinitIOP(void)
{
    fillZeroes((volatile uint8_t *)IOPVX, DIGEST_SIZE);
}

/**
 * @brief This function calculates and sets IO protection x-version bytes that pointed by IOPX.
 * @param[out]   IOPX Pointer to DIGEST_SIZE bytes to set the actual byte values.
 * @return  None
 */
static void computeIOPX(uint8_t IOPX[DIGEST_SIZE])
{
    unique_bytes_s BLI;
    getBli(&BLI); /* !< For BL and MF here will be their own firmware */

    crypto_sha256fullSequence(BLI.bytes, BLI.len, IOPX);
}

/**
 * @brief This function calculates and sets IO protection x-version bytes for application.
 * @param[out]   IOPX Pointer to DIGEST_SIZE bytes to set the actual byte values.
 * @return  None
 */
static void computeIOPXtoMF(uint8_t IOPX[DIGEST_SIZE])
{
    unique_bytes_s MFI;
    getMfi(&MFI); /* !< For BL and MF here will be their own firmware */

    crypto_sha256fullSequence(MFI.bytes, MFI.len, IOPX);
}

/**
 * @brief This function clears len bytes by data pointer.
 * @param[out]   data Pointer to data bytes to clean.
 * @param   len Length in bytes to clean.
 * @return  None
 */
static void fillZeroes(volatile uint8_t *data, uint8_t len)
{
    while (len--) {
        *data++ = 0x00U;
    }
}

/** Get start of FLASH region for current firmware. Defined in LD script */
extern uint32_t _firmware_flash_start; // to ? _edata

/**
 * @brief   This function allows to get signed bytes of bootloader image by updating the values in
 * struct.
 * @param[out]   Bli Pointer to struct, where .bytes will set to actual memory address with BLI
 * bytes, .len will set to actual length of this bytes.
 * @return  None
 */
static void getBli(unique_bytes_s *Bli)
{
    int ret = 0;
    bootutil_data_t boot_data = {0};

    Bli->bytes = NULL;
    Bli->len = 0;

    /* Read the primary slot of bootloader from internal flash memory. */
    ret = parser_initBootData(&boot_data, FLASH_AREA_IMAGE_SELF_ID);
    if (ret != 0) {
        printf("Image at %lX - not found (%d)\n",
               boot_data.fap != NULL ? boot_data.fap->fa_off : -1, ret);
        parser_deinitBootData(&boot_data);
        return;
    }

#ifndef USE_FULL_FLASH_SIZE_FOR_IOPX_BL
    Bli->bytes = (uint8_t *)boot_data.fap->fa_off;
    Bli->len = boot_data.hdr.ih_hdr_size + boot_data.hdr.ih_img_size +
               boot_data.hdr.ih_protect_tlv_size; /* Image signed part of BL */
#else
    Bli->bytes = (uint8_t *)boot_data.fap->fa_off;
    Bli->len = boot_data.fap->fa_size;
#endif
    parser_deinitBootData(&boot_data);
}

/**
 * @brief   This function allows to get application executable part of image by updating the values
 * in struct.
 * @param[out]   Mfi Pointer to struct, where .bytes will set to actual memory address with MFI
 * bytes, .len will set to actual length of this bytes.
 * @return  None
 */
static void getMfi(unique_bytes_s *Mfi)
{
    int ret = 0;
    bootutil_data_t boot_data = {0};

    Mfi->bytes = NULL;
    Mfi->len = 0;

    /* Read the primary slot of application from internal flash memory. */
    ret = parser_initBootData(&boot_data, FLASH_AREA_IMAGE_MAIN_APP_PRIMARY_ID);
    if (ret != 0) {
        printf("Image at %lX - not found (%d)\n",
               boot_data.fap != NULL ? boot_data.fap->fa_off : -1, ret);
        parser_deinitBootData(&boot_data);
        return;
    }

#ifndef USE_FULL_FLASH_SIZE_FOR_IOPX_MF
    Mfi->bytes = (uint8_t *)(boot_data.fap->fa_off + boot_data.hdr.ih_hdr_size);
    Mfi->len = boot_data.hdr.ih_img_size; /* Only executable part of MF */
#else
    Mfi->bytes = (uint8_t *)boot_data.fap->fa_off;
    Mfi->len = boot_data.fap->fa_size;
#endif
    parser_deinitBootData(&boot_data);
}

#ifdef USE_SOFT_STORAGE_FOR_SE
    /** Only slots with public keys are available in the bootloader.
     *  So it's start public key slot number.
     */
    #define START_SOFT_SLOT_IN_BL SLOT_ID_PUB_RESERVE
    /** The last public key slot available to bootloader. */
    #define END_SOFT_SLOT_IN_BL SLOT_ID_PUBKEY_SECRET
    /** Slot size required for software implementation of the SE slot. */
    #define SOFT_SLOT_SIZE (SE_KEY_LEN + SE_DIGEST_LEN)
    #if (BACKUPSRAM_LIBSE_AREA_SIZE < SOFT_SLOT_SIZE*(END_SOFT_SLOT_IN_BL-START_SOFT_SLOT_IN_BL))
        #error "Not enough memory has been allocated backupSram for the BL!"
    #endif
static bool secure_readSlot(uint8_t slot, uint8_t *data, size_t len)
{
    bool ret = false;
    do {
        if (len > SOFT_SLOT_SIZE) {
            break;
        }
        if (slot >= START_SOFT_SLOT_IN_BL && START_SOFT_SLOT_IN_BL <= END_SOFT_SLOT_IN_BL) {
            slot -= START_SOFT_SLOT_IN_BL;
            ret = backupSram_read(BACKUPSRAM_OFFSET_LIBSE_AREA + SOFT_SLOT_SIZE * slot, data, len);
        }
    } while(0);
    return ret;
}

static bool secure_writeSlot(uint8_t slot, uint8_t *data, size_t len)
{
    bool ret = false;
    do {
        if (len > SOFT_SLOT_SIZE) {
            break;
        }
        if (slot >= START_SOFT_SLOT_IN_BL && START_SOFT_SLOT_IN_BL <= END_SOFT_SLOT_IN_BL) {
            slot -= START_SOFT_SLOT_IN_BL;
            ret = backupSram_write(BACKUPSRAM_OFFSET_LIBSE_AREA + SOFT_SLOT_SIZE * slot, data, len);
        }
        if (ret) {
            /* Below the crutch for stm32h753. */
            uint8_t empty_bytes[4] = {0};
            ret = backupSram_write(BACKUPSRAM_OFFSET_FREE, empty_bytes, 4);
        }
    } while(0);
    return ret;
}
#endif /* USE_SOFT_STORAGE_FOR_SE */
