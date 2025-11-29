/**
 * @file screens.h
 * @brief Screens header. Arrays of screen function and themselves declared here.
 */
#ifndef _SCREENS_H_
#define _SCREENS_H_

#include <stdbool.h>

/** Screen numbers to display */
typedef enum {
    SCREEN_ID_WAIT_USER,         /*!< User waiting screen */
    SCREEN_ID_LOW_BATTERY_LEVEL, /*!< Low battery level screen */
    SCREEN_ID_INFO,              /*!< Info screen */
    SCREEN_ID_SHIPPING,          /*!< Shipping screen */
    SCREEN_ID_DOWNLOAD,          /*!< Download screen */
    SCREEN_ID_UPDATING,          /*!< Update screen */
    SCREEN_ID_ERROR,             /*!< Error screen */
    SCREEN_ID_FORCE_DOWNLOAD,    /*!< Force download screen */
    SCREEN_ID_MORE_INFO,         /*!< Additional info screen */
    SCREEN_NUM_ID,               /*!< Number of screens */
} screen_id_e;

void screenDownload_switch(void);
void screenError_switch(void);
void screenInfo_switch(void);
void screenInfo2_switch(void);
void screenShipping_switch(void);
void screenUpdating_switch(void);
void screenLowBattery_switch(void);
void screenWaitUpdate_switch(void);
void screenForceDownload_switch(void);

bool screenDownload_poll(void);
bool screenError_poll(void);
bool screenInfo_poll(void);
bool screenInfo2_poll(void);
bool screenShipping_poll(void);
bool screenUpdating_poll(void);
bool screenLowBattery_poll(void);
bool screenWaitUpdate_poll(void);
bool screenForceDownload_poll(void);

#endif /* _SCREENS_H_ */
