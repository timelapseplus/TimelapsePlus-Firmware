/*
 *  5110LCD.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#define LCD_WIDTH 84
#define LCD_HEIGHT 48
//#define LCD_UPSIDEDOWN

#ifndef NOKIA_BW_LCD
#define NOKIA_BW_LCD


class LCD
{
public:

    LCD();
    void setPixel(unsigned char x, unsigned char y);
    unsigned char getPixel(unsigned char x, unsigned char y);
    void clearPixel(unsigned char x, unsigned char y);
    void xorPixel(unsigned char x, unsigned char y);
    void pixel(unsigned char x, unsigned char y, int8_t color);
    void writeString(unsigned char x, unsigned char y, char *s);
    void writeString(unsigned char x, unsigned char y, const char *s);
    void writeChar(unsigned char x, unsigned char y, unsigned char c);
    unsigned char writeCharTiny(unsigned char x, unsigned char y, unsigned char c);
    void writeUint(unsigned char x, unsigned char y, unsigned int n);
    unsigned char writeNumber(unsigned char x, unsigned char y, unsigned int n, unsigned char mode, unsigned char justification, bool minus);
    void writeStringBig(unsigned char x, unsigned char y, char *s);
    void writeStringBig(unsigned char x, unsigned char y, const char *s);
    char writeStringTiny(unsigned char x, unsigned char y, char *s);
    char writeStringTiny(unsigned char x, unsigned char y, const char *s);
    char measureStringTiny(char *s);
    char measureStringTiny(const char *s);
    char measureCharTiny(char c);
    void writeCharBig(unsigned char x, unsigned char y, unsigned char c);
    //void drawBMP(unsigned char x, unsigned char y, unsigned char *pBMP);
    void update();
    void init(uint8_t contrast);
    void off(void);
    void cls(void);
    void color(int8_t red);
    void backlight(unsigned char amount);
    unsigned char getBacklight();
    void drawBox(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
    void drawLine(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
    void drawHighlight(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
    void eraseBox(unsigned char x1, unsigned char y1, unsigned char x2, unsigned char y2);
    //void drawCircle(unsigned char x, unsigned char y, unsigned char r);

    unsigned char screen[LCD_WIDTH][LCD_HEIGHT>>3];

    uint8_t disableUpdate;

private:

    void writeByte(unsigned char dat, unsigned char dat_type);
    void setXY(unsigned char X, unsigned char Y);
    void clear(void);
    unsigned char swapBits(unsigned char b);
    char sgn(char x);
    unsigned char blVal;
    unsigned char blRed;
//                unsigned char invert;
};

#endif
