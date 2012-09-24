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

void debug(char c)
{
	VirtualSerial_PutChar(c);
}

void debug(uint8_t c)
{
	char buf[3];
	buf[0] = c % 10;
	c -= buf[0]; c /= 10;
	buf[1] = c % 10;
	c -= buf[1]; c /= 10;
	buf[2] = c % 10;
	c -= buf[2]; c /= 10;
	if(buf[2]) VirtualSerial_PutChar(buf[2] + '0');
	if(buf[1])VirtualSerial_PutChar(buf[1] + '0');
	VirtualSerial_PutChar(buf[0] + '0');
}

void debug(uint16_t n)
{
	char buf[5];
	buf[0] = n % 10;
	n -= buf[0]; n /= 10;
	buf[1] = n % 10;
	n -= buf[1]; n /= 10;
	buf[2] = n % 10;
	n -= buf[2]; n /= 10;
	buf[3] = n % 10;
	n -= buf[3]; n /= 10;
	buf[4] = n % 10;
	n -= buf[5]; n /= 10;
	if(buf[4]) VirtualSerial_PutChar(buf[4] + '0');
	if(buf[3] || buf[4]) VirtualSerial_PutChar(buf[3] + '0');
	if(buf[2] || buf[3] || buf[4]) VirtualSerial_PutChar(buf[2] + '0');
	if(buf[1] || buf[2] || buf[3] || buf[4]) VirtualSerial_PutChar(buf[1] + '0');
	VirtualSerial_PutChar(buf[0] + '0');
}

void debug(char *s)
{
	VirtualSerial_PutString(s);
}

void debug_nl()
{
	VirtualSerial_PutChar('\r');
	VirtualSerial_PutChar('\n');
}