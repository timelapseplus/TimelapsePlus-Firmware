/*
 *  timelapseplus.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#define VERSION 20120108
//#define PROTOTYPE
//#define PRODUCTION

#define NOTIFY_CHARGE 1
#define NOTIFY_BT 2
#define NOTIFY_CAMERA 3

int main();
void setup(void);

void message_notify(uint8_t id);

