/*
             LUFA Library
     Copyright (C) Dean Camera, 2012.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for StillImageHost.c.
 */

#ifndef _STILL_IMAGE_HOST_H_
#define _STILL_IMAGE_HOST_H_

//When defined, this breaks BT functionality
//#define PTP_DEBUG
//#define PTP_DEBUG_SELECTIVE

#define PTP_RETURN_OK 0
#define PTP_RETURN_ERROR 1
#define PTP_RETURN_DATA_REMAINING 2
#define PTP_FIRST_TIME 255

// was 1280
#define PTP_BUFFER_SIZE 1024

#define NO_RECEIVE_DATA 0
#define RECEIVE_DATA 1


/* Includes: */
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <stdio.h>

#include "hardware.h"
#include "PTP_Codes.h"

#include <LUFA/Drivers/USB/USB.h>
#if defined(PTP_DEBUG) || defined(PTP_DEBUG_SELECTIVE)
#include <LUFA/Drivers/Peripheral/Serial.h>
#endif
#ifdef __cplusplus
extern "C"
{
#endif

/* Function Prototypes: */
void PTP_Enable(void);
void PTP_Disable(void);
void PTP_Task(void);

void EVENT_USB_Host_HostError(const uint8_t ErrorCode);
void EVENT_USB_Host_DeviceAttached(void);
void EVENT_USB_Host_DeviceUnattached(void);
void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t ErrorCode,
                                            const uint8_t SubErrorCode);
void EVENT_USB_Host_DeviceEnumerationComplete(void);
uint16_t PTP_GetEvent(uint32_t *event_value);
uint8_t PTP_Transaction(uint16_t opCode, uint8_t receive_data, uint8_t paramCount, uint32_t *params, uint8_t dataBytes, uint8_t *data);
uint8_t PTP_FetchData(uint16_t offset);
uint8_t SI_Host_ReceiveResponseCode(USB_ClassInfo_SI_Host_t* const SIInterfaceInfo, PIMA_Container_t *PIMABlock);
uint8_t SI_Host_ReceiveEventHeaderTLP(USB_ClassInfo_SI_Host_t* const SIInterfaceInfo, PIMA_Container_t* const PIMAHeader);
uint8_t PTP_OpenSession(void);
uint8_t PTP_CloseSession(void);
uint8_t PTP_GetDeviceInfo(void);
void UnicodeToASCII(char *UnicodeString,
                char *Buffer, uint8_t MaxLength);

extern char PTP_Buffer[PTP_BUFFER_SIZE];
extern uint16_t PTP_Bytes_Received;
extern uint16_t PTP_Bytes_Total;
extern char PTP_CameraModel[23];
extern char PTP_CameraMake[23];
extern char PTP_CameraSerial[23];
extern volatile uint8_t PTP_Ready, PTP_Connected, PTP_Run_Task, PTP_IgnoreErrorsForNextTransaction;
extern volatile uint16_t PTP_Error, PTP_Response_Code;
extern uint16_t supportedOperationsCount;
extern uint16_t *supportedOperations; // note that this memory space is reused -- only available immediately after init
extern uint16_t supportedPropertiesCount;
extern uint16_t *supportedProperties; // note that this memory space is reused -- only available immediately after init


#ifdef __cplusplus
}
#endif

#endif
