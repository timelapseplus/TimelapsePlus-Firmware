#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <LUFA/Drivers/Peripheral/Serial.h>
#include "tldefs.h"
#include "5110LCD.h"
#include "AVRS_logo.h"
#include "clock.h"
#include "button.h"
#include "Menu.h"
#include "hardware.h"
#include "shutter.h"
#include "IR.h"
#include "timelapseplus.h"
#include "VirtualSerial.h"
#include "TWI_Master.h"
#include "LCD_Term.h"
#include "debug.h"
#include "bluetooth.h"
#include "settings.h"
#include "PTP_Driver.h"
#include "math.h"
#include "remote.h"
#include "tlp_menu_functions.h"

extern BT bt;
extern shutter timer;
extern MENU menu;
extern uint8_t battery_percent;
extern settings conf;

Remote::Remote()
{
	requestActive = 0;
}

uint8_t Remote::request(uint8_t id)
{
	requestActive = 1;
	return bt.sendDATA(id, REMOTE_TYPE_REQUEST, 0, 0);
}

uint8_t Remote::set(uint8_t id)
{
	return send(id, REMOTE_TYPE_SET);
}

uint8_t Remote::send(uint8_t id, uint8_t type)
{
	switch(id)
	{
		case REMOTE_BATTERY:
			if(type < REMOTE_TYPE_NOTIFY_SET) return bt.sendDATA(id, type, (void *) &battery_percent, sizeof(uint8_t));
		case REMOTE_STATUS:
			return bt.sendDATA(id, type, (void *) &timer.status, sizeof(timer_status));
		case REMOTE_PROGRAM:
			return bt.sendDATA(id, type, (void *) &timer.current, sizeof(program));
		case REMOTE_FIRMWARE:
		{
			unsigned long version = VERSION;
			void *ptr = &version;
			return bt.sendDATA(id, type, ptr, sizeof(version));
		}
		case REMOTE_BT_FW_VERSION:
		{
			uint8_t btVersion = bt.version();
			return bt.sendDATA(id, type, (void *) &btVersion, sizeof(btVersion));
		}
		case REMOTE_PROTOCOL_VERSION:
		{
			unsigned long remoteVersion = REMOTE_VERSION;
			void *ptr = &remoteVersion;
			return bt.sendDATA(id, type, ptr, sizeof(remoteVersion));
		}
		case REMOTE_CAMERA_FPS:
			return bt.sendDATA(id, type, (void *) &conf.cameraFPS, sizeof(conf.cameraFPS));
		case REMOTE_CAMERA_MAKE:
			return bt.sendDATA(id, type, (void *) &conf.cameraMake, sizeof(conf.cameraMake));
		default:
			return bt.sendDATA(id, type, 0, 0);
	}
	return 0;
}

void Remote::event()
{
	switch(bt.event)
	{
		case BT_EVENT_DISCONNECT:
			connected = 0;
			notifyBattery = 0;
			notifyCameraFPS = 0;
			notifyCameraMake = 0;
			break;

		case BT_EVENT_CONNECT:
			connected = 1;
			break;

		case BT_EVENT_DATA:
			switch(bt.dataId)
			{
				case REMOTE_STATUS:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND) memcpy(&status, bt.data, bt.dataSize);
					if(bt.dataType == REMOTE_TYPE_SET) memcpy(&status, bt.data, bt.dataSize);
					break;
				case REMOTE_PROGRAM:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND) memcpy(&current, bt.data, bt.dataSize);
					if(bt.dataType == REMOTE_TYPE_SET)
					{
						memcpy((void*)&timer.current, bt.data, bt.dataSize);
						menu.refresh();
					}
					break;
				case REMOTE_BATTERY:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND) memcpy(&battery, bt.data, 1);
					if(bt.dataType == REMOTE_TYPE_SET) memcpy(&battery, bt.data, 1);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_SET) notifyBattery = 1;
					if(bt.dataType == REMOTE_TYPE_NOTIFY_UNSET) notifyBattery = 0;
					break;
				case REMOTE_START:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(timer.running ? REMOTE_START : REMOTE_STOP, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND) running = 1;
					if(bt.dataType == REMOTE_TYPE_SET) runHandler(FR_KEY, 1);
					break;
				case REMOTE_STOP:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(timer.running ? REMOTE_START : REMOTE_STOP, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND) running = 0;
					if(bt.dataType == REMOTE_TYPE_SET) timerStop(FR_KEY, 1);
					break;
				case REMOTE_BULB_START:
					if(bt.dataType == REMOTE_TYPE_SET) timer.bulbStart();
					break;
				case REMOTE_BULB_END:
					if(bt.dataType == REMOTE_TYPE_SET) timer.bulbEnd();
					break;
				case REMOTE_CAPTURE:
					if(bt.dataType == REMOTE_TYPE_SET) timer.capture();
					break;
				case REMOTE_FIRMWARE:
				case REMOTE_BT_FW_VERSION:
				case REMOTE_PROTOCOL_VERSION:
					if(bt.dataType == REMOTE_TYPE_REQUEST)
send(bt.dataId, REMOTE_TYPE_SEND);
					break;
				case REMOTE_CAMERA_FPS:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_SET) notifyCameraFPS = 1;
					if(bt.dataType == REMOTE_TYPE_NOTIFY_UNSET) notifyCameraFPS = 0;
					break;
				case REMOTE_CAMERA_MAKE:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_SET) notifyCameraMake = 1;
					if(bt.dataType == REMOTE_TYPE_NOTIFY_UNSET) notifyCameraMake = 0;
					break;
				default:
					return;
			}
			bt.event = BT_EVENT_NULL;
			break;
	}
	requestActive = 0;
}


