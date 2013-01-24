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

#define BT_MODE_SLEEP 0
#define BT_MODE_DISCOVERABLE 1
#define HALF_PRESS_ENABLED 0
#define HALF_PRESS_DISABLED 1

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
    char test0;
    char test1;
    char test2;
    char test3;
    char test4;
    char test5;
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