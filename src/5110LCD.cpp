/*
 *  5110LCD.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>
#include "5110LCD.h"
#include "fonts.h"
#include "hardware.h"

/******************************************************************
 *
 *   LCD::LCD()
 *
 *
 ******************************************************************/

LCD::LCD()
{
}

/******************************************************************
 *
 *   LCD::setPixel
 *
 *
 ******************************************************************/

void LCD::setPixel(unsigned char x, unsigned char y)
{
    if(x <= LCD_WIDTH - 1 && y <= LCD_HEIGHT - 1) screen[x][y >> 3] |= 1 << (y % 8);
}

/******************************************************************
 *
 *   LCD::clearPixel
 *
 *
 ******************************************************************/

void LCD::clearPixel(unsigned char x, unsigned char y)
{
    if(x <= LCD_WIDTH - 1 && y <= LCD_HEIGHT - 1) screen[x][y >> 3] &= ~(1 << (y % 8));
}

/******************************************************************
 *
 *   LCD::xorPixel
 *
 *
 ******************************************************************/

void LCD::xorPixel(unsigned char x, unsigned char y)
{
    if(getPixel(x, y))
    {
        clearPixel(x, y);
    } else
    {
        setPixel(x, y);
    }
}

/******************************************************************
 *
 *   LCD::getPixel
 *
 *
 ******************************************************************/

unsigned char LCD::getPixel(unsigned char x, unsigned char y)
{
    if(screen[x][y >> 3] & (1 << (y % 8))) return 1;
    else return 0;
}

/******************************************************************
 *
 *   LCD::writeString
 *
 *
 ******************************************************************/

void LCD::writeString(unsigned char x, unsigned char y, char *s)
{
    while (*(s) != 0)
    {
        writeChar(x, y, *s);
        x += 6;
        s++;
    }
}

/******************************************************************
 *
 *   LCD::writeUint
 *
 *
 ******************************************************************/

void LCD::writeUint(unsigned char x, unsigned char y, unsigned int n)
{

    if(n > 9) x += 6;
    if(n > 99) x += 6;
    if(n > 999) x += 6;
    if(n > 9999) x += 6;

    do
    {
        writeChar(x, y, '0' + (n % 10));
        n -= n % 10;
        n /= 10;
        x -= 6;
    }
    while (n > 0);

}

/******************************************************************
 *
 *   LCD::writeNumber
 *
 *
 ******************************************************************/

unsigned char LCD::writeNumber(unsigned char x, unsigned char y, unsigned int n, unsigned char mode, unsigned char justification)
{
    if(justification == 'R')
    {
        x -= 6;
    } else
    {
        if(n > 9) x += 6;
        if(n > 99) x += 6;
        if(n > 999) x += 6;
        if(n > 9999) x += 6;
    }

    justification = 0;

    switch(mode)
    {
    case 'U': // Unsigned int //
        do
        {
            writeChar(x, y, '0' + (n % 10));
            n -= n % 10;
            n /= 10;
            x -= 6;
            justification++;
        }
        while (n > 0);
        break;
        
    case 'T': // Time (hh:mm:ss) //
        {
            char b, p;
            unsigned int c;

            // seconds //
            c = n % 60;
            n -= c; n /= 60;
            b = c % 10;
            writeChar(x, y, '0' + b); x -= 6; justification++;
            c -= b; c /= 10;
            b = c; if(b) p = justification;
            writeChar(x, y, '0' + b); x -= 6; justification++;

            writeChar(x, y, ':'); x -= 6; justification++;
            p = justification;

            // minutes //
            c = n % 60;
            n -= c; n /= 60;
            b = c % 10; if(b) p = justification;
            writeChar(x, y, '0' + b); x -= 6; justification++;
            c -= b; c /= 10;
            b = c; if(b) p = justification;
            writeChar(x, y, '0' + b); x -= 6; justification++;

            writeChar(x, y, ':'); x -= 6; justification++;

            // hours //
            c = n % 60;
            n -= c; n /= 60;
            b = c % 10; if(b) p = justification;
            writeChar(x, y, '0' + b); x -= 6; justification++;
            if(p)
            {
                eraseBox(x, y, x + (justification - p) * 6, y + 7);
                justification -= p;
            }
            break;
        }
        
    case 'F': // Fractional time (mm:ss.s) //
        {
            char b, p;
            unsigned int c;

            // seconds //
            c = n % 600;
            n -= c; n /= 600;
            b = c % 10;
            writeChar(x, y, '0' + b); x -= 6; justification++;

            writeChar(x, y, '.'); x -= 6; justification++;
            p = justification;

            c -= b; c /= 10;
            b = c % 10; if(b) p = justification;
            writeChar(x, y, '0' + b); x -= 6; justification++;

            c -= b; c /= 10;
            b = c; if(b) p = justification;
            writeChar(x, y, '0' + b); x -= 6; justification++;

            writeChar(x, y, ':'); x -= 6; justification++;

            // minutes //
            c = n % 600;
            b = c % 10; if(b) p = justification;
            writeChar(x, y, '0' + b); x -= 6; justification++;

            c -= b; c /= 10;
            b = c; if(b) p = justification;
            writeChar(x, y, '0' + b); x -= 6; justification++;
            if(p)
            {
                eraseBox(x, y, x + (justification - p) * 6, y + 7);
                justification -= p;
            }
            break;
        }
    }

    return justification; // number of chars printed //
}

/******************************************************************
 *
 *   LCD::writeStringBig
 *
 *
 ******************************************************************/

void LCD::writeStringBig(unsigned char x, unsigned char y, char *s)
{
    while (*(s) != 0)
    {
        writeCharBig(x, y, *s);
        x += 16;
        s++;
    }
}

/******************************************************************
 *
 *   LCD::writeStringTiny
 *
 *
 ******************************************************************/

void LCD::writeStringTiny(unsigned char x, unsigned char y, char *s)
{
    while (*(s) != 0)
    {
        x += writeCharTiny(x, y, *s) + 1;
        s++;
    }
}

/******************************************************************
 *
 *   LCD::measureStringTiny
 *
 *
 ******************************************************************/

char LCD::measureStringTiny(char *s)
{
    unsigned char *pFont;
    pFont = (unsigned char*)font4_5;
    char l = 0, c;

    while (*s != 0)
    {
        if(*s == ' ')
        {
            l += 2;
        } else if(*s == '.')
        {
            l += 0;
        } else if(*s >= '0' && *s <= '9')
        {
            l += 3;
        } else
        {
            c = *s;
            if(c > 90) c -= 32;
            c -= 'A';
            l += pgm_read_byte(pFont + c * 6) + 1;
        }
        s++;
    }
    return l;
}

/******************************************************************
 *
 *   LCD::writeCharTiny
 *
 *
 ******************************************************************/

unsigned char LCD::writeCharTiny(unsigned char x, unsigned char y, unsigned char c)
{
    uint8_t line, b, len;
    uint8_t *pFont;
    uint8_t ch;

    pFont = (unsigned char*)font4_5;
    if(c == ' ') return 2;
    if(c == '.') return 0;

    if(c >= '0' && c <= '9')
    {
        c = (c - '0') + ('Z' - 'A') + 1;
    } else
    {
        if(c > 90) c -= 32;
        c -= 'A';
        if(c > 'Z') return 0;
    }

    line = 0; // todo BUG -- line used here but not initialized. Setting it to 0 for now - John
    
    len = pgm_read_byte(pFont + c * 6 + line);
    for(line = 0; line < len; line++)
    {
        ch = pgm_read_byte(pFont + c * 6 + line + 1);
        for(b = 0; b < 5; b++)
        {
            if(ch & 1 << b) setPixel(x + line, y + b);
        }
    }
    return len;
}

/******************************************************************
 *
 *   LCD::writeChar
 *
 *
 ******************************************************************/

void LCD::writeChar(unsigned char x, unsigned char y, unsigned char c)
{
    unsigned char line, b;
    unsigned char *pFont;
    unsigned char ch;

    pFont = (unsigned char*)font6_8;
    c -= 32;

    for(line = 0; line < 6; line++)
    {
        ch = pgm_read_byte(pFont + c * 6 + line);
        for(b = 0; b < 8; b++)
        {
            if(ch & 1 << b) setPixel(x + line, y + b);
        }
    }
}

/******************************************************************
 *
 *   LCD::writeCharBig
 * 
 *   write char in big font
 *
 ******************************************************************/

void LCD::writeCharBig(unsigned char x, unsigned char y, unsigned char c)
{
    unsigned char i, j, b;
    unsigned char *pFont;
    unsigned char ch_dat;

    pFont = (unsigned char*)big_number;

    if(c == '.') c = 10;
    else if(c == '+') c = 11;
    else if(c == '-') c = 12;
    else c = c & 0x0f;


    for(i = 0; i < 3; i++)
    {
        for(j = 0; j < 16; j++)
        {
            ch_dat = pgm_read_byte(pFont + c * 48 + (i) * 16 + (j));
            for(b = 0; b < 8; b++)
            {
                if(ch_dat & 1 << b) setPixel(x + j, y + i * 8 + b);
            }
        }
    }
}

/******************************************************************
 *
 *   LCD::drawBox
 *
 *
 ******************************************************************/

void LCD::drawBox(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
    drawLine(x1, y1, x2, y1);
    drawLine(x2, y1, x2, y2);
    drawLine(x1, y1, x1, y2);
    drawLine(x1, y2, x2, y2);
}

/******************************************************************
 *
 *   LCD::pixel
 *
 *
 ******************************************************************/

void LCD::pixel(unsigned char x, unsigned char y, int8_t color)
{
    switch(color)
    {
    case 1:
        setPixel(x, y);
        break;
    case 0:
        clearPixel(x, y);
        break;
    case -1:
        xorPixel(x, y);
        break;
    }
}

/******************************************************************
 *
 *   LCD::drawLine
 *
 *
 ******************************************************************/

void LCD::drawLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
    char n, sgndeltax, sgndeltay, deltaxabs, deltayabs, x, y;

    x2 = x2 - x1;
    y2 = y2 - y1;
    deltaxabs = abs(x2);
    deltayabs = abs(y2);
    sgndeltax = sgn(x2);
    sgndeltay = sgn(y2);
    x = deltayabs >> 1;
    y = deltaxabs >> 1;

    setPixel(x1, y1);

    if(deltaxabs >= deltayabs)
    {
        for(n = 0; n < deltaxabs; n++)
        {
            y += deltayabs;
            if(y >= deltaxabs)
            {
                y -= deltaxabs;
                y1 += sgndeltay;
            }
            x1 += sgndeltax;
            setPixel(x1, y1);
        }
    } else
    {
        for(n = 0; n < deltayabs; n++)
        {
            x += deltaxabs;
            if(x >= deltayabs)
            {
                x -= deltayabs;
                x1 += sgndeltax;
            }
            y1 += sgndeltay;
            setPixel(x1, y1);
        }
    }
}

/******************************************************************
 *
 *   LCD::sgn
 *
 *
 ******************************************************************/

char LCD::sgn(char x)
{
    if(x > 0) return 1;
    if(x < 0) return -1;
    return 0;
}

/******************************************************************
 *
 *   LCD::drawHighlight
 *
 *
 ******************************************************************/

void LCD::drawHighlight(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
    unsigned char i, j;
    for(j = x1; j <= x2; j++)
    {
        for(i = y1; i <= y2; i++)
        {
            xorPixel(j, i);
        }
    }
}

/******************************************************************
 *
 *   LCD::eraseBox
 *
 *
 ******************************************************************/

void LCD::eraseBox(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2)
{
    unsigned char i, j;
    for(j = x1; j <= x2; j++)
    {
        for(i = y1; i <= y2; i++)
        {
            clearPixel(j, i);
        }
    }
}

/******************************************************************
 *
 *   LCD::drawBMP
 *
 *
 ******************************************************************/

