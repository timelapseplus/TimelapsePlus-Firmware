/*
 *  LCD_Term.cpp
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

#include "LCD_Term.h"
#include "5110LCD.h"


extern LCD lcd;

uint8_t x, y;

void termInit()
{
    lcd.init();
    termClear();
}

void termPrintChar(char c)
{
    if(c == 13 || c == 10)
    {
        y += TERM_LINE_HEIGHT;
        x = 1;
    } else
    {
        x += lcd.writeCharTiny(x, y, c) + 1;
        if(x > TERM_WIDTH)
        {
            y += TERM_LINE_HEIGHT;
            x = 1;
        }
    }
    if(y > TERM_HEIGHT)
    {
        char j, i;
        for(i = 0; i < LCD_WIDTH; i++)
        {
            for(j = 0; j < LCD_HEIGHT - TERM_LINE_HEIGHT; j++)
            {
                if(lcd.getPixel(i, j + TERM_LINE_HEIGHT)) lcd.setPixel(i, j);
                else lcd.clearPixel(i, j);
            }
            for(j = LCD_HEIGHT - TERM_LINE_HEIGHT; j < LCD_HEIGHT; j++)
            {
                lcd.clearPixel(i, j);
            }
        }
        y -= TERM_LINE_HEIGHT;
    }
}

void termPrintByte(uint8_t c)
{
    char buf[3];
    buf[0] = c % 10;
    c -= buf[0]; c /= 10;
    buf[1] = c % 10;
    c -= buf[1]; c /= 10;
    buf[2] = c % 10;
    c -= buf[2]; c /= 10;
    if(buf[2]) termPrintChar(buf[2] + '0');
    if(buf[1]) termPrintChar(buf[1] + '0');
    termPrintChar(buf[0] + '0');
}


void termPrintStr(char *s)
{
    while (*(s) != 0)
    {
        termPrintChar(*s);
        s++;
    }
    lcd.update();
}

void termClear()
{
    lcd.cls();
    x = 1;
    y = 1;
    lcd.update();
}
