/***************************************************************************//**
 * @file btl_communication.h
 * @brief Communication interface for Silicon Labs Bootloader.
 * @author Silicon Labs
 * @version 1.6.0
 *******************************************************************************
 * # License
 * <b>Copyright 2016 Silicon Laboratories, Inc. http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#ifndef BTL_COMMUNICATION_H
#define BTL_COMMUNICATION_H

#include <stdint.h>
#include "api/btl_errorcode.h"

/***************************************************************************//**
 * @addtogroup Plugin
 * @{
 * @addtogroup Communication Communication
 * @{
 * @brief Host communication interface
 * @details
 *   The Communication plugin provides an interface for implementing
 *   communication with a host device, such as a computer or a microcontroller.
 * @section communication_impl Communication Protocol Implementations
 *   Several plugins implement the communication interface, using different
 *   transports and protocols.
 ******************************************************************************/

/***************************************************************************//**
 * Initialize hardware for the communication plugin.
 ******************************************************************************/
void communication_init(void);

/***************************************************************************//**
 * Initialize communication between the bootloader and external host. E.g.
 * indicate to the external host that we're alive and kicking.
 *
 * @return Error code indicating success or failure
 * @retval ::BOOTLOADER_OK on success
 * @retval ::BOOTLOADER_ERROR_COMMUNICATION_START on failure
 ******************************************************************************/
int32_t communication_start(void);

/***************************************************************************//**
 * This function is not supposed to return until the host signals the end of the
 * current session, or a new image has been flashed and verified.
 *
 * @return Error code indicating sucess or failure
 * @retval ::BOOTLOADER_OK when a new image was flashed
 * @retval ::BOOTLOADER_ERROR_COMMUNICATION_ERROR on communication error
 * @retval ::BOOTLOADER_ERROR_COMMUNICATION_DONE when no image was received
 * @retval ::BOOTLOADER_ERROR_COMMUNICATION_IMAGE_ERROR when received image
 *         is invalid
 ******************************************************************************/
int32_t communication_main(void);

/***************************************************************************//**
 * Stop communication between the bootloader and external host.
 ******************************************************************************/
void communication_shutdown(void);

/** @} addtogroup Communication */
/** @} addtogroup Plugin */

#endif // BTL_COMMUNICATION_H
