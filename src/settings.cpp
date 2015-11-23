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
#include <avr/wdt.h>
#include "settings.h"
#include "5110LCD.h"
#include "IR.h"
#include "shutter.h"
#include "bluetooth.h"
#include "remote.h"
#include "nmx.h"
#include "timelapseplus.h"
#include "tlp_menu_functions.h"

settings_t conf_eep EEMEM;
camera_settings_t camera_settings_eep[MAX_CAMERAS_SETTINGS] EEMEM;
char lastNMXaddress[14] EEMEM;

volatile settings_t conf;
uint8_t settings_reset = 0;
uint8_t settings_camera_index = 0;

extern LCD lcd;
extern IR ir;
extern Remote remote;
extern shutter timer;
extern BT bt;
extern NMX motor1;
extern NMX motor2;
extern NMX motor3;

/******************************************************************
 *
 *   settings_save
 *
 *
 ******************************************************************/

void settings_save()
{
    if(conf.camera.cameraMake == PANASONIC) conf.camera.halfPress = HALF_PRESS_DISABLED;
    if(settings_camera_index > 0)
    {
        eeprom_write_block((const void*)&conf.camera, &camera_settings_eep[settings_camera_index - 1], sizeof(camera_settings_t));
        eeprom_read_block((void*)&conf.camera, &conf_eep.camera, sizeof(camera_settings_t));
    }
    conf.camera.autoConfigured = 0;
    eeprom_write_block((const void*)&conf, &conf_eep, sizeof(settings_t));
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
    eeprom_read_block((void*)&conf, &conf_eep, sizeof(settings_t));
    if(settings_camera_index > 0)
    {
        eeprom_read_block((void*)&conf.camera, &camera_settings_eep[settings_camera_index - 1], sizeof(camera_settings_t));
    }
    //if(conf.lcdContrast > 0xf || conf.lcdContrast < 0x1) conf.lcdContrast = 0x8;
    //if(conf.lcdCoefficent > 0x7 || conf.lcdCoefficent < 0x3) conf.lcdCoefficent = 0x7;
    //if(conf.lcdBias > 0x4 || conf.lcdBias < 0x3) conf.lcdBias = 0x4;
    if(conf.camera.brampGap > 20 || conf.camera.brampGap == 0) conf.camera.brampGap = 6;
    //if(conf.camera.canonLVOC > 0) conf.camera.canonLVOC = 0;
    if(!timer.running) lcd.color(conf.lcdColor);
    ir.init();
    ir.make = conf.camera.cameraMake;
    if(conf.auxPort != AUX_MODE_DISABLED)
    {
        aux1_off();
        aux2_off();
    }
    if(bt.present && !remote.connected)
    {
        if(conf.btMode == BT_MODE_SLEEP && !remote.nmx) bt.sleep(); else bt.advertise();
        if(remote.nmx)
        {
          if(motor1.connected && motor1.enabled && !motor1.running())
          {
            motor1.enable();
          }
          if(motor2.connected && motor2.enabled && !motor2.running())
          {
            motor2.enable();
          }
          if(motor3.connected && motor3.enabled && !motor3.running())
          {
            motor3.enable();
          }
        }
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
    memset((void*)&conf, 0, sizeof(settings_t));
    strcpy((char*)conf.sysName, "           ");
    conf.warnTime = 2;
    conf.mirrorTime = 2;
    //conf.lcdColor = 0;
    conf.settingsVersion = SETTINGS_VERSION;
    conf.shutterVersion = SHUTTER_VERSION;
    conf.lcdBacklightTime = 3;
    conf.sysOffTime = 12;
    conf.flashlightOffTime = 3;
    //conf.devMode = 0;
    conf.auxPort = AUX_MODE_DISABLED;
    conf.btMode = BT_MODE_SLEEP;
    conf.brampMode = BRAMP_MODE_BULB_ISO;
    conf.autoRun = AUTO_RUN_OFF;
    conf.dollyPulse = 100;
    conf.dollyPulse2 = 100;
    conf.lcdContrast = 3;
    conf.lcdCoefficent = 0x7;
    conf.lcdBias = 0x3;
    conf.isoMax = 10;
    conf.apertureMax = 31;
    conf.apertureMin = 2;
    //conf.debugEnabled = 0;
    //conf.arbitraryBulb = 0;
    conf.menuWrap = 1;
    //conf.extendedRamp = 0;
    conf.lightIntegrationMinutes = 5;
    conf.pFactor = 10;
    conf.iFactor = 11;
    conf.dFactor = 11;
    //conf.errorAlert = 0;
    conf.lightThreshold = 18;
    //conf.linearInterpolation = 0;
    conf.motionStep1 = 1000;
    conf.motionStep2 = 250;
    conf.motionStep3 = 250;
    conf.focusStep = 10;
    conf.focusEnabled = 1;
    //conf.pcSyncRequired = 0;
    //conf.motionBacklash1 = 0;
    //conf.motionBacklash2 = 0;
    //conf.motionBacklash3 = 0;
    //conf.motionPowerSave1 = 0;
    //conf.motionPowerSave2 = 0;
    //conf.motionPowerSave3 = 0;
    conf.motionSetupMove1 = 1;
    conf.motionSetupMove2 = 1;
    conf.motionSetupMove3 = 1;
    //conf.motionUnit1 = 0;
    //conf.motionUnit2 = 0;
    //conf.motionUnit3 = 0;
    //conf.motionMicroSteps1 = 0;
    //conf.motionMicroSteps2 = 0;
    //conf.motionMicroSteps3 = 0;
    conf.motionSpeed1 = 256;
    conf.motionSpeed2 = 256;
    conf.motionSpeed3 = 256;
    //conf.keyframeTimeByMinute = 0;
    conf.cameraPort = AUX_CAM_DEFAULT;
    
    eeprom_write_byte((uint8_t *)lastNMXaddress, 0);


    conf.camera.cameraFPS = 33;
    //conf.camera.nikonUSB = 0;
    conf.camera.bulbEndOffset = 8;
    conf.camera.bulbMin = 56;
    conf.camera.bulbOffset = 75;
    conf.camera.negBulbOffset = 0;
    conf.camera.interface = INTERFACE_AUTO;
    conf.camera.cameraMake = CANON;
    //conf.camera.bulbMode = 0;
    conf.camera.halfPress = HALF_PRESS_ENABLED;
    conf.camera.modeSwitch = USB_CHANGE_MODE_DISABLED;
    conf.camera.brampGap = 6;
    //conf.camera.canonLVOC = 0;

    for(uint8_t i = 0; i < MAX_CAMERAS_SETTINGS; i++)
    {
        camera_settings_t cs;
        memset((void*)&cs, 0, sizeof(camera_settings_t));
        eeprom_write_block((const void*)&cs, &camera_settings_eep[i], sizeof(camera_settings_t));
        wdt_reset();
    }

    settings_save();
    wdt_reset();
    settings_load();
    wdt_reset();
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
        wdt_reset();
        timer.setDefault();
        conf.shutterVersion = SHUTTER_VERSION;
        need_save = 1;
    }
    if(conf.settingsVersion != SETTINGS_VERSION)
    {
        wdt_reset();
        settings_default();
        need_save = 0;
        settings_reset = 1;
        // This is where we'd put a setup wizard
    }
    wdt_reset();
    if(need_save) settings_save();
}

void settings_load_camera_default()
{
    settings_camera_index = 0;
    settings_load();
}

void settings_load_camera_index(uint8_t index)
{
    settings_camera_index = index + 1;
    settings_load();
}

void settings_setup_camera_index(char *serial)
{
    if(strlen(serial) == 0) return;

    uint8_t last_empty_index = MAX_CAMERAS_SETTINGS - 1;
    camera_settings_t cs;

    for(uint8_t i = 0; i < MAX_CAMERAS_SETTINGS; i++)
    {
        wdt_reset();
        eeprom_read_block((void*)&cs, &camera_settings_eep[i], sizeof(camera_settings_t));
        if(strncmp(serial, cs.cameraSerial, 21) == 0)
        {
            settings_load_camera_index(i);
            return;
        }
        else if(cs.cameraSerial[0] == '\0')
        {
            last_empty_index = i;
        }
    }

    eeprom_read_block((void*)&cs, &conf_eep.camera, sizeof(camera_settings_t));
    strncpy(cs.cameraSerial, serial, 22);
    eeprom_write_block((const void*)&cs, &camera_settings_eep[last_empty_index], sizeof(camera_settings_t));
    settings_load_camera_index(last_empty_index);
}