void LCD::drawBMP(unsigned char x, unsigned char y, unsigned char *pBMP)
{
    unsigned char i, j, y2, b, w, h;
    unsigned char ch_dat;

    w = pgm_read_byte(pBMP++);
    h = pgm_read_byte(pBMP++);

    if(x == 0 && y == 0 && w == LCD_WIDTH && h == LCD_HEIGHT)
    {
        for(y = 0; y < (LCD_HEIGHT >> 3); y++)
        {
            for(x = 0; x < LCD_WIDTH; x++)
            {
                screen[x][y] = pgm_read_byte(pBMP++);
            }
        }
    } else
    {
        for(j = 0; j < (h >> 3); j++)
        {
            for(i = 0; i < w; i++)
            {
                ch_dat = pgm_read_byte(pBMP++);
                for(b = 0; b < 8; b++)
                {
                    y2 = y + (j * 8) + b;
                    if(y2 <= h + y)
                    {
                        if(ch_dat & 1 << b) setPixel(x + i, y2);
                        else clearPixel(x + i, y2);
                    }
                }
            }
        }
    }
}

/******************************************************************
 *
 *   LCD::drawCircle
 *
 *
 ******************************************************************/

void LCD::drawCircle(unsigned char x, unsigned char y, unsigned char r)
{
    char x1, y1, p;

    setPixel(x - r, y);
    setPixel(x, y - r);
    setPixel(x + r, y);
    setPixel(x, y + r);

    x1 = 0; y1 = r; p = 3 - (2 * r);
    while (x1 <= y1)
    {
        x1 = x1 + 1;
        if(p < 0)
        {
            p = p + (4 * x1) + 6;
        } else
        {
            y1 = y1 - 1;
            p = p + (4 * (x1 - y1)) + 10;
        }
        setPixel(x + x1, y + y1);
        setPixel(x - x1, y - y1);
        setPixel(x + x1, y - y1);
        setPixel(x - x1, y + y1);
        setPixel(x + y1, y + x1);
        setPixel(x - y1, y - x1);
        setPixel(x + y1, y - x1);
        setPixel(x - y1, y + x1);
    }
}

/******************************************************************
 *
 *   LCD::swapBits
 *
 *
 ******************************************************************/

unsigned char LCD::swapBits(unsigned char b)
{
    unsigned long l;
    l = (unsigned long)b;
    l = ((l * 0x0802 & 0x22110) | (l * 0x8020 & 0x88440)) * 0x10101 >> 16;
    b = (unsigned char)l;
    return b;
}

/******************************************************************
 *
 *   LCD::cls
 * 
 *   clear screen
 *
 ******************************************************************/

void LCD::cls()
{
    unsigned char i, j;
    for(j = 0; j < 6; j++)
    {
        for(i = 0; i < 84; i++)
        {
            screen[i][j] = 0;
        }
    }
}

/*void LCD::flash(char *message, unsigned int duration)
{

}
*/

/******************************************************************
 *
 *   LCD::update
 *
 *
 ******************************************************************/

void LCD::update()
{
    uint8_t i, j, b;
    setXY(0, 0);
#ifdef LCD_UPSIDEDOWN
    for(j = (LCD_HEIGHT >> 3) - 1; j >= 0; j--)
    {
        for(i = LCD_WIDTH - 1; i >= 0; i--)
        {
            b = swapBits(screen[i][j]);
//      if(invert) b = ~b;
            writeByte(b, 1);
        }
    }
#else
    for(j = 0; j < (LCD_HEIGHT >> 3); j++)
    {
        for(i = 0; i < LCD_WIDTH; i++)
        {
            b = screen[i][j];
//      if(invert) b = ~b;
            writeByte(b, 1);
        }
    }
#endif
}

/************************************************************************************
 * 			LCD Hardware Functions
 *************************************************************************************/

/******************************************************************
 *
 *   LCD::init
 *
 *
 ******************************************************************/

