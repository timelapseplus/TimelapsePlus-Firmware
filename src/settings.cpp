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

settings conf_eep EEMEM;
volatile settings conf;

extern LCD lcd;
extern IR ir;
extern shutter timer;

/******************************************************************
 *
 *   settings_save
 *
 *
 ******************************************************************/

void settings_save()
{
    eeprom_write_block((const void*)&conf, &conf_eep, sizeof(settings));
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
    lcd.color(conf.lcdColor);
    ir.make = conf.cameraMake;
}

/******************************************************************
 *
 *   settings_update
 *
 *
 ******************************************************************/

void settings_update()
{
    eeprom_write_block((const void*)&conf, &conf_eep, sizeof(settings));
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
    
    if(eeprom_read_byte((const uint8_t*)&conf_eep) == 255 || conf.version != VERSION)
    {
        timer.setDefault();
        strcpy((char*)conf.sysName, "sys01");
        conf.warnTime = 2;
        conf.mirrorTime = 2;
        conf.cameraFPS = 34;
        conf.bulbMode = 0;
        conf.lcdColor = 0;
        conf.cameraMake = CANON;
        conf.version = VERSION;
        conf.lcdBacklightTime = 3;
        conf.sysOffTime = 12;
        conf.flashlightOffTime = 3;
        settings_save();
    }
    
    settings_load();
}

