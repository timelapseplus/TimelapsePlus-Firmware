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
#include "notify.h"
#include "PTP.h"
//#include "thm-sample.h"

extern BT bt;
extern shutter timer;
extern MENU menu;
extern uint8_t battery_percent;
extern settings_t conf;
extern Notify notify;
extern PTP camera;

Remote::Remote()
{
	requestActive = 0;
}

uint8_t Remote::request(uint8_t id)
{
	requestActive = 1;
	return bt.sendDATA(id, REMOTE_TYPE_REQUEST, 0, 0);
}

uint8_t Remote::watch(uint8_t id)
{
	return bt.sendDATA(id, REMOTE_TYPE_NOTIFY_WATCH, 0, 0);
}

uint8_t Remote::unWatch(uint8_t id)
{
	return bt.sendDATA(id, REMOTE_TYPE_NOTIFY_UNWATCH, 0, 0);
}

uint8_t Remote::set(uint8_t id)
{
	return send(id, REMOTE_TYPE_SET);
}

uint8_t Remote::set(uint8_t id, uint8_t value)
{
	return bt.sendDATA(id, REMOTE_TYPE_SET, (void *) &value, sizeof(value));
}

uint8_t Remote::debugMessage(char *str)
{
	uint8_t len = 0;
	while(*(str + len) != '\0' && len < 255) len++;
	return bt.sendDATA(REMOTE_DEBUG, REMOTE_TYPE_SEND, (void *) str, len);
}

uint8_t Remote::send(uint8_t id, uint8_t type)
{
	switch(id)
	{
		case REMOTE_BATTERY:
			return bt.sendDATA(id, type, (void *) &battery_percent, sizeof(battery_percent));
		case REMOTE_STATUS:
			return bt.sendDATA(id, type, (void *) &timer.status, sizeof(timer.status));
		case REMOTE_PROGRAM:
			return bt.sendDATA(id, type, (void *) &timer.current, sizeof(timer.current));
		case REMOTE_MODEL:
		{
			uint8_t tmp = REMOTE_MODEL_TLP;
			return bt.sendDATA(id, type, (void *) &tmp, sizeof(uint8_t));
		}
		case REMOTE_FIRMWARE:
		{
			unsigned long version = VERSION;
			void *ptr = &version;
			return bt.sendDATA(id, type, ptr, sizeof(version));
		}
		//case REMOTE_BT_FW_VERSION:
		//{
		//	uint8_t btVersion = bt.version();
		//	return bt.sendDATA(id, type, (void *) &btVersion, sizeof(btVersion));
		//}
		case REMOTE_PROTOCOL_VERSION:
		{
			unsigned long remoteVersion = REMOTE_VERSION;
			void *ptr = &remoteVersion;
			return bt.sendDATA(id, type, ptr, sizeof(remoteVersion));
		}
		//case REMOTE_CAMERA_FPS:
		//	return bt.sendDATA(id, type, (void *) &conf.camera.cameraFPS, sizeof(conf.camera.cameraFPS));
		//case REMOTE_CAMERA_MAKE:
		//	return bt.sendDATA(id, type, (void *) &conf.camera.cameraMake, sizeof(conf.camera.cameraMake));
		case REMOTE_ISO:
		{
			uint8_t tmp = camera.iso();
			return bt.sendDATA(id, type, (void *) &tmp, sizeof(tmp));
		}
		case REMOTE_APERTURE:
		{
			uint8_t tmp = camera.aperture();
			return bt.sendDATA(id, type, (void *) &tmp, sizeof(tmp));
		}
		case REMOTE_SHUTTER:
		{
			uint8_t tmp = camera.shutter();
			return bt.sendDATA(id, type, (void *) &tmp, sizeof(tmp));
		}
		case REMOTE_VIDEO:
		{
			uint8_t tmp = camera.recording;
			return bt.sendDATA(id, type, (void *) &tmp, sizeof(tmp));
		}
		case REMOTE_LIVEVIEW:
		{
			uint8_t tmp = camera.modeLiveView;
			return bt.sendDATA(id, type, (void *) &tmp, sizeof(tmp));
		}
	/*	case REMOTE_THUMBNAIL:
		{
			menu.message(STR("Busy"));

			uint8_t ret = camera.getCurrentThumbStart();
			if(ret != PTP_RETURN_ERROR)
			{
				bt.sendDATA(REMOTE_THUMBNAIL_SIZE, type, (void *) &PTP_Bytes_Total, sizeof(PTP_Bytes_Total));
				bt.sendDATA(id, type, (void *) PTP_Buffer, PTP_Bytes_Received);
				while(ret == PTP_RETURN_DATA_REMAINING)
				{
					ret = camera.getCurrentThumbContinued();
					bt.sendDATA(id, type, (void *) PTP_Buffer, PTP_Bytes_Received);
				}
			}

			///////////////////////// DEMO Code ////////////////////////////
//			PTP_Bytes_Total = sizeof(thm);
//			bt.sendDATA(REMOTE_THUMBNAIL_SIZE, type, (void *) &PTP_Bytes_Total, sizeof(PTP_Bytes_Total));
//			uint16_t total_sent = 0, i;
//			while(total_sent < PTP_Bytes_Total)
//			{
//				for(i = 0; i < PTP_BUFFER_SIZE; i++)
//				{
//					PTP_Buffer[i] = pgm_read_byte(&thm[i + total_sent]);
//					if(total_sent + i >= PTP_Bytes_Total) break;
//				}
//				PTP_Bytes_Received = i;
//				total_sent += PTP_Bytes_Received;
//				if(PTP_Bytes_Received == 0) break;
//				bt.sendDATA(id, type, (void *) PTP_Buffer, PTP_Bytes_Received);
//				if(total_sent >= PTP_Bytes_Total) break;
//			}
//			/////////////////////////////////////////////////////////////////
			return 0;
		}*/
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
			notify.unWatch(&remote_notify); // stop all active notifications
			//DEBUG(STR("REMOTE::EVENT: Disconnected\r\n"));
			connected = 0;
			nmx = 0;
			break;

		case BT_EVENT_CONNECT:
			if(bt.state == BT_ST_CONNECTED)
			{
				connected = 1;
				//DEBUG(STR("REMOTE::EVENT: Connected\r\n"));
				request(REMOTE_MODEL);
			}
			else if(bt.state == BT_ST_CONNECTED_NMX)
			{
				DEBUG(PSTR("NMX CONNECTED"));
				nmx = 1;
			}
            else if(bt.state == BT_ST_CONNECTED_GM)
            {
                DEBUG(PSTR("GM CONNECTED"));
                nmx = 1;
            }
			break;

		case BT_EVENT_DATA:
			switch(bt.dataId)
			{
				case REMOTE_STATUS:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND && bt.dataSize == sizeof(timer_status)) memcpy(&status, bt.data, bt.dataSize);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_WATCH) notify.watch(REMOTE_STATUS, (void *)&timer.status, sizeof(timer.status), &remote_notify);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_UNWATCH) notify.unWatch(REMOTE_STATUS, &remote_notify);
					break;
				case REMOTE_PROGRAM:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND && bt.dataSize == sizeof(program)) memcpy(&current, bt.data, bt.dataSize);
					if(bt.dataType == REMOTE_TYPE_SET && bt.dataSize == sizeof(program))
					{
						memcpy((void*)&timer.current, bt.data, bt.dataSize);
						menu.refresh();
					}
					//if(bt.dataType == REMOTE_TYPE_NOTIFY_WATCH) notify.watch(REMOTE_PROGRAM, (void *)&timer.current, sizeof(timer.current), &remote_notify);
					//if(bt.dataType == REMOTE_TYPE_NOTIFY_UNWATCH) notify.unWatch(REMOTE_PROGRAM, &remote_notify);
					break;
				case REMOTE_BATTERY:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND) memcpy(&battery, bt.data, bt.dataSize);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_WATCH) notify.watch(REMOTE_BATTERY, (void *)&battery_percent, sizeof(battery_percent), &remote_notify);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_UNWATCH) notify.unWatch(REMOTE_BATTERY, &remote_notify);
					break;
				case REMOTE_START:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(timer.running ? REMOTE_START : REMOTE_STOP, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND) running = 1;
					if(bt.dataType == REMOTE_TYPE_SET) runHandler(FR_KEY, 1);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_WATCH) notify.watch(REMOTE_START, (void *)&timer.running, sizeof(timer.running), &remote_notify);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_UNWATCH) notify.unWatch(REMOTE_START, &remote_notify);
					break;
				case REMOTE_STOP:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(timer.running ? REMOTE_START : REMOTE_STOP, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND) running = 0;
					if(bt.dataType == REMOTE_TYPE_SET) timerStop(FR_KEY, 1);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_WATCH) notify.watch(REMOTE_START, (void *)&timer.running, sizeof(timer.running), &remote_notify);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_UNWATCH) notify.unWatch(REMOTE_START, &remote_notify);
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
				case REMOTE_MODEL:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(REMOTE_MODEL, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SEND) memcpy(&model, bt.data, bt.dataSize);
					break;
				case REMOTE_FIRMWARE:
				//case REMOTE_BT_FW_VERSION:
				case REMOTE_PROTOCOL_VERSION:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					break;
				//case REMOTE_CAMERA_FPS:
				//	if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
				//	if(bt.dataType == REMOTE_TYPE_NOTIFY_WATCH) notify.watch(REMOTE_CAMERA_FPS, (void *)&conf.camera.cameraFPS, sizeof(conf.camera.cameraFPS), &remote_notify);
				//	if(bt.dataType == REMOTE_TYPE_NOTIFY_UNWATCH) notify.unWatch(REMOTE_CAMERA_FPS, &remote_notify);
				//	break;
				//case REMOTE_CAMERA_MAKE:
				//	if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
				//	if(bt.dataType == REMOTE_TYPE_NOTIFY_WATCH) notify.watch(REMOTE_CAMERA_MAKE, (void *)&conf.camera.cameraMake, sizeof(conf.camera.cameraMake), &remote_notify);
				//	if(bt.dataType == REMOTE_TYPE_NOTIFY_UNWATCH) notify.unWatch(REMOTE_CAMERA_MAKE, &remote_notify);
				//	break;
#ifdef DEBUG_ENABLED
				case REMOTE_DEBUG:
					if(bt.dataType == REMOTE_TYPE_SEND)
					{
						bt.data[bt.dataSize] = 0;
						debug_remote(bt.data);
					}
					break;
#endif
				case REMOTE_ISO:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SET && bt.dataSize == sizeof(uint8_t))
					{
						uint8_t tmp;
						memcpy((void*)&tmp, bt.data, bt.dataSize);
						camera.setISO(tmp);
					}
					break;
				case REMOTE_APERTURE:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SET && bt.dataSize == sizeof(uint8_t))
					{
						uint8_t tmp;
						memcpy((void*)&tmp, bt.data, bt.dataSize);
						camera.setAperture(tmp);
					}
					break;
				case REMOTE_SHUTTER:
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_SET && bt.dataSize == sizeof(uint8_t))
					{
						uint8_t tmp;
						memcpy((void*)&tmp, bt.data, bt.dataSize);
						camera.setShutter(tmp);
					}
					break;
				//case REMOTE_THUMBNAIL:
				//	if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
				//	break;
				case REMOTE_VIDEO:
					if(bt.dataType == REMOTE_TYPE_SET && bt.dataSize == sizeof(uint8_t))
					{
						uint8_t tmp;
						memcpy((void*)&tmp, bt.data, bt.dataSize);
						if(tmp) camera.videoStart(); else camera.videoStop();
					}
					if(bt.dataType == REMOTE_TYPE_SEND && bt.dataSize == sizeof(uint8_t))
					{
						uint8_t tmp;
						memcpy((void*)&tmp, bt.data, bt.dataSize);
						if(tmp) recording = true; else recording = false;
					}
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_WATCH) notify.watch(REMOTE_VIDEO, (void *)&camera.recording, sizeof(camera.recording), &remote_notify);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_UNWATCH) notify.unWatch(REMOTE_VIDEO, &remote_notify);
					break;
				case REMOTE_LIVEVIEW:
					if(bt.dataType == REMOTE_TYPE_SET && bt.dataSize == sizeof(uint8_t))
					{
						uint8_t tmp;
						memcpy((void*)&tmp, bt.data, bt.dataSize);
						if(tmp) camera.liveView(tmp);
					}
					if(bt.dataType == REMOTE_TYPE_SEND && bt.dataSize == sizeof(uint8_t))
					{
						uint8_t tmp;
						memcpy((void*)&tmp, bt.data, bt.dataSize);
						if(tmp) modeLiveView = true; else modeLiveView = false;
					}
					if(bt.dataType == REMOTE_TYPE_REQUEST) send(bt.dataId, REMOTE_TYPE_SEND);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_WATCH) notify.watch(REMOTE_LIVEVIEW, (void *)&camera.modeLiveView, sizeof(camera.modeLiveView), &remote_notify);
					if(bt.dataType == REMOTE_TYPE_NOTIFY_UNWATCH) notify.unWatch(REMOTE_LIVEVIEW, &remote_notify);
					break;
				default:
					return;
			}
			bt.event = BT_EVENT_NULL;
			break;
	}
	requestActive = 0;
}

void remote_notify(uint8_t id)
{
	switch(id)
	{
		case REMOTE_START:
		case REMOTE_STOP:
			Remote::send(timer.running ? REMOTE_START : REMOTE_STOP, REMOTE_TYPE_SEND);
			break;
		default:
			Remote::send(id, REMOTE_TYPE_SEND);
			break;
	}
}
