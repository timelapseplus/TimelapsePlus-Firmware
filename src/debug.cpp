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

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(char c)
{
    VirtualSerial_PutChar(c);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(uint8_t c)
{
    char buf[5];
    
    int_to_str((uint16_t)c, buf);
    VirtualSerial_PutString(buf);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(uint16_t n)
{
    char buf[6];

    int_to_str(n, buf);
    VirtualSerial_PutString(buf);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug(char *s)
{
    VirtualSerial_PutString(s);
}

/******************************************************************
 *
 *   debug
 *
 *
 ******************************************************************/

void debug_nl()
{
    VirtualSerial_PutChar('\r');
    VirtualSerial_PutChar('\n');
}

