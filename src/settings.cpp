/*
 *  settings.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#include <avr/eeprom.h>
#include <string.h>
#include "settings.h"
#include "5110LCD.h"
#include "IR.h"
#include "shutter.h"
#include "bluetooth.h"
#include "remote.h"
#include "timelapseplus.h"
#include "tlp_menu_functions.h"

settings conf_eep EEMEM;
volatile settings conf;
uint8_t settings_reset = 0;

extern LCD lcd;
extern IR ir;
extern Remote remote;
extern shutter timer;
extern BT bt;

/******************************************************************
 *
 *   settings_save
 *
 *
 ******************************************************************/

void settings_save()
{
    if(conf.cameraMake == PANASONIC) conf.halfPress = HALF_PRESS_DISABLED;
    eeprom_write_block((const void*)&conf, &conf_eep, sizeof(settings));
    lcd.init(conf.lcdContrast);
    lcd.update();
}

/******************************************************************
 *
 *   settings_load
 *
 *
 ******************************************************************/

void settings_load()
{
    eeprom_read_block((void*)&conf, &conf_eep, sizeof(settings));
    if(conf.lcdContrast > 0xf || conf.lcdContrast < 0x1) conf.lcdContrast = 0x8;
    if(conf.lcdCoefficent > 0x7 || conf.lcdCoefficent < 0x3) conf.lcdCoefficent = 0x7;
    if(conf.lcdBias > 0x4 || conf.lcdBias < 0x3) conf.lcdBias = 0x4;
    if(conf.apertureMin > 100 || conf.apertureMin < 2) conf.apertureMin = 2;
    if(conf.lightIntegrationMinutes == 255 || conf.lightIntegrationMinutes == 0) conf.lightIntegrationMinutes = 10;
    if(conf.brampGap > 20 || conf.brampGap == 0) conf.brampGap = 6;
    if(conf.brampRateMax > 50 || conf.brampRateMax == 0) conf.brampRateMax = 16;
    if(conf.brampRateMin > 20) conf.brampRateMin = 4;
    if(conf.brampRateFactor > 30 || conf.brampRateFactor == 0) conf.brampRateFactor = 20;
    lcd.color(conf.lcdColor);
    ir.init();
    ir.make = conf.cameraMake;
    if(conf.auxPort != AUX_MODE_DISABLED)
    {
        aux1_off();
        aux2_off();
    }
    if(bt.present && !remote.connected)
    {
        if(conf.btMode == BT_MODE_SLEEP) bt.sleep(); else bt.advertise();
    }
}

/******************************************************************
 *
 *   settings_update
 *
 *
 ******************************************************************/

void settings_update()
{
    settings_save();
    settings_load();
}

/******************************************************************
 *
 *   settings_default
 *      restores factory defaults
 *
 ******************************************************************/

void settings_default()
{
    timer.setDefault();
    strcpy((char*)conf.sysName, "           ");
    conf.warnTime = 2;
    conf.mirrorTime = 2;
    conf.cameraFPS = 33;
    conf.bulbMode = 0;
    conf.lcdColor = 0;
    conf.cameraMake = CANON;
    conf.settingsVersion = SETTINGS_VERSION;
    conf.shutterVersion = SHUTTER_VERSION;
    conf.lcdBacklightTime = 3;
    conf.sysOffTime = 12;
    conf.flashlightOffTime = 3;
    conf.devMode = 0;
    conf.auxPort = AUX_MODE_DISABLED;
    conf.btMode = BT_MODE_SLEEP;
    conf.halfPress = HALF_PRESS_ENABLED;
    conf.bulbOffset = 75;
    conf.interface = INTERFACE_AUTO;
    conf.brampMode = BRAMP_MODE_BULB_ISO;
    conf.autoRun = AUTO_RUN_OFF;
    conf.modeSwitch = USB_CHANGE_MODE_DISABLED;
    conf.dollyPulse = 100;
    conf.dollyPulse2 = 100;
    conf.lcdContrast = 3;
    conf.lcdCoefficent = 0x7;
    conf.lcdBias = 0x3;
    conf.bulbMin = 56;
    conf.isoMax = 10;
    conf.apertureMax = 31;
    conf.apertureMin = 2;
    conf.debugEnabled = 0;
    conf.arbitraryBulb = 0;
    conf.menuWrap = 1;
    conf.extendedRamp = 0;
    conf.lightIntegrationMinutes = 10;
    conf.brampGap = 6;
    conf.brampRateMax = 16;
    conf.brampRateMin = 4;
    conf.brampRateFactor = 20;
    conf.nikonUSB = 0;

    settings_save();
    settings_load();
}

/******************************************************************
 *
 *   settings_init
 *
 *
 ******************************************************************/

void settings_init()
{
    settings_load();
    uint8_t need_save = 0;
    if(conf.shutterVersion != SHUTTER_VERSION)
    {
        timer.setDefault();
        conf.shutterVersion = SHUTTER_VERSION;
        need_save = 1;
    }
    if(conf.settingsVersion != SETTINGS_VERSION)
    {
        settings_default();
        need_save = 0;
        settings_reset = 1;
        // This is where we'd put a setup wizard
    }
    if(need_save) settings_save();
}

