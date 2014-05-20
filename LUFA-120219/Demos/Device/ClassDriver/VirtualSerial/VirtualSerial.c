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

#include "VirtualSerial.h"

#define shutter_mask 0b10101010
#define aux_mask 0b01010101

#define shutter_open 0b11110111 | shutter_mask
#define shutter_half 0b01010111 | shutter_mask
#define shutter_full 0b01010101 | shutter_mask



/** LUFA CDC Class driver interface configuration and state information. This structure is
 *  passed to all CDC Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber         = 0,

				.DataINEndpointNumber           = CDC_TX_EPNUM,
				.DataINEndpointSize             = CDC_TXRX_EPSIZE,
				.DataINEndpointDoubleBank       = false,

				.DataOUTEndpointNumber          = CDC_RX_EPNUM,
				.DataOUTEndpointSize            = CDC_TXRX_EPSIZE,
				.DataOUTEndpointDoubleBank      = false,

				.NotificationEndpointNumber     = CDC_NOTIFICATION_EPNUM,
				.NotificationEndpointSize       = CDC_NOTIFICATION_EPSIZE,
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
char VirtualSerial_Buffer[31];
char VirtualSerial_Buffer_Pointer;

void VirtualSerial_Task()
{
	CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
	USB_USBTask();
	int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
	if (!(ReceivedByte < 0))
	{
		VirtualSerial_Buffer[VirtualSerial_Buffer_Pointer] = (uint8_t)ReceivedByte;
		VirtualSerial_Buffer_Pointer++;
		if(VirtualSerial_Buffer_Pointer > size_of(VirtualSerial_Buffer)) VirtualSerial_Buffer_Pointer = 0;
	}
}

void VirtualSerial_PutChar(char c)
{
	fputc(c, &USBSerialStream);
}

char VirtualSerial_CharWaiting()
{
	return VirtualSerial_Buffer_Pointer;
}

char VirtualSerial_GetChar()
{
	char c = 255;
	if(VirtualSerial_Buffer_Pointer > 0)
	{
		c = VirtualSerial_Buffer[0];
		for(char i = 1; i <= VirtualSerial_Buffer_Pointer; i++) VirtualSerial_Buffer[i - 1] = VirtualSerial_Buffer[i];
		VirtualSerial_Buffer_Pointer--;
	}
	return c;
}

void VirtualSerial_PutString(char *s)
{
	fputs(s, &USBSerialStream);
}

void VirtualSerial_Reset(char asp_also)
{
  	if(asp_also) DDRA |= _BV(PA5);
	USB_Detach();
	//USB_ResetInterface();
	_delay_ms(1500);
	USB_Attach();
	//USB_Init(USB_MODE_Device);
	//VirtualSerial_FlushBuffer();
    if(asp_also) DDRA &= ~_BV(PA5); // Hold USB on for USBasp
}

void USBasp_Reset()
{
  	DDRA |= _BV(PA5);
	_delay_ms(1500);
    DDRA &= ~_BV(PA5); // Hold USB on for USBasp
}


int main(void)
{
	uint8_t mode = 0;
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	USB_Init();

	/* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	sei();

    DDRD |= _BV(PE4);
    DDRE |= _BV(PE2);
    PORTE |= _BV(PE2);

    DDRC = 0x00;
    PORTC = 0xFF;

    DDRA &= ~_BV(PA6);
    DDRA &= ~_BV(PA7);
    PORTA &= ~_BV(PA6);
    PORTA &= ~_BV(PA7);
    PORTA &= ~_BV(PA5);

  	DDRA |= _BV(PA5);
    _delay_ms(1500);
    DDRA &= ~_BV(PA5); // Hold USB on for USBasp

    uint32_t count = 0;

	for(;;)
	{
		count++;
		if(count > 4000000)
		{
			// Reset system
//			VirtualSerial_Reset(0);
//			DDRA &= ~_BV(PA6);PORTD &= ~_BV(PD4);
			count = 0;
		}	

		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();

		if((PINC & shutter_mask) == shutter_full)
		{
			DDRC = aux_mask;
			PORTC = ~aux_mask;
			//PORTD |= _BV(PD4);
		}
		else
		{
		    DDRC = 0x00;
		    PORTC = 0xFF;
		    //PORTD &= ~_BV(PD4);
		}

		int16_t ReceivedByte = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
		if (!(ReceivedByte < 0))
		{
			count = 0;
			if(mode == 1)
			{
				mode = 0;
				PORTC = (uint8_t)ReceivedByte;
			}
			if(mode == 2)
			{
				mode = 0;
				DDRC = (uint8_t)ReceivedByte;
			}
			else
			{
				mode = 0;
				if((uint8_t)ReceivedByte == 'L') PORTD |= _BV(PD4); 		// Light On
//				else if((uint8_t)ReceivedByte == 'O') PORTD &= ~_BV(PD4);	// Light Off
				else if((uint8_t)ReceivedByte == 'P') mode = 1;				// Set PORTC
				else if((uint8_t)ReceivedByte == 'D') mode = 2;				// Set DDRC
				else if((uint8_t)ReceivedByte == 'I') CDC_Device_SendByte(&VirtualSerial_CDC_Interface, (uint8_t)PINC);	// Read PINC
				else if((uint8_t)ReceivedByte == 'T') CDC_Device_SendByte(&VirtualSerial_CDC_Interface, (uint8_t)'+');	// Test response
				else if((uint8_t)ReceivedByte == 'S') {DDRA |= _BV(PA6);PORTD |= _BV(PD4);}		// Slow ISP Transfer
				else if((uint8_t)ReceivedByte == 'F') {DDRA &= ~_BV(PA6);PORTD &= ~_BV(PD4);}	// Fast ISP Transfer
				else if((uint8_t)ReceivedByte == 'R') {_delay_ms(100); DDRA |= _BV(PA7);}	// RESET
				else if((uint8_t)ReceivedByte == 'X') VirtualSerial_Reset(1);				// Reset USB Connection
				else if((uint8_t)ReceivedByte == 'U') USBasp_Reset();				// Reset USBasp Connection
			}
		}
		
	}
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void VirtualSerial_Init(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);

	/* Hardware Initialization */
	USB_Init();

	/* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	sei();
}


/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_ENUMERATING);
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	LEDs_SetAllLEDs(LEDMASK_USB_NOTREADY);
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

	LEDs_SetAllLEDs(ConfigSuccess ? LEDMASK_USB_READY : LEDMASK_USB_ERROR);
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

