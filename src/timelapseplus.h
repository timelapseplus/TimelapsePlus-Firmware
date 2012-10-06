/*
 *  timelapseplus.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#define VERSION 20121005


volatile char timerRevert(char key, char first);
volatile char timerSaveCurrent(char key, char first);
volatile char timerSaveDefault(char key, char first);
volatile char menuBack(char key, char first);
volatile char timerStop(char key, char first);
volatile char runHandlerQuick(char key, char first);
volatile char runHandler(char key, char first);
volatile char lightMeter(char key, char first);
volatile char batteryStatus(char key, char first);
volatile char viewSeconds(char key, char first);
volatile char memoryFree(char key, char first);
volatile char IRremote(char key, char first);
volatile char sysInfo(char key, char first);
volatile char sysStatus(char key, char first);
volatile char cableRelease(char key, char first);
volatile char shutterTest(char key, char first);
volatile char shutterLagTest(char key, char first);
volatile char lcd_white(char key, char first);
volatile char lcd_red(char key, char first);
volatile char notYet(char key, char first);
volatile char usbPlug(char key, char first);
volatile char shutter_addKeyframe(char key, char first);
volatile char shutter_removeKeyframe(char key, char first);
volatile char shutter_saveAs(char key, char first);
volatile char shutter_load(char key, char first);

void setup(void);
void int_to_str(uint16_t n, char buf[6]);

