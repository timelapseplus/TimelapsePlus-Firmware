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
#include "shutter.h"
#include "bluetooth.h"
#include "remote.h"
#include "tldefs.h"

extern settings conf;
extern Remote remote;
extern BT bt;

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(char *s)
{
    if(conf.devMode == 0) return;
    if(VirtualSerial_connected)
        VirtualSerial_PutString(s);
//    else if(remote.connected && remote.model == REMOTE_MODEL_TLP)
//        remote.debug(s);
//    else if(bt.state == BT_ST_CONNECTED && remote.model == 0)
//       bt.send(s);
}

/******************************************************************
 *
 *   debug_remote
 *
 *
 ******************************************************************/

void debug_remote(char *s)
{
    if(VirtualSerial_connected)
    {
        VirtualSerial_PutString(s);
    }
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

void debug(float n)
{
    if(conf.devMode == 0) return;

    debug((int16_t)n);
    n -= (float)((int16_t)n);
    if(n < 0) n = 0 - n;
    debug('.');
    n *= 10.0;
    debug((uint8_t)n);
    n -= (float)((int16_t)n);
    n *= 10.0;
    debug((uint8_t)n);
    n -= (float)((int16_t)n);
    n *= 10.0;
    debug((uint8_t)n);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(int16_t n)
{
    if(conf.devMode == 0) return;
    char buf[7];

    buf[0] = '+';
    if(n < 0)
    {
        n = 0 - n;
        buf[0] = '-';
    }
    int_to_str(n, &buf[1]);
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
    buf[i] = '\0';
    while(i > 0 && n > 0)
    {
        i--;
        buf[i] = ((char)(n % 10)) + '0';
        n -= n % 10;
        n /= 10;
    }
    debug((char*)&buf[i]);
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

