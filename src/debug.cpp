/*
 *  debug.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#include "VirtualSerial.h"
#include "debug.h"
#include "timelapseplus.h"
#include "tlp_menu_functions.h"
#include "settings.h"
#include "tldefs.h"

extern settings conf;

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(char *s)
{
    if(conf.devMode == 0) return;
    VirtualSerial_PutString(s);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(char c)
{
    if(conf.devMode == 0) return;
    char buf[2];
    buf[0] = c;
    buf[1] = '\0';    
    debug((char*)buf);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(uint8_t c)
{
    if(conf.devMode == 0) return;
    char buf[5];
    
    int_to_str((uint16_t)c, buf);
    debug((char*)buf);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(uint16_t n)
{
    if(conf.devMode == 0) return;
    char buf[6];

    int_to_str(n, buf);
    debug((char*)buf);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(uint32_t n)
{
    if(conf.devMode == 0) return;

    uint8_t i = 9;
    char buf[10];
    buf[9] = '\0';
    while(n)
    {
        i--;
        buf[i] = (char)(n % 10) + '0';
        n -= n % 10;
        n /= 10;
    }
    while(i--) debug((char*)&buf[i]);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug_nl()
{
    if(conf.devMode == 0) return;
    debug((char*)STR("\r\n"));
}

