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

#include "PTP_Driver.h"

/** LUFA Still Image Class driver interface configuration and state information. This structure is
 *  passed to all Still Image Class driver functions, so that multiple instances of the same class
 *  within a device can be differentiated from one another.
 */

USB_ClassInfo_SI_Host_t DigitalCamera_SI_Interface =
    {
        .Config =
            {
                .DataINPipeNumber       = 1,
                .DataINPipeDoubleBank   = false,

                .DataOUTPipeNumber      = 2,
                .DataOUTPipeDoubleBank  = false,

                .EventsPipeNumber       = 3,
                .EventsPipeDoubleBank   = false,
            },
    };

char PTP_Buffer[512];
char PTP_CameraModel[21];
PIMA_Container_t PIMA_Block;
uint8_t PTP_Ready, PTP_Connected, configured;

/** Task to print device information through the serial port, and open/close a test PIMA session with the
 *  attached Still Image device.
 */
void PTP_Task(void)
{
    for(uint8_t i = 0; i < 100; i++)
    {
        USB_USBTask();
        if(USB_HostState == HOST_STATE_Configured)
        {
            if(configured != USB_HostState)
            {
                configured = USB_HostState;
                PTP_OpenSession();
                if(PTP_GetDeviceInfo() == 0) PTP_Ready = 1;
            }
        }
        else
        {
            configured = USB_HostState;
            PTP_Ready = 0;
        }
    }
}

/** Configures the board hardware and chip peripherals for the demo's functionality. */
void PTP_Enable(void)
{
    /* Hardware Initialization */
    Serial_Init(115200, true);

    USB_Init(USB_MODE_Host);

    /* Create a stdio stream for the serial port for stdin and stdout */
    Serial_CreateStream(NULL);
    puts_P(PSTR("Camera Enabled.\r\n"));
    PTP_Ready = 0;
}

void PTP_Disable(void)
{
    PTP_CloseSession();
    USB_USBTask();
    USB_Detach();
    USB_Disable();
    puts_P(PSTR("Camera Disabled.\r\n"));
    PTP_Ready = 0;
    PTP_Connected = 0;
    return;
}


uint8_t PTP_Transaction(uint16_t opCode, uint8_t mode, uint8_t paramCount, uint32_t *params)
{
    if(mode == 1)
        SI_Host_SendCommand(&DigitalCamera_SI_Interface, CPU_TO_LE16(opCode), 0, NULL);
    else
        SI_Host_SendCommand(&DigitalCamera_SI_Interface, CPU_TO_LE16(opCode), paramCount, params);

    if(mode == 1 && paramCount > 0 && params) // send data
    {
        DigitalCamera_SI_Interface.State.TransactionID--;

        PIMA_Block = (PIMA_Container_t)
        {
            .DataLength = CPU_TO_LE32(PIMA_DATA_SIZE(sizeof(uint32_t) * paramCount)),
            .Type = CPU_TO_LE16(PIMA_CONTAINER_DataBlock),
            .Code = CPU_TO_LE16(opCode),
        };
        memcpy(&PIMA_Block.Params, params, sizeof(uint32_t) * paramCount);
        SI_Host_SendBlockHeader(&DigitalCamera_SI_Interface, &PIMA_Block);
    }
    else if(mode == 2) // receive data
    {
        SI_Host_ReceiveBlockHeader(&DigitalCamera_SI_Interface, &PIMA_Block);
        uint16_t receivedBytes = (PIMA_Block.DataLength - PIMA_COMMAND_SIZE(0));
        printf_P(PSTR("   Bytes received: %d\r\n\r\n"), receivedBytes);
        SI_Host_ReadData(&DigitalCamera_SI_Interface, PTP_Buffer, receivedBytes);
        PTP_Buffer[receivedBytes] = 0;
    }

    if (SI_Host_ReceiveResponse(&DigitalCamera_SI_Interface))
    {
        puts_P(PSTR("PTP_Command Error.\r\n"));
        USB_Host_SetDeviceConfiguration(0);
        return 1;
    }
    return 0;
}


uint8_t PTP_OpenSession()
{
    if (SI_Host_OpenSession(&DigitalCamera_SI_Interface) != PIPE_RWSTREAM_NoError)
    {
        puts_P(PSTR("Could not open PIMA session.\r\n"));
        USB_Host_SetDeviceConfiguration(0);
        return 1;
    }
    return 0;
}

uint8_t PTP_CloseSession()
{
    if (SI_Host_CloseSession(&DigitalCamera_SI_Interface) != PIPE_RWSTREAM_NoError)
    {
        puts_P(PSTR("Could not close PIMA session.\r\n"));
        USB_Host_SetDeviceConfiguration(0);
        return 1;
    }
    USB_Host_SetDeviceConfiguration(0);
    return 0;
}


