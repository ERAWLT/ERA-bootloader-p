/**
 * @file  secure.h
 * @brief This file contains security functions.
 */

#ifndef _SECURE_H_
#define _SECURE_H_

#include <stdint.h>
#include <stddef.h>

#define DIGEST_SIZE       (32) /*!< Digest length in bytes, must be equal SE_DIGEST_LEN */

/**
 * @brief Initializing data for working with the secure element (including IO protection
 * initialization if available).
 * @return None
 */
void secure_init(void);

/**
 * @brief   This function allows to get IO protection bytes to exchange with secure element.
 * @param[out]   iop The clear IOPV bytes.
 * @return  None
 */
void secure_getIOP(uint8_t iop[DIGEST_SIZE]);

/**
 * @brief   This function clears IO protection bytes.
 * @return  None
 */
void secure_deinitIOP(void);

/**
 * @brief   Sends secret bytes to application.
 * @return  None
 */
void secure_sendIOPtoMF(void);

#endif /*_SECURE_H_*/
