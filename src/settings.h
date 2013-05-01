/*
 *  settings.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#define SETTINGS_VERSION 20130124

#define ALL -1
#define CANON 0
#define NIKON 1
#define SONY 2
#define MINOLTA 3
#define OLYMPUS 4
#define PENTAX 5
#define PANASONIC 6
#define OTHER 100 

#define BT_MODE_SLEEP 0
#define BT_MODE_DISCOVERABLE 1

#define HALF_PRESS_ENABLED 0
#define HALF_PRESS_DISABLED 1

#define INTERFACE_AUTO      0b111
#define INTERFACE_USB_CABLE 0b101
#define INTERFACE_USB       0b100
#define INTERFACE_IR        0b010
#define INTERFACE_CABLE     0b001

#define BRAMP_MODE_ALL      0b111
#define BRAMP_MODE_BULB_ISO 0b011
#define BRAMP_MODE_ISO      0b010
#define BRAMP_MODE_APERTURE 0b100
#define BRAMP_MODE_BULB     0b001

#define USB_CHANGE_MODE_ENABLED 0
#define USB_CHANGE_MODE_DISABLED 1

#define AUTO_RUN_OFF 0
#define AUTO_RUN_ON 1

struct settings
{
    char cameraFPS;
    char warnTime;      // 2 bytes
    char mirrorTime;    // 2 bytes
    char sysName[10];   // 10 bytes
    char bulbMode;
    char cameraMake;
    char lcdColor;
    char lcdBacklightTime;
    char sysOffTime;
    char flashlightOffTime;
    uint32_t settingsVersion;
    uint32_t shutterVersion;
    char devMode;
    uint32_t firmwareVersion;
    char auxPort;
    char btMode;
    char halfPress;
    uint16_t bulbOffset;
    uint8_t brampMode;
    uint8_t interface;
    uint8_t autoRun;
    uint8_t modeSwitch;
    uint16_t dollyPulse;
    char test6;
    char test7;
    char test8;
    char test9;
};

void settings_load(void);
void settings_save(void);
void settings_default(void);
void settings_update(void);
void settings_init(void);

extern uint8_t settings_reset;