uint8_t PTP_GetDeviceInfo()
{
    if(PTP_Transaction(PIMA_OPERATION_GETDEVICEINFO, 2, 0, NULL)) return 1;
    char *DeviceInfoPos = PTP_Buffer;

    /* Skip over the data before the unicode device information strings */
    DeviceInfoPos += 8;                                          // Skip to VendorExtensionDesc String
    DeviceInfoPos += (1 + UNICODE_STRING_LENGTH(*DeviceInfoPos)); // Skip over VendorExtensionDesc String
    DeviceInfoPos += 2;                                          // Skip over FunctionalMode
    DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Supported Operations Array
    DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Supported Events Array
    DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Supported Device Properties Array
    DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Capture Formats Array
    DeviceInfoPos += (4 + (*(uint32_t*)DeviceInfoPos << 1));      // Skip over Image Formats Array

    /* Extract and convert the Manufacturer Unicode string to ASCII and print it through the USART */
    char Manufacturer[*DeviceInfoPos];
    UnicodeToASCII(DeviceInfoPos, Manufacturer);
    printf_P(PSTR("   Manufacturer: %s\r\n"), Manufacturer);

    DeviceInfoPos += 1 + UNICODE_STRING_LENGTH(*DeviceInfoPos);   // Skip over Manufacturer String

    /* Extract and convert the Model Unicode string to ASCII and print it through the USART */
    char Model[*DeviceInfoPos];
    UnicodeToASCII(DeviceInfoPos, Model);
    printf_P(PSTR("   Model: %s\r\n"), Model);

    strncpy(PTP_CameraModel, Model, 20);

    DeviceInfoPos += 1 + UNICODE_STRING_LENGTH(*DeviceInfoPos);   // Skip over Model String

    /* Extract and convert the Device Version Unicode string to ASCII and print it through the USART */
    char DeviceVersion[*DeviceInfoPos];
    UnicodeToASCII(DeviceInfoPos, DeviceVersion);
    printf_P(PSTR("   Device Version: %s\r\n\r\n"), DeviceVersion);
    return 0;
}

/** Event handler for the USB_DeviceAttached event. This indicates that a device has been attached to the host, and
 *  starts the library USB task to begin the enumeration and USB management process.
 */
void EVENT_USB_Host_DeviceAttached(void)
{
    PTP_Connected = 1;
    puts_P(PSTR("Device Attached.\r\n"));
}

/** Event handler for the USB_DeviceUnattached event. This indicates that a device has been removed from the host, and
 *  stops the library USB task management process.
 */
void EVENT_USB_Host_DeviceUnattached(void)
{
    PTP_Connected = 0;
    puts_P(PSTR("\r\nDevice Unattached.\r\n"));
}

/** Event handler for the USB_DeviceEnumerationComplete event. This indicates that a device has been successfully
 *  enumerated by the host and is now ready to be used by the application.
 */
void EVENT_USB_Host_DeviceEnumerationComplete(void)
{
    uint16_t ConfigDescriptorSize;

    if (USB_Host_GetDeviceConfigDescriptor(1, &ConfigDescriptorSize, PTP_Buffer,
                                           sizeof(PTP_Buffer)) != HOST_GETCONFIG_Successful)
    {
        puts_P(PSTR("Error Retrieving Configuration Descriptor.\r\n"));
        return;
    }

    if (SI_Host_ConfigurePipes(&DigitalCamera_SI_Interface,
                               ConfigDescriptorSize, PTP_Buffer) != SI_ENUMERROR_NoError)
    {
        puts_P(PSTR("Attached Device Not a Valid Still Image Class Device.\r\n"));
        return;
    }

    if (USB_Host_SetDeviceConfiguration(1) != HOST_SENDCONTROL_Successful)
    {
        puts_P(PSTR("Error Setting Device Configuration.\r\n"));
        return;
    }

    puts_P(PSTR("Still Image Device Enumerated.\r\n"));
}

/** Event handler for the USB_HostError event. This indicates that a hardware error occurred while in host mode. */
void EVENT_USB_Host_HostError(const uint8_t ErrorCode)
{
    PTP_Disable();

    printf_P(PSTR( "Host Mode Error\r\n"
                             " -- Error Code %d\r\n" ), ErrorCode);

    for(;;);
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
                             " -- In State %d\r\n" ), ErrorCode, SubErrorCode, USB_HostState);

}

/** Function to convert a given Unicode encoded string to ASCII. This function will only work correctly on Unicode
 *  strings which contain ASCII printable characters only.
 *
 *  \param[in] UnicodeString  Pointer to a Unicode encoded input string
 *  \param[out] Buffer        Pointer to a buffer where the converted ASCII string should be stored
 */
void UnicodeToASCII(char *UnicodeString,
                    char *Buffer)
{
    /* Get the number of characters in the string, skip to the start of the string data */
    uint8_t CharactersRemaining = *(UnicodeString);
    UnicodeString++;

    /* Loop through the entire unicode string */
    while (--CharactersRemaining)
    {
        /* Load in the next unicode character (only the lower byte, as only Unicode coded ASCII is supported) */
        *(Buffer++) = *UnicodeString;

        /* Jump to the next unicode character */
        UnicodeString += 2;
    }

    /* Null terminate the string */
    *Buffer = 0;
}


