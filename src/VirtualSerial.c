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
 *  Main source file for the VirtualSerial demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */
#define _VIRTUALSERIAL_
#include "VirtualSerial.h"
#include "hardware.h"

char VirtualSerial_connected;

/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
{
    .Config =
    {
        .ControlInterfaceNumber = 0,

        .DataINEndpointNumber = CDC_TX_EPNUM,
        .DataINEndpointSize = CDC_TXRX_EPSIZE,
        .DataINEndpointDoubleBank = false,

        .DataOUTEndpointNumber = CDC_RX_EPNUM,
        .DataOUTEndpointSize = CDC_TXRX_EPSIZE,
        .DataOUTEndpointDoubleBank = false,

        .NotificationEndpointNumber = CDC_NOTIFICATION_EPNUM,
        .NotificationEndpointSize = CDC_NOTIFICATION_EPSIZE,
        .NotificationEndpointDoubleBank = false,
    },
};

/** Standard file stream for the CDC interface when set up, so that the virtual CDC COM port can be
 *  used like any regular character stream in the C APIs
 */
static FILE USBSerialStream;


/** Main program entry point. This routine contains the overall program flow, including initial
 *  setup of all components and the main program loop.
 */
static char VirtualSerial_Buffer[31];
static uint8_t VirtualSerial_Buffer_Pointer;

void VirtualSerial_Task(void)
{
    CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
    USB_USBTask();
    int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
    if(!(ReceivedByte < 0))
    {
        VirtualSerial_Buffer[VirtualSerial_Buffer_Pointer] = (uint8_t)ReceivedByte;
        VirtualSerial_Buffer_Pointer++;
        if(VirtualSerial_Buffer_Pointer > sizeof(VirtualSerial_Buffer)) VirtualSerial_Buffer_Pointer = 0;
    }
}

void VirtualSerial_FlushBuffer()
{
    VirtualSerial_Buffer_Pointer = 0;
}

void VirtualSerial_PutChar(char c)
{
    fputc(c, &USBSerialStream);
}

void VirtualSerial_Reset(void)
{
    USB_Detach();
    //USB_ResetInterface();
    _delay_ms(1000);
    USB_Attach();
    //USB_Init(USB_MODE_Device);
    //VirtualSerial_FlushBuffer();
}

char VirtualSerial_CharWaiting(void)
{
    return VirtualSerial_Buffer_Pointer;
}

char VirtualSerial_GetChar(void)
{
    char c = 255;
    if(VirtualSerial_Buffer_Pointer > 0)
    {
        c = VirtualSerial_Buffer[0];
        for(uint8_t i = 1; i <= VirtualSerial_Buffer_Pointer; i++) VirtualSerial_Buffer[i - 1] = VirtualSerial_Buffer[i];
        VirtualSerial_Buffer_Pointer--;
    }
    return c;
}

void VirtualSerial_PutString(char *s)
{
    fputs(s, &USBSerialStream);
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void VirtualSerial_Init(void)
{
    /* Disable watchdog if enabled by bootloader/fuses */
//	MCUSR &= ~(1 << WDRF);
//	wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);

    /* Hardware Initialization */
    USB_Init(USB_MODE_Device);

    /* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
    CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

    sei();
}


/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
/*	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING); */
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
    VirtualSerial_connected = 0;
    USB_ResetInterface();
/*	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY); */
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    bool ConfigSuccess = true;
    VirtualSerial_connected = 1;

    ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

/*	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR); */
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
    CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

