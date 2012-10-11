/*
 *  hardware.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#ifndef	HARDWARE_H
#define HARDWARE_H

//This is the hardware map for the prototype version
//#include "hardware_map_20120329.h"

//This is the hardware main for the production version
#include "hardware_map_20120703.h"

#ifdef __cplusplus
extern "C"
{
#endif

struct light_reading
{
    int level1;
    int level2;
    int level3;
};

void hardware_init(void);
int hardware_freeMemory(void);
unsigned int hardware_readLight(uint8_t r);
void hardware_readLightAll(void *result);
uint16_t hardware_analogRead(uint8_t ch);
void hardware_off(void);
char hardware_flashlight(char on);
char hardware_flashlightIsOn(void);
void hardware_bootloader(void);

uint16_t battery_read_raw(void);
uint8_t battery_read(void);
char battery_status(void);

#ifdef __cplusplus
}
#endif

/* MACROS */
#define hardware_USB_InHostMode (isHigh(UVCONN_PIN))
#define hardware_USB_InDeviceMode (!hardware_USB_InHostMode)
#define hardware_USB_HostConnected (!getPin(USBID_PIN))
#define hardware_USB_SetHostMode() setOut(UVCONN_PIN); setOut(USB_SELECT_PIN); setHigh(USB_SELECT_PIN); setHigh(UVCONN_PIN)
#define hardware_USB_SetDeviceMode() setOut(UVCONN_PIN); setOut(USB_SELECT_PIN); setLow(USB_SELECT_PIN); setLow(UVCONN_PIN)
#define hardware_USB_Enable() setIn(USBID_PIN); setHigh(USBID_PIN)
#define hardware_USB_Disable() setLow(USBID_PIN)

#define FOREVER         for(;;)

#define INPUT 0
#define OUTPUT 1
#define HIGH 1
#define LOW 0
#define clrBit(b, p) p &= ~_BV(b)
#define setBit(b, p) p |= _BV(b)
#define getBit(b, p) ((p & _BV(b)) != 0)

#define _setOut(b, p) DDR ## p |= _BV(b)
#define _setIn(b, p) DDR ## p &= ~_BV(b)
#define _setHigh(b, p) PORT ## p |= _BV(b)
#define _setLow(b, p) PORT ## p &= ~_BV(b)
#define _getPin(b, p) ((PIN ## p & _BV(b)) != 0)
#define _isOut(b, p) ((DDR ## p & _BV(b)) != 0)
#define _isHigh(b, p) ((PORT ## p & _BV(b)) != 0)

#define setOut(args) _setOut(args)
#define setIn(args) _setIn(args)
#define setHigh(args) _setHigh(args)
#define setLow(args) _setLow(args)
#define getPin(args) _getPin(args)
#define isOut(args) _isOut(args)
#define isHigh(args) _isHigh(args)

#endif

