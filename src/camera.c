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
 *  Main source file for the StillImageHost demo. This file contains the main tasks of
 *  the demo and is responsible for the initial application hardware configuration.
 */

#include "camera.h"

volatile char Camera_Connected;

/** Task to print device information through the serial port, and open/close a test PIMA session with the
 *  attached Still Image device.
 */
void Camera_Task(void)
{
    USB_USBTask();

	if (USB_HostState != HOST_STATE_Configured)
	  return;

	puts_P(PSTR("Retrieving Device Info...\r\n"));
	_delay_ms(50);

	Camera_GetModel();

	Camera_Open();
//	Camera_SetParam(EOS_DPC_ISO, EOS_DVC_ISO_1250);
	Camera_Capture();
	Camera_Close();

}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void Camera_Enable(void)
{
	/* Hardware Initialization */
	Serial_Init(115200, true);

	USB_Init(USB_MODE_Host);

	/* Create a stdio stream for the serial port for stdin and stdout */
	Serial_CreateStream(NULL);
}

void Camera_Disable(void)
{
	USB_Detach();
	USB_Disable();
	Camera_Connected = 0;
	return;	
}

/** Event handler for the USB_DeviceAttached event. This indicates that a device has been attached to the host, and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Host_DeviceAttached(void)
{
	Camera_Connected = 1;
	puts_P(PSTR( "Device Attached.\r\n" ));
}

/** Event handler for the USB_DeviceUnattached event. This indicates that a device has been removed from the host, and
 *  stops the library USB task management process.
 */
void EVENT_USB_Host_DeviceUnattached(void)
{
	Camera_Connected = 0;
	puts_P(PSTR( "\r\nDevice Unattached.\r\n" ));
}

/** Event handler for the USB_DeviceEnumerationComplete event. This indicates that a device has been successfully
 *  enumerated by the host and is now ready to be used by the application.
 */
void EVENT_USB_Host_DeviceEnumerationComplete(void)
{
	puts_P(PSTR("Getting Config Data.\r\n"));

	uint8_t ErrorCode;

	/* Get and process the configuration descriptor data */
	if ((ErrorCode = ProcessConfigurationDescriptor()) != SuccessfulConfigRead)
	{
		if (ErrorCode == ControlError)
		  puts_P(PSTR( "Control Error (Get Configuration).\r\n"));
		else
		  puts_P(PSTR( "Invalid Device.\r\n"));

		printf_P(PSTR(" -- Error Code: %d\r\n"), ErrorCode);

		return;
	}

	/* Set the device configuration to the first configuration (rarely do devices use multiple configurations) */
	if ((ErrorCode = USB_Host_SetDeviceConfiguration(1)) != HOST_SENDCONTROL_Successful)
	{
		printf_P(PSTR( "Control Error (Set Configuration).\r\n"
		                         " -- Error Code: %d\r\n"), ErrorCode);

		return;
	}

	puts_P(PSTR("Still Image Device Enumerated.\r\n"));
}

/** Event handler for the USB_HostError event. This indicates that a hardware error occurred while in host mode. */
void EVENT_USB_Host_HostError(const uint8_t ErrorCode)
{
	USB_Disable();
	Camera_Connected = 0;

	printf_P(PSTR( "Host Mode Error\r\n"
	                         " -- Error Code %d\r\n"), ErrorCode);

//	for(;;);
}

/** Event handler for the USB_DeviceEnumerationFailed event. This indicates that a problem occurred while
 *  enumerating an attached USB device.
 */
void EVENT_USB_Host_DeviceEnumerationFailed(const uint8_t ErrorCode,
                                            const uint8_t SubErrorCode)
{
	printf_P(PSTR( "Dev Enum Error\r\n"
	                         " -- Error Code %d\r\n"
	                         " -- Sub Error Code %d\r\n"
	                         " -- In State %d\r\n"), ErrorCode, SubErrorCode, USB_HostState);

}

void Camera_Capture(void)
{
	if (USB_HostState != HOST_STATE_Configured)
	  return;

	uint8_t ErrorCode;

	puts_P(PSTR("Taking picture...\r\n"));
	_delay_ms(50);

	PIMA_SendBlock = (PIMA_Container_t)
		{
			.DataLength    = PIMA_COMMAND_SIZE(0),
			.Type          = PIMA_CONTAINER_CommandBlock,
			.Code          = EOS_OC_CAPTURE,
			.TransactionID = 0x00000001,
			.Params        = {},
		};

	/* Send the GETDEVICEINFO block */
	SImage_SendBlockHeader();

	/* Receive the response data block */
	if ((ErrorCode = SImage_ReceiveBlockHeader()) != PIPE_RWSTREAM_NoError)
	{
		puts_P(PSTR("Error 8\r\n"));
		ShowCommandError(ErrorCode, false);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}

	return;
}


