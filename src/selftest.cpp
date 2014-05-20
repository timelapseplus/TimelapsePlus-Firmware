/*
 *  selftest.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *	Licensed under GPLv3
 *
 */

#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "tldefs.h"
#include "LCD_Term.h"
#include "5110LCD.h"
#include "clock.h"
#include "button.h"
#include "Menu.h"
#include "hardware.h"
#include "shutter.h"
#include "IR.h"
#include "timelapseplus.h"
#include "VirtualSerial.h"
#include "TWI_Master.h"
#include "debug.h"
#include "bluetooth.h"
#include "settings.h"
#include "selftest.h"
#include "math.h"
#include "light.h"

extern shutter timer;
extern LCD lcd;
extern MENU menu;
extern Clock clock;
extern Button button;
extern BT bt;
extern IR ir;
extern Light light;
extern settings conf;

/******************************************************************
 *
 *   test_assert
 *
 *
 ******************************************************************/

int8_t test_assert(char test_case)
{
    if(test_case)
    {
        termPrintStrP(PSTR("   Passed\n"));
        return 1;
    } 
    
    termPrintStrP(PSTR("   Failed\n"));
    return 0;
}

/******************************************************************
 *
 *   run_tests
 *
 *
 ******************************************************************/

int8_t run_tests()
{
    int8_t pass = 1;

    wdt_reset();
    
    if(pass)
    {
        if(bt.present)
        {
            termPrintStrP(PSTR("Testing BlueTooth\n"));
            pass &= test_assert(bt.version() == 3);
        }
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Testing Battery\n"));
        pass &= test_assert(battery_read_raw() > 400);
        termPrintStrP(PSTR("Testing Charger\n"));
        pass &= test_assert(battery_status() > 0);
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Testing Timer\n"));
        clock.tare();
        _delay_ms(100);
        uint32_t ms = clock.eventMs();
        //termPrintByte((uint8_t) ms);
        pass &= test_assert(ms >= 80 && ms <= 120);
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Testing Light\n"));
        lcd.backlight(0);
        light.start();
        _delay_ms(100);
        light.setRangeAuto();
        _delay_ms(100);
        uint16_t light_off = light.readRaw();
        lcd.backlight(255);
        hardware_flashlight(1);
        _delay_ms(200);
        uint16_t light_on = light.readRaw();
        hardware_flashlight(0);
        light.stop();
        pass &= test_assert(light_off < light_on);
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Testing Shutter\n"));
        ENABLE_SHUTTER;
        ENABLE_MIRROR;
        ENABLE_AUX_PORT1;
        ENABLE_AUX_PORT2;
        uint8_t i = 0;      // this was uninitialized. I set it to 0 -- John
        
        while (i < 30)
        {
            i++;
            wdt_reset();
            _delay_ms(100);

            if(AUX_INPUT1) 
                break;
        }
        pass &= test_assert(AUX_INPUT1);
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press top l key\n"));
        pass &= test_assert(button.waitfor(FL_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press top r key\n"));
        pass &= test_assert(button.waitfor(FR_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press left key\n"));
        pass &= test_assert(button.waitfor(LEFT_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press right key\n"));
        pass &= test_assert(button.waitfor(RIGHT_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press up key\n"));
        pass &= test_assert(button.waitfor(UP_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        termPrintStrP(PSTR("Press down key\n"));
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        lcd.color(1);
        termPrintStrP(PSTR("LCD BL RED\n"));
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        lcd.color(0);
        termPrintStrP(PSTR("LCD BL WHITE\n"));
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        for(char i = 0; i < 48; i++)
        {
            lcd.drawLine(0, i, 83, i);
        }
        
        lcd.update();
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        lcd.cls();
        lcd.update();
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    
    wdt_reset();
    
    if(pass)
    {
        if(!bt.present)
        {
            termPrintStrP(PSTR("NO BLUETOOTH\n  CONFIRM\n"));
            pass &= test_assert(button.waitfor(DOWN_KEY));
        }
    }
    
    return pass;
}

float light_test_results[120]EEMEM;

/******************************************************************
 *
 *   lightTest
 *
 *
 ******************************************************************/

void lightTest()
{
    Light light;
    float result;

    termInit();

    lcd.backlight(0);

    termPrintStrP(PSTR("\nRunning light\nsensor test\n\n"));

    light.integrationStart(10);
    

    uint8_t i;

    clock.tare();

    for(i = 0; i < 120; i++)
    {
        timer.half();
        _delay_ms(500);
        timer.full();
        _delay_ms(50);
        timer.off();
        result = light.readIntegratedEv();
        eeprom_write_block((const void*)&result, &light_test_results[i], sizeof(result));
        //termPrintStrP(PSTR("Photo "));
        termPrintByte(i + 1);
        termPrintStrP(PSTR(" of 120: "));
        termPrintByte((int8_t)result);
        termPrintStrP(PSTR("\n"));
        
        while (clock.eventMs() < 60000) 
        {
            light.task();
            wdt_reset();
        }

        clock.tare();
    }
    termPrintStrP(PSTR("\nDone!\n"));
}

/******************************************************************
 *
 *   readLightTest
 *
 *
 ******************************************************************/

void readLightTest()
{
    float result;

    termInit();

    termPrintStrP(PSTR("\nReading light\nsensor test\ndata...\n\n"));

    uint8_t i;

    for(i = 0; i < 120; i++)
    {
        eeprom_read_block((void*)&result, &light_test_results[i], sizeof(result));

        DEBUG(i);
        DEBUG(STR(", "));
        DEBUG(result);
        DEBUG_NL();

        termPrintStrP(PSTR("Sent "));
        termPrintByte(i + 1);
        termPrintStrP(PSTR(" of 120\n"));

        VirtualSerial_Task();
        wdt_reset();
    }
    
    termPrintStrP(PSTR("\nDone!\n"));
}

