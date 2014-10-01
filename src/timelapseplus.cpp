/*
 *  timelapseplus.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

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
#include "debug.h"
#include "bluetooth.h"
#include "settings.h"
#include "PTP_Driver.h"
#include "math.h"
#include "remote.h"
#include "tlp_menu_functions.h"
#include "notify.h"
#include "PTP.h"
#include "light.h"

#ifdef PRODUCTION
#include "LCD_Term.h"
#include "selftest.h"
#endif

char system_tested EEMEM;

extern volatile uint8_t showGap;
extern volatile uint8_t timerNotRunning;
extern volatile uint8_t modeHDR;
extern volatile uint8_t modeTimelapse;
extern volatile uint8_t modeStandard;
extern volatile uint8_t modeStandardExp;
extern volatile uint8_t modeStandardExpNikon;
extern volatile uint8_t modeStandardExpArb;
extern volatile uint8_t modeRamp;
extern volatile uint8_t modeNoRamp;
extern volatile uint8_t modeRampKeyAdd;
extern volatile uint8_t modeRampKeyDel;
extern volatile uint8_t modeBulb;
extern volatile uint8_t bulb1;
extern volatile uint8_t bulb2;
extern volatile uint8_t bulb3;
extern volatile uint8_t bulb4;
extern volatile uint8_t bulb5;
extern volatile uint8_t bulb6;
extern volatile uint8_t bulb7;
extern volatile uint8_t bulb8;
extern volatile uint8_t bulb9;
extern volatile uint8_t showRemoteStart;
extern volatile uint8_t showRemoteInfo;
extern volatile uint8_t brampKeyframe;
extern volatile uint8_t brampGuided;
extern volatile uint8_t brampAuto;
extern volatile uint8_t brampNotAuto;
extern volatile uint8_t brampNotGuided;
extern volatile uint8_t showIntervalMaxMin;
extern volatile uint8_t modeRampNormal;
extern volatile uint8_t modeRampExtended;
extern volatile uint8_t rampISO;
extern volatile uint8_t rampAperture;
extern volatile uint8_t rampTargetCustom;
extern volatile uint8_t cameraMakeNikon;

volatile uint8_t connectUSBcamera = 0;

uint8_t battery_percent, charge_status, USBmode = 0;

extern settings_t conf;

extern uint8_t Camera_Connected;

shutter timer = shutter();
LCD lcd = LCD();
MENU menu = MENU();
Clock clock = Clock();
Button button = Button();
BT bt = BT();
IR ir = IR();
Remote remote = Remote();
Notify notify = Notify();
PTP camera = PTP();
Light light = Light();

#include "Menu_Map.h"


/******************************************************************
 *
 *   setup
 *
 *
 ******************************************************************/

void setup()
{
	wdt_reset();
	wdt_disable();
	wdt_enable(WDTO_4S);

	setIn(BUTTON_FL_PIN);
	setHigh(BUTTON_FL_PIN);

#ifdef PRODUCTION
	if(battery_status() == 0 && getPin(BUTTON_FL_PIN)) // Looks like it's connected to the programmer
	{
		lcd.init(0x3);
		termInit();
		termPrintStrP(PSTR("\n  Programming\n  Successful\n\n  Move to test\n  station\n"));
		wdt_disable();
		hardware_off();
	}
#endif

	hardware_init();
	settings_init();

	// Splash Screen
	lcd.init(conf.lcdContrast);
    lcd.writeString(12, 07, STR("Timelapse+"));
    uint8_t l = lcd.measureStringTiny(conf.sysName) / 2;
    lcd.writeStringTiny(41 - l, 17, conf.sysName);

    #define BATT_X 26
    #define BATT_Y 26
    lcd.drawLine(0+BATT_X, BATT_Y+0, 30+BATT_X, BATT_Y+0);
    lcd.drawLine(0+BATT_X, BATT_Y+8, 30+BATT_X, BATT_Y+8);
    lcd.drawLine(0+BATT_X, BATT_Y+0, 0+BATT_X, BATT_Y+8);
    lcd.drawLine(30+BATT_X, BATT_Y+0, 30+BATT_X, BATT_Y+8);
    lcd.drawLine(31+BATT_X, BATT_Y+3, 31+BATT_X, BATT_Y+5);

    uint8_t battery = (uint8_t) ((float)battery_read()/100.0 * 27.0);

    for(uint8_t i = 0; i < battery; i++)
    {
	    lcd.drawLine(2+BATT_X+i, BATT_Y+2, 2+BATT_X+i, BATT_Y+6);
    }

	uint32_t version = VERSION;
	char buf[2], *text;
	l = 0;
	while(version)
	{
		char c = (char)(version % 10);
		buf[0] = ((char)(c + '0'));
		buf[1] = 0;
		text = buf;
		l += lcd.measureStringTiny(text) + 1;
		lcd.writeStringTiny(57 - l, 40, text);

		version -= (uint32_t)c;
		version /= 10;
	}
    lcd.update();


	clock.init();

	menu.lcd = &lcd;
	menu.button = &button;

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();

	battery_percent = battery_read();

	VirtualSerial_Init();

	bt.init();
	if(!bt.present)
	{
		_delay_ms(100);
		bt.init(); // give it one more try
	}
	if(conf.btMode == BT_MODE_SLEEP) bt.sleep(); else bt.advertise();
    
    _delay_ms(300); // just a little more delay so splash screen can be read

#ifdef PRODUCTION
	// Check to see if system has passed self-test
	if(eeprom_read_byte((const uint8_t*)&system_tested) == 0xFF || eeprom_read_byte((const uint8_t*)&system_tested) == 0x00 || button.get() == RIGHT_KEY)
	{
		if(test()) // Run self-test (requires connection to test station)
		{
			// Test passed
			eeprom_write_byte((uint8_t*)&system_tested, 0x01);
		}
		// Reboot
		wdt_enable(WDTO_8S);
		for(;;);
	}
#endif
}

/******************************************************************
 *
 *   main
 *
 *
 ******************************************************************/

int main()
{
//    setOut(POWER_HOLD_PIN); // Hold PWR on
//    setLow(POWER_HOLD_PIN);
	/****************************
	   Initialization
	*****************************/

	setup();

	menu.init((menu_item*)menu_main);

    if(conf.firmwareVersion != VERSION)
    {
        conf.firmwareVersion = VERSION;
        settings_save();
        menu.spawn((void*)firmwareUpdated);
    }
    else
    {
		if(battery_status() > 0)
			hardware_off(); // If it was just plugged in, show the charging screen

    }

	lcd.update();

	uint16_t count = 0;

	notify.watch(NOTIFY_CHARGE, (void *)&charge_status, sizeof(charge_status), &message_notify);
	notify.watch(NOTIFY_BT, (void *)&remote.connected, sizeof(remote.connected), &message_notify);
	notify.watch(NOTIFY_CAMERA, (void *)&PTP_Ready, sizeof(PTP_Ready), &message_notify);

	if(conf.autoRun)
	{
		menu.message(TEXT("Auto Started"));
		timer.begin();
		menu.spawn((void*)timerStatus);	
	}

	/****************************
	   Main Loop
	*****************************/

	for(;;)
	{
		wdt_reset();

		if(VirtualSerial_CharWaiting()) // Process USB Commands from PC (needs to be moved to sub-module)
		{
			char c = VirtualSerial_GetChar();

			switch(c)
			{
			   case '$':
				   hardware_bootloader();
				   break;

			   case 'V': // prints version backward
				   {
					   uint32_t version = VERSION;
					   char c;
					   while(version)
					   {
						   c = (char)(version % 10);
						   VirtualSerial_PutChar((char)(c + '0'));
						   version -= (uint32_t)c;
						   version /= 10;
					   }
					   VirtualSerial_PutChar('\r');
					   VirtualSerial_PutChar('\n');
					   break;
				   }

			   case 'v': // binary version (4 bytes)
				   {
					   uint32_t v = VERSION;
					   char *ptr;
					   ptr = (char*) &v;
					   VirtualSerial_PutChar(*ptr);
					   VirtualSerial_PutChar(*(ptr+1));
					   VirtualSerial_PutChar(*(ptr+2));
					   VirtualSerial_PutChar(*(ptr+3));
					   break;
				   }

			   case 'T':
				   VirtualSerial_PutChar('E');
				   break;

			   case 't':
			   		remote.send(REMOTE_THUMBNAIL, REMOTE_TYPE_SEND);
			   		break;

			   case 'C': // Capture
				   {
			   	       DEBUG(PSTR("Taking picture...\r\n"));
			   	       timer.capture();
					   break;
				   }

			   case 'M':
			   	    DEBUG(PSTR("REMOTE_MODEL: "));
				    DEBUG(remote.model);
				    DEBUG_NL();
			   	    DEBUG(PSTR("REMOTE_CONNECTED: "));
				    DEBUG(remote.connected);
				    DEBUG_NL();
				    break;

			   case 'B':
				   bt.init();
				   break;

			   case 'L':
			   	   light.start();
			   	   light.setRange(0);
			   	   DEBUG(PSTR("Light Sensor INIT\r\n"));
				   break;
#ifdef PRODUCTION
			   case 'R':
			   	   DEBUG(PSTR("Light Sensor Test Results:\r\n"));
			   	   readLightTest();
				   break;
#endif
			   case 'I':
			   	   light.integrationStart(10);
			   	   DEBUG(PSTR("Light Sensor Integration Start\r\n"));
				   break;

			   case 'l':
			   	   DEBUG(PSTR("Raw: "));
			   	   DEBUG(light.readRaw());
				   DEBUG_NL();
			   	   DEBUG(PSTR("Lux: "));
			   	   DEBUG(light.readLux());
				   DEBUG_NL();
			   	   DEBUG(PSTR("Ev: "));
			   	   DEBUG(light.readEv());
				   DEBUG_NL();
			   	   DEBUG(PSTR("IntEv: "));
			   	   DEBUG(light.readIntegratedEv());
				   DEBUG_NL();
			   	   DEBUG(PSTR("IntSlpope: "));
			   	   DEBUG(light.readIntegratedSlope());
				   DEBUG_NL();
				   break;

			   case 'S': // Screen dump
				   for(uint8_t j = 0; j < (LCD_HEIGHT >> 3); j++)
				   {
				       for(uint8_t i = 0; i < LCD_WIDTH; i++)
				       {
				           char b = lcd.screen[i][j];
				           VirtualSerial_PutChar(b);
				       }
				    }
			}
		}


		/****************************
		   Tasks
		*****************************/

		updateConditions();
		menu.task();
		timer.task();
		clock.task();
		bt.task();
		notify.task();
		light.task();

		if(USBmode == 1)
			PTP_Task();
		else
			VirtualSerial_Task();


		/****************************
		   Events / Notifications
		*****************************/
		
		count++;
		if(count > 5)
		{
			count = 0;
			charge_status = battery_status();
			camera.checkEvent();
		}

		if(bt.event) remote.event();

		if(menu.unusedKey == FR_KEY)
			hardware_flashlight_toggle();


		static uint32_t startTime = 0;

		if(clock.Ms() > startTime + 60000)
		{
			startTime = clock.Ms();
			battery_percent = battery_read();
		}

		if((hardware_USB_HostConnected || connectUSBcamera) && (USBmode == 0))
		{
			USBmode = 1;
			USB_Detach();
			USB_Disable();
			hardware_USB_SetHostMode();
			PTP_Enable();
		}
		else if((!hardware_USB_HostConnected && !connectUSBcamera) && (USBmode == 1))
		{
			USBmode = 0;
			PTP_Disable();
			hardware_USB_SetDeviceMode();
			VirtualSerial_Init();
		}

	}
}

void message_notify(uint8_t id)
{
	switch(id)
	{
		case NOTIFY_CHARGE:
			switch(charge_status)
			{
				case 0:
					menu.message(STR("Unplugged"));
					break;
				case 1:
					menu.message(STR("Charging"));
					break;
				case 2:
					menu.message(STR("Charged"));
					break;
			}
			break;
			
		case NOTIFY_BT:
			if(remote.connected)
			{
				menu.message(STR("BT Connected"));
			}
			else
			{
				menu.message(STR("Disconnected"));
				bt.advertise(); // this keeps the device awake after disconnecting, regardless of the conf.btMode setting
			}
			break;

		case NOTIFY_CAMERA:
			if(PTP_Ready)
			{
				settings_setup_camera_index(PTP_CameraSerial);
				menu.message(STR("USB Camera"));
				menu.refresh();
				camera.init();
				if(!conf.camera.autoConfigured && conf.auxPort == AUX_MODE_SYNC)
				{
			        menu.spawn((void*)autoConfigureCameraTiming);
				}
			}
			else
			{
				settings_load_camera_default();
				if(PTP_Error && timer.running)
				{
					//timerStop(0, 1);
					//menu.push(1);
					//menu.spawn((void*)usbPlug);
					menu.message(STR("PTP Error"));
				}
				else
				{
					menu.message(STR("Disconnected"));
				}
				camera.close();
				menu.refresh();
			}
			break;
	}
}

/******************************************************************
 *
 *   ISR
 * 
 *   Timer2 interrupt routine - called every millisecond
 *   Configured in hardware.cpp hardware_init()
 *
 ******************************************************************/

ISR(TIMER2_COMPA_vect)
{
	clock.count();
	button.poll();
    if(PTP_Run_Task) USB_USBTask();
}

/******************************************************************
 *
 *   ISR
 * 
 *   INT6 external interrupt routine - lightening trigger
 *   Configured in hardware.cpp hardware_lightening_enable()
 *
 ******************************************************************/

ISR(INT6_vect) // Lightening Trigger
{ 
	shutter_capture();
} 