void Camera_SetProperty(uint32_t param, uint32_t val)
{
	if (USB_HostState != HOST_STATE_Configured)
	  return;

	uint8_t ErrorCode;

	puts_P(PSTR("Changing Property...\r\n"));
	_delay_ms(50);

	PIMA_SendBlock = (PIMA_Container_t)
		{
			.DataLength    = PIMA_COMMAND_SIZE(1)+2,
			.Type          = PIMA_CONTAINER_CommandBlock,
			.Code          = EOS_OC_PROPERTY_SET,
			.TransactionID = 0x00000001,
			.Params        = {param},
		};

	/* Send the GETDEVICEINFO block */
	SImage_SendBlockHeader();

	uint8_t		*SendData = ((uint8_t*)((void*)(&val)));

	/* Receive the response data block */
	if ((ErrorCode = SImage_ReceiveBlockHeader()) != PIPE_RWSTREAM_NoError)
	{
		puts_P(PSTR("Error 9\r\n"));
		ShowCommandError(ErrorCode, false);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}

	SImage_SendData(SendData, 2);

	/* Receive the response data block */
	if ((ErrorCode = SImage_ReceiveBlockHeader()) != PIPE_RWSTREAM_NoError)
	{
		puts_P(PSTR("Error 12\r\n"));
		ShowCommandError(ErrorCode, false);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}

	return;
}

void Camera_GetModel(void)
{
	if (USB_HostState != HOST_STATE_Configured)
	  return;

	uint8_t ErrorCode;

	PIMA_SendBlock = (PIMA_Container_t)
		{
			.DataLength    = PIMA_COMMAND_SIZE(0),
			.Type          = PIMA_CONTAINER_CommandBlock,
			.Code          = PIMA_OPERATION_GETDEVICEINFO,
			.TransactionID = 0x00000000,
			.Params        = {},
		};

	/* Send the GETDEVICEINFO block */
	SImage_SendBlockHeader();

	/* Receive the response data block */
	if ((ErrorCode = SImage_ReceiveBlockHeader()) != PIPE_RWSTREAM_NoError)
	{
		puts_P(PSTR("Error 1\r\n"));
		ShowCommandError(ErrorCode, false);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}

	/* Calculate the size of the returned device info data structure */
	uint16_t DeviceInfoSize = (PIMA_ReceivedBlock.DataLength - PIMA_COMMAND_SIZE(0));

	/* Create a buffer large enough to hold the entire device info */
	uint8_t DeviceInfo[DeviceInfoSize];

	/* Read in the data block data (containing device info) */
	SImage_ReadData(DeviceInfo, DeviceInfoSize);

	/* Once all the data has been read, the pipe must be cleared before the response can be sent */
	Pipe_ClearIN();

	/* Create a pointer for walking through the info dataset */
	uint8_t* DeviceInfoPos = DeviceInfo;

	/* Skip over the data before the unicode device information strings */
	DeviceInfoPos +=  8;                                          // Skip to VendorExtensionDesc String
	DeviceInfoPos += (1 + UNICODE_STRING_LENGTH(*DeviceInfoPos)); // Skip over VendorExtensionDesc String
	DeviceInfoPos +=  2;                                          // Skip over FunctionalMode
	DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Supported Operations Array
	DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Supported Events Array
	DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Supported Device Properties Array
	DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Capture Formats Array
	DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Image Formats Array

	/* Extract and convert the Manufacturer Unicode string to ASCII and print it through the USART */
	char Manufacturer[*DeviceInfoPos];
	UnicodeToASCII(DeviceInfoPos, Manufacturer);
	printf_P(PSTR("   Manufacturer: %s\r\n"), Manufacturer);
	_delay_ms(50);

	DeviceInfoPos += 1 + UNICODE_STRING_LENGTH(*DeviceInfoPos);   // Skip over Manufacturer String

	/* Extract and convert the Model Unicode string to ASCII and print it through the USART */
	char Model[*DeviceInfoPos];
	UnicodeToASCII(DeviceInfoPos, Model);
	printf_P(PSTR("   Model: %s\r\n"), Model);
	_delay_ms(50);

	DeviceInfoPos += 1 + UNICODE_STRING_LENGTH(*DeviceInfoPos);   // Skip over Model String

	/* Extract and convert the Device Version Unicode string to ASCII and print it through the USART */
	char DeviceVersion[*DeviceInfoPos];
	UnicodeToASCII(DeviceInfoPos, DeviceVersion);
	printf_P(PSTR("   Device Version: %s\r\n\r\n"), DeviceVersion);
	_delay_ms(50);

	/* Receive the final response block from the device */
	if ((ErrorCode = SImage_ReceiveBlockHeader()) != PIPE_RWSTREAM_NoError)
	{
		puts_P(PSTR("Error 2\r\n"));
		ShowCommandError(ErrorCode, false);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}

	/* Verify that the command completed successfully */
/*	if ((PIMA_ReceivedBlock.Type != PIMA_CONTAINER_ResponseBlock) || (PIMA_ReceivedBlock.Code != PIMA_RESPONSE_OK))
	{
		puts_P(PSTR("Error 3\r\n"));
		ShowCommandError(PIMA_ReceivedBlock.Code, true);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}
*/
	return;
}


