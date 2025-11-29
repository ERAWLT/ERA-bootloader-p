/**
 * @file endOfWork_manager.c
 * @brief This file contains end of work functions.
 *
 * It includes functionalities to prepare the system for the jump by disabling
 * interrupts and resetting peripherals to a clean state.
 * 
 * @note Almost all functions are implemented in the blocking-thread mode.
 */

#include "endOfWork_manager.h"

#include "mail_engine.h"
#include "board_partial_download.h"

#include "board_batt_control.h"
#include "board_pwr.h"
#include "board.h"

#include "shell_bootloader.h"
#include "cmsis_os.h" /* To use delay for print */

#define INTERRUPT_REGISTERS_COUNT  (8)   /*!< Number of NVIC registers */

typedef void (*pFunction)(void);

static void prepareToJump(void);
static void prepareToShutdown(void);

void endOfWork_jumpTo(uint32_t address)
{
    printf("Jump to %lX...\n", address);
    TEST_WAIT_CONSOLE();
#ifdef MCUBOOT_HAVE_LOGGING
    osDelay(1000); /* Temporary to check log */
#endif

    prepareToJump();

    pFunction Jump_To_Application;
    uint32_t JumpAddress;

    /* Rebase the stack pointer */
    __set_MSP(*(volatile uint32_t *)address);

    /* Get the function pointer to the reset vector address */
    JumpAddress = *(volatile uint32_t *)(address + 4);
    Jump_To_Application = (pFunction)JumpAddress;

    Jump_To_Application();
}

void endOfWork_goSleep(void)
{
    printf("Going to standby mode...\n");
    prepareToShutdown();
#ifdef MCUBOOT_HAVE_LOGGING
    osDelay(100); // check work without it!
#endif

#if (HWLT_BOARD_REVISION_NUM == 1)
    board_sleep(PWR_WKUP_STANDBY_SRC_NFC | PWR_WKUP_STANDBY_SRC_WCHARGE);
#else
    board_sleep(PWR_WKUP_STANDBY_SRC_BUTTON | PWR_WKUP_STANDBY_SRC_NFC | PWR_WKUP_STANDBY_SRC_WCHARGE);
#endif
}

bool endOfWork_shippingMode(void)
{
    printf("Request for shipping mode...\n");
    prepareToShutdown();
    return battControl_requestForShippingMode();
}

void endOfWork_powerOff(void)
{
    printf("Power off via smart button...\n");
    prepareToShutdown();
    smartButton_powerOff();
}

void endOfWork_reboot(void)
{
    printf("reboot...\n");
    prepareToJump();
    board_reboot();
}

/**
 * @brief Prepare MCU to jump.
 * @return None
 */
static void prepareToJump(void)
{
    prepareToShutdown();
    vPortEnterCritical();

    __disable_irq();
    HAL_RCC_DeInit();
    HAL_DeInit();
    for (uint8_t i = 0; i < INTERRUPT_REGISTERS_COUNT; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF; /* Disable all interrupts */
        NVIC->ICPR[i] = 0xFFFFFFFF; /* Clear pendings interrupts */
    }
}

/**
 * @brief Prepare the bootloader to be turned off.
 * @return None
 * @note It is executed only once
 */
static void prepareToShutdown(void)
{
    static bool request_completed = false;
    if (!request_completed) {
        request_completed = true;
        partialDownload_pause();
        mailEngine_removeMsgToBL();
    }
}