void LCD::init(void)
{
    // LCD_RST = 0;
    setOut(SPI_CS);
    setOut(SPI_MOSI);
    setOut(SPI_SCK);
    setOut(LCD_VCC);
    setOut(LCD_BL);
    setOut(LCD_BL_DIM);
    setOut(LCD_DC);
    setOut(LCD_RST);

#ifdef LCD_RED
    setOut(LCD_RED);
#endif

    setHigh(LCD_VCC);

    color(blRed);

    _delay_us(10);

    setLow(LCD_RST);

    _delay_us(1);

    setHigh(LCD_RST);

    SPCR = 0x51;   // enable SPI master, fosc/16 = 1MH

    writeByte(0x21, 0);
    writeByte(0xc0, 0);
    writeByte(0x06, 0);
    writeByte(0x13, 0);
    writeByte(0x20, 0);
    clear();
    writeByte(0x0c, 0);
    backlight(255);
}

/******************************************************************
 *
 *   LCD::writeByte
 *
 *
 ******************************************************************/

void LCD::writeByte(unsigned char dat, unsigned char dat_type)
{
    setLow(SPI_CS);
    if(dat_type == 0) setLow(LCD_DC);
    else setHigh(LCD_DC);

    SPDR = dat;

    while ((SPSR & 0x80) == 0);

    setHigh(SPI_CS);

}

/******************************************************************
 *
 *   LCD::setXY
 *
 *
 ******************************************************************/

void LCD::setXY(unsigned char X, unsigned char Y)
{
    writeByte(0x40 | Y, 0);       // column
    writeByte(0x80 | X, 0);          // row
}

/******************************************************************
 *
 *   LCD::clear
 *
 *
 ******************************************************************/

void LCD::clear(void)
{
    unsigned int i;

    writeByte(0x0c, 0);
    writeByte(0x80, 0);

    for(i = 0; i < 504; i++) writeByte(0, 1);
}

/******************************************************************
 *
 *   LCD::off
 *
 *
 ******************************************************************/

void LCD::off()
{
    setLow(SPI_CS);
    setLow(SPI_MOSI);
    setLow(SPI_SCK);
    setLow(LCD_VCC);
    setLow(LCD_BL);
    setLow(LCD_DC);
    setLow(LCD_RST);
#ifdef LCD_RED
    setLow(LCD_RED);
#endif
}

/******************************************************************
 *
 *   LCD::color
 *
 *
 ******************************************************************/

void LCD::color(int8_t red)
{
#ifndef LCD_RED
    if(red == 1)
    {
        blRed = 1;
        setLow(LCD_BL);
        setHigh(LCD_BL_DIM);
    } else if(red == -1)
    {
        setLow(LCD_BL);
        setLow(LCD_BL_DIM);
    } else
    {
        blRed = 0;
        setHigh(LCD_BL);
        setLow(LCD_BL_DIM);
    }
#else
    if(red == 1)
    {
        blRed = 1;
        setLow(LCD_BL);
        setHigh(LCD_RED);
        setLow(LCD_BL_DIM);
    } else if(red == -1)
    {
        setLow(LCD_BL);
        setLow(LCD_RED);
        setLow(LCD_BL_DIM);
    } else
    {
        blRed = 0;
        setHigh(LCD_BL);
        setLow(LCD_RED);
        setLow(LCD_BL_DIM);
    }
#endif
}

/******************************************************************
 *
 *   LCD::backlight
 *
 *
 ******************************************************************/

void LCD::backlight(unsigned char amount)
{
/*  unsigned char i;
  if(amount < blVal)
  {
    for(i = blVal; i > amount; i/=2)
    {
      //analogWrite(LCD_BL_DIM, i);
      _delay_ms(20);
    }
  }
  else
  {
    for(i = blVal; i < amount; i++)
    {
      //analogWrite(LCD_BL_DIM, i);
      _delay_us(300);
    }
  }
  //analogWrite(LCD_BL_DIM, amount);
*/
    blVal = amount;
    if(amount > 0) color(blRed);
    else color(-1);
}

/******************************************************************
 *
 *   LCD::getBacklight
 *
 *
 ******************************************************************/

unsigned char LCD::getBacklight()
{
    return blVal;
}