void Camera_Open(void)
{
	if (USB_HostState != HOST_STATE_Configured)
	  return;

	uint8_t ErrorCode;

	puts_P(PSTR("Opening Session...\r\n"));
	_delay_ms(50);

	PIMA_SendBlock = (PIMA_Container_t)
		{
			.DataLength    = PIMA_COMMAND_SIZE(1),
			.Type          = PIMA_CONTAINER_CommandBlock,
			.Code          = PIMA_OPERATION_OPENSESSION,
			.TransactionID = 0x00000000,
			.Params        = {0x00000001},
		};

	/* Send the OPENSESSION block, open a session with an ID of 0x0001 */
	SImage_SendBlockHeader();

	/* Receive the response block from the device */
	if ((ErrorCode = SImage_ReceiveBlockHeader()) != PIPE_RWSTREAM_NoError)
	{
		puts_P(PSTR("Error 4\r\n"));
		ShowCommandError(ErrorCode, false);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}

	/* Verify that the command completed successfully */
/*	if ((PIMA_ReceivedBlock.Type != PIMA_CONTAINER_ResponseBlock) || (PIMA_ReceivedBlock.Code != PIMA_RESPONSE_OK))
	{
		puts_P(PSTR("Error 5\r\n"));
		ShowCommandError(PIMA_ReceivedBlock.Code, true);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}
*/
	return;	
}

void Camera_Close(void)
{
	if (USB_HostState != HOST_STATE_Configured)
	  return;

	uint8_t ErrorCode;

	puts_P(PSTR("Closing Session...\r\n"));
	_delay_ms(50);

	PIMA_SendBlock = (PIMA_Container_t)
		{
			.DataLength    = PIMA_COMMAND_SIZE(1),
			.Type          = PIMA_CONTAINER_CommandBlock,
			.Code          = PIMA_OPERATION_CLOSESESSION,
			.TransactionID = 0x00000001,
			.Params        = {0x00000001},
		};

	/* Send the CLOSESESSION block, close the session with an ID of 0x0001 */
	SImage_SendBlockHeader();

	/* Receive the response block from the device */
	if ((ErrorCode = SImage_ReceiveBlockHeader()) != PIPE_RWSTREAM_NoError)
	{
		puts_P(PSTR("Error 6\r\n"));
		ShowCommandError(ErrorCode, false);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}

	/* Verify that the command completed successfully */
/*	if ((PIMA_ReceivedBlock.Type != PIMA_CONTAINER_ResponseBlock) || (PIMA_ReceivedBlock.Code != PIMA_RESPONSE_OK))
	{
		puts_P(PSTR("Error 7\r\n"));
		ShowCommandError(PIMA_ReceivedBlock.Code, true);
		USB_Host_SetDeviceConfiguration(0);
		return;
	}
*/
	puts_P(PSTR("Done.\r\n"));
	_delay_ms(50);

	USB_Host_SetDeviceConfiguration(0);
	return;	
}

/** Function to convert a given Unicode encoded string to ASCII. This function will only work correctly on Unicode
 *  strings which contain ASCII printable characters only.
 *
 *  \param[in] UnicodeString  Pointer to a Unicode encoded input string
 *  \param[out] Buffer        Pointer to a buffer where the converted ASCII string should be stored
 */
void UnicodeToASCII(uint8_t* UnicodeString,
                    char* Buffer)
{
	/* Get the number of characters in the string, skip to the start of the string data */
	uint8_t CharactersRemaining = *(UnicodeString++);

	/* Loop through the entire unicode string */
	while (CharactersRemaining--)
	{
		/* Load in the next unicode character (only the lower byte, as only Unicode coded ASCII is supported) */
		*(Buffer++) = *UnicodeString;

		/* Jump to the next unicode character */
		UnicodeString += 2;
	}

	/* Null terminate the string */
	*Buffer = 0;
}

/** Displays a PIMA command error via the device's serial port.
 *
 *  \param[in] ErrorCode          Error code of the function which failed to complete successfully
 *  \param[in] ResponseCodeError  Indicates if the error is due to a command failed indication from the device, or a communication failure
 */
void ShowCommandError(uint8_t ErrorCode,
                      bool ResponseCodeError)
{
	const char* FailureType = ((ResponseCodeError) ? PSTR("Response Code != OK") : PSTR("Transaction Fail"));

	printf_P(PSTR( "Command Error (%S).\r\n"
	                         " -- Error Code %d\r\n" ), FailureType, ErrorCode);
	_delay_ms(50);

}

