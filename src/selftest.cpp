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
#include "camera.h"
#include "selftest.h"
#include "math.h"

extern shutter timer;
extern LCD lcd;
extern MENU menu;
extern Clock clock;
extern Button button;
extern BT bt;
extern IR ir;


int8_t test()
{
//	int8_t failed = 0;

    termInit();

    termPrintStr(STR("\nReady for Testing\n\nWaiting on PC\n"));

    for(;;)
    {
        VirtualSerial_Task();
        wdt_reset();

        if(VirtualSerial_connected == 1)
        {
            if(run_tests())
            {
                debug('1');
                termPrintStr(STR("Passed All Tests\n"));
                VirtualSerial_Reset();
                return 1;
            } else
            {
                debug('0');
                termPrintStr(STR("Failed Tests\n"));
                VirtualSerial_Reset();
                return 0;
            }
        }


        if(VirtualSerial_CharWaiting())
        {
            char c = VirtualSerial_GetChar();
            if(c == 'P')
            {
                termPrintStr(STR("Passed All Tests\n"));
                VirtualSerial_Reset();
                return 1;
            }
            if(c == 'E')
            {
//				failed = 1;		// set but never referenced
                termPrintStr(STR("Tests Failed\n"));
                for(;;) ;
            }
            if(c == 'R')
            {
                lcd.color(1);
                termPrintStr(STR("LCD BL RED\n"));
            }
            if(c == 'W')
            {
                lcd.color(0);
                termPrintStr(STR("LCD BL WHITE\n"));
            }
            if(c == 'D')
            {
                for(char i = 0; i < 48; i++)
                {
                    lcd.drawLine(0, i, 83, i);
                }
                lcd.update();
            }
            if(c == 'c')
            {
                lcd.cls();
                lcd.update();
            }
            if(c == 'T')
            {
                debug('E');
                termPrintStr(STR("USB Connected\n"));
            }
            if(c == 'O')
            {
                timer.off();
                termPrintStr(STR("Shutter Closed\n"));
            }
            if(c == 'F')
            {
                timer.full();
                termPrintStr(STR("Shutter Full\n"));
            }
            if(c == 'H')
            {
                timer.half();
                termPrintStr(STR("Shutter Half\n"));
            }
            if(c == 'C')
            {
                if(timer.cableIsConnected()) debug('1');
                else debug('0');
            }
            if(c == 'S')
            {
                if(run_tests())
                {
                    debug('1');
                    termPrintStr(STR("Passed All Tests\n"));
                    VirtualSerial_Reset();
                    return 1;
                } else
                {
                    debug('0');
                    termPrintStr(STR("Failed Tests\n"));
                    VirtualSerial_Reset();
                    return 0;
                }
            }
            if(c == 'B')
            {
                termPrintStr(STR("Checking charger\n"));
                debug((char)battery_status());
            }
            if(c == 'X') // RESET USB
            {
                VirtualSerial_Reset();
            }
        }
    }
    return 0;
}



int8_t test_assert(char test_case)
{
    if(test_case)
    {
        termPrintStr(STR("   Passed\n"));
        return 1;
    } else
    {
        termPrintStr(STR("   Failed\n"));
        return 0;
    }
}


int8_t run_tests()
{
    int8_t pass = 1;

    wdt_reset();
    if(pass)
    {
        if(bt.present)
        {
            termPrintStr(STR("Testing BlueTooth\n"));
            pass &= test_assert(bt.version() == 3);
        }
    }
    wdt_reset();
    if(pass)
    {
        termPrintStr(STR("Testing Battery\n"));
        pass &= test_assert(battery_read_raw() > 400);
        pass &= test_assert(battery_status() > 0);
    }
    wdt_reset();
    if(pass)
    {
        termPrintStr(STR("Testing Timer\n"));
        clock.tare();
        _delay_ms(100);
        uint32_t ms = clock.eventMs();
        pass &= test_assert(ms >= 90 && ms <= 110);
    }
    wdt_reset();
    if(pass)
    {
        termPrintStr(STR("Testing Shutter\n"));
        ENABLE_SHUTTER;
        ENABLE_MIRROR;
        ENABLE_AUX_INPUT;
        uint8_t i = 0;      // this was uninitialized. I set it to 0 -- John
        while (i < 30)
        {
            wdt_reset();
            _delay_ms(100);
            if(AUX_INPUT) break;
        }
        pass &= test_assert(AUX_INPUT);
    }
    wdt_reset();
    if(pass)
    {
        termPrintStr(STR("Press top l key\n"));
        pass &= test_assert(button.waitfor(FL_KEY));
    }
    wdt_reset();
    if(pass)
    {
        termPrintStr(STR("Press top r key\n"));
        pass &= test_assert(button.waitfor(FR_KEY));
    }
    wdt_reset();
    if(pass)
    {
        termPrintStr(STR("Press left key\n"));
        pass &= test_assert(button.waitfor(LEFT_KEY));
    }
    wdt_reset();
    if(pass)
    {
        termPrintStr(STR("Press right key\n"));
        pass &= test_assert(button.waitfor(RIGHT_KEY));
    }
    wdt_reset();
    if(pass)
    {
        termPrintStr(STR("Press up key\n"));
        pass &= test_assert(button.waitfor(UP_KEY));
    }
    wdt_reset();
    if(pass)
    {
        termPrintStr(STR("Press down key\n"));
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    wdt_reset();
    if(pass)
    {
        lcd.color(1);
        termPrintStr(STR("LCD BL RED\n"));
        pass &= test_assert(button.waitfor(DOWN_KEY));
    }
    wdt_reset();
    if(pass)
    {
        lcd.color(0);
        termPrintStr(STR("LCD BL WHITE\n"));
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
            termPrintStr(STR("NO BLUETOOTH\n  CONFIRM\n"));
            pass &= test_assert(button.waitfor(DOWN_KEY));
        }
    }
    return pass;
}

light_reading light_test_results[120]EEMEM;

void lightTest()
{
    light_reading result;

    termInit();

    lcd.backlight(0);

    termPrintStr(STR("\nRunning light\nsensor test\n\n"));

    uint8_t i;

    clock.tare();

    for(i = 0; i < 120; i++)
    {
        timer.half();
        _delay_ms(500);
        timer.full();
        _delay_ms(50);
        timer.off();
        hardware_readLightAll(&result);
        eeprom_write_block((const void*)&result, &light_test_results[i], sizeof(light_reading));
        termPrintStr(TEXT("Photo "));
        termPrintByte(i + 1);
        termPrintStr(TEXT(" of 120\n"));
        while (clock.eventMs() < 120000) wdt_reset();
        clock.tare();
    }
    termPrintStr(TEXT("\nDone!\n"));
}

void readLightTest()
{
    light_reading result;

    termInit();

    termPrintStr(TEXT("\nReading light\nsensor test\ndata...\n\n"));

    uint8_t i;

    for(i = 0; i < 120; i++)
    {
        eeprom_read_block((void*)&result, &light_test_results[i], sizeof(light_reading));

        debug(i);
        debug(STR(", "));
        debug((uint16_t)result.level1);
        debug(STR(", "));
        debug((uint16_t)result.level2);
        debug(STR(", "));
        debug((uint16_t)result.level3);
        debug_nl();

        termPrintStr(STR("Sent "));
        termPrintByte(i + 1);
        termPrintStr(STR(" of 120\n"));

        VirtualSerial_Task();
        wdt_reset();

    }
    termPrintStr(STR("\nDone!\n"));
}
