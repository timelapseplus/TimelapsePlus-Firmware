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
#include "LCD_Term.h"
#include "debug.h"
#include "bluetooth.h"
#include "settings.h"
#include "camera.h"
#include "math.h"
#include "selftest.h"
#include "remote.h"
#include "tlp_menu_functions.h"

unsigned char I2C_Buf[4];
#define I2C_ADDR  0b1000100

char system_tested EEMEM;

volatile uint8_t showGap = 0;
volatile uint8_t timerNotRunning = 1;
volatile uint8_t modeHDR = 0;
volatile uint8_t modeTimelapse = 1;
volatile uint8_t modeStandard = 1;
volatile uint8_t modeRamp = 0;
volatile uint8_t modeRampKeyAdd = 0;
volatile uint8_t modeRampKeyDel = 0;
volatile uint8_t modeBulb = 0;
volatile uint8_t bulb1 = 0;
volatile uint8_t bulb2 = 0;
volatile uint8_t bulb3 = 0;
volatile uint8_t bulb4 = 0;
volatile uint8_t showRemoteStart = 0;

volatile uint8_t connectUSBcamera = 0;

uint8_t battery_percent;

extern settings conf;

extern uint8_t Camera_Connected;

shutter timer = shutter();
LCD lcd = LCD();
MENU menu = MENU();
Clock clock = Clock();
Button button = Button();
BT bt = BT();
IR ir = IR();
Remote remote = Remote();

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
	wdt_enable(WDTO_2S);

	setIn(BUTTON_FL_PIN);
	setHigh(BUTTON_FL_PIN);

#ifdef PRODUCTION
	if(battery_status() == 0 && getPin(BUTTON_FL_PIN)) // Looks like it's connected to the programmer
	{
		lcd.init();
		termInit();
		termPrintStr(TEXT("\n  Programming\n  Successful\n\n  Move to test\n  station\n"));
		wdt_disable();
		hardware_off();
	}
#endif

	hardware_init();
	settings_init();

	clock.init();

	lcd.init();

	menu.lcd = &lcd;
	menu.button = &button;

	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	sleep_enable();

	battery_percent = battery_read();

	menu.init((menu_item*)menu_main);

	lcd.update();

	VirtualSerial_Init();

	// Configure I2C //
	TWI_Master_Initialise();

	bt.init();
	if(!bt.present)
	{
		_delay_ms(100);
		bt.init(); // give it one more try
	}
	bt.sleep();

#ifdef PRODUCTION
	//eeprom_write_byte((uint8_t *) &system_tested, 0x01);

	// Check to see if system has passed self-test
	if(eeprom_read_byte((const uint8_t*)&system_tested) == 0xFF || eeprom_read_byte((const uint8_t*)&system_tested) == 0x00 || button.get() == RIGHT_KEY)
	{
		if(test())
		{
			eeprom_write_byte((uint8_t*)&system_tested, 0x01);
		}
		wdt_enable(WDTO_8S);
		for(;;) ;
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
	char key;

	setup();

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


	timer.current.Keyframes = 1;
	uint16_t count = 0;

	for(;;)
	{
		count++;

		wdt_reset();

		if((hardware_USB_HostConnected || connectUSBcamera) && !hardware_USB_InHostMode)
		{
			USB_Detach();
			USB_Disable();
			hardware_USB_SetHostMode();
			Camera_Enable();
		}
		else if((!hardware_USB_HostConnected && !connectUSBcamera) && hardware_USB_InHostMode)
		{
			Camera_Disable();
			hardware_USB_SetDeviceMode();
			VirtualSerial_Init();
		}

		if(hardware_USB_InHostMode)
		{
			Camera_Task();
		}
		else
		{
			VirtualSerial_Task();
		}

		if(timerNotRunning != !timer.running) menu.refresh();
		timerNotRunning = !timer.running;
		modeTimelapse = (timer.current.Mode & TIMELAPSE);
		modeHDR = (timer.current.Mode & HDR);
		modeStandard = (!modeHDR && !modeRamp);
		modeRamp = (timer.current.Mode & RAMP);
		modeRampKeyAdd = (modeRamp && (timer.current.Keyframes < MAX_KEYFRAMES));
		modeRampKeyDel = (modeRamp && (timer.current.Keyframes > 1));
		bulb1 = timer.current.Keyframes > 1 && modeRamp;
		bulb2 = timer.current.Keyframes > 2 && modeRamp;
		bulb3 = timer.current.Keyframes > 3 && modeRamp;
		bulb4 = timer.current.Keyframes > 4 && modeRamp;
		showGap = timer.current.Photos > 1 && modeTimelapse;
		showRemoteStart = (remote.connected && !remote.running);

		if(VirtualSerial_CharWaiting()) // Process USB Commands from PC
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
						   debug((char)(c + '0'));
						   version -= (uint32_t)c;
						   version /= 10;
					   }
					   debug_nl();
					   break;
				   }

			   case 'T':
				   debug('E');
				   break;

			   case 'L':
				   readLightTest();
				   break;

			   case 'S': // Screen dump
				   for(uint8_t j = 0; j < (LCD_HEIGHT >> 3); j++)
				   {
				       for(uint8_t i = 0; i < LCD_WIDTH; i++)
				       {
				           char b = lcd.screen[i][j];
				           debug(b);
				       }
				    }
			}
		}

		switch(bt.task())
		{
			case BT_EVENT_CONNECT:
				menu.message(TEXT("Connected!"));
				remote.event();
				break;
			case BT_EVENT_DISCONNECT:
				menu.message(TEXT("Disconnected!"));
				remote.event();
				break;
			case BT_EVENT_DATA:
				remote.event();
				break;
		}

		key = menu.run();
		timer.run();

		if(key == FR_KEY)
		{
			if(hardware_flashlightIsOn())
			{
				hardware_flashlight(0);
			}
			else
			{
				hardware_flashlight(1);
			}
		}

		clock.sleepOk = timerNotRunning && !timer.cableIsConnected() && bt.state != BT_ST_CONNECTED;
		clock.sleep();

		uint8_t batteryRead = battery_read();

		if(batteryRead != battery_percent)
		{
			battery_percent = batteryRead;

			if(remote.notifyBattery)
			{
				remote.send(REMOTE_BATTERY, REMOTE_TYPE_SEND);
			}
		}
	}
}



/******************************************************************
 *
 *   ISR
 * 
 *   Timer2 interrupt routine - called every millisecond
 *
 ******************************************************************/

ISR(TIMER2_COMPA_vect)
{
	clock.count();
	button.poll();
}

