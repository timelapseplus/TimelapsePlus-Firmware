#ifndef _VIRTUALSERIAL_H_
#define _VIRTUALSERIAL_H_

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/power.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "Descriptors.h"

#include <LUFA/Version.h>
#include <LUFA/Drivers/USB/USB.h>

/* Function Prototypes: */
#ifdef __cplusplus
extern "C"
{
#endif
void EVENT_USB_Device_Connect(void);
void EVENT_USB_Device_Disconnect(void);
void EVENT_USB_Device_ConfigurationChanged(void);
void EVENT_USB_Device_ControlRequest(void);

void VirtualSerial_Reset(void);
void VirtualSerial_Task(void);
void VirtualSerial_PutChar(char c);
char VirtualSerial_CharWaiting(void);
char VirtualSerial_GetChar(void);
void VirtualSerial_PutString(char *s);
void VirtualSerial_Init(void);
void VirtualSerial_FlushBuffer(void);
extern char VirtualSerial_connected;
#ifdef __cplusplus
}
#endif
#endif
