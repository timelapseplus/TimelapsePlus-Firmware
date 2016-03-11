/*
 *  timelapseplus.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#define VERSION 20160310

#define TYPE_DEFAULT 0
#define TYPE_PROTOTYPE 1
#define TYPE_PRODUCTION 2

#undef USB_SERIAL_COMMANDS_ENABLED

#ifndef SYSTEM_TYPE
 #define SYSTEM_TYPE TYPE_DEFAULT
#endif

#if SYSTEM_TYPE==TYPE_PROTOTYPE
	#define PROTOTYPE
#elif SYSTEM_TYPE==TYPE_PRODUCTION
	#define PRODUCTION
#endif

#define NOTIFY_CHARGE 1
#define NOTIFY_BT 2
#define NOTIFY_CAMERA 3
#define NOTIFY_NMX 4
 
int main();
void setup(void);

void message_notify(uint8_t id);

