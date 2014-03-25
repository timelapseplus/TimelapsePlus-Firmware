/*
 *  timelapseplus.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#define VERSION 20140101


#define TYPE_DEFAULT 0
#define TYPE_PROTOTYPE 1
#define TYPE_PRODUCTION 2

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

int main();
void setup(void);

void message_notify(uint8_t id);

