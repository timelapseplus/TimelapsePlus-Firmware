/*
 *  settings.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#define SETTINGS_VERSION 20130510

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

#define AUX_MODE_DISABLED 0
#define AUX_MODE_DOLLY 1
#define AUX_MODE_IR 2
#define AUX_MODE_SYNC 3


struct settings
{
    uint8_t cameraFPS;
    uint8_t warnTime;
    uint8_t mirrorTime;
    char sysName[13];
    uint8_t bulbMode;
    uint8_t cameraMake;
    uint8_t lcdColor;
    uint8_t lcdBacklightTime;
    uint8_t sysOffTime;
    uint8_t flashlightOffTime;
    uint32_t settingsVersion;
    uint32_t shutterVersion;
    uint8_t devMode;
    uint32_t firmwareVersion;
    uint8_t auxPort;
    uint8_t btMode;
    uint8_t halfPress;
    uint16_t bulbOffset;
    uint8_t brampMode;
    uint8_t interface;
    uint8_t autoRun;
    uint8_t modeSwitch;
    uint16_t dollyPulse;
    uint8_t lcdContrast;
    uint8_t lcdCoefficent;
    uint8_t lcdBias;
    uint8_t bulbMin;
    uint8_t isoMax;
    uint8_t apertureMax;
    uint8_t apertureMin;
    uint8_t debugEnabled;
    uint8_t arbitraryBulb;
    uint8_t menuWrap;
    uint8_t extendedRamp;
    uint16_t dollyPulse2;
    uint8_t lightIntegrationMinutes;
    uint16_t brampGap;
    uint16_t brampRateMax;
    uint16_t brampRateMin;
    uint8_t brampRateFactor;
    uint8_t nikonUSB;
    uint16_t bulbEndOffset;
    uint8_t pad[13];
};

void settings_load(void);
void settings_save(void);
void settings_default(void);
void settings_update(void);
void settings_init(void);

extern uint8_t settings_reset;



