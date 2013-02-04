#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/wdt.h>
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
#include "PTP.h"
#include "math.h"
#include "selftest.h"
#include "remote.h"
#include "tlp_menu_functions.h"

volatile uint8_t showGap = 0;
volatile uint8_t timerNotRunning = 1;
volatile uint8_t modeHDR = 0;
volatile uint8_t modeTimelapse = 1;
volatile uint8_t modeStandard = 1;
volatile uint8_t modeRamp = 0;
volatile uint8_t modeNoRamp = 1;
volatile uint8_t modeRampKeyAdd = 0;
volatile uint8_t modeRampKeyDel = 0;
volatile uint8_t modeBulb = 0;
volatile uint8_t bulb1 = 0;
volatile uint8_t bulb2 = 0;
volatile uint8_t bulb3 = 0;
volatile uint8_t bulb4 = 0;
volatile uint8_t showRemoteStart = 0;
volatile uint8_t showRemoteInfo = 0;

extern uint8_t battery_percent;
extern settings conf;
extern uint8_t Camera_Connected;
extern volatile uint8_t connectUSBcamera;

extern shutter timer;
extern LCD lcd;
extern MENU menu;
extern Clock clock;
extern Button button;
extern BT bt;
extern IR ir;
extern Remote remote;
extern PTP camera;

uint8_t sleepOk = 1;

#include "Menu_Map.h"



/******************************************************************
 *
 *   updateConditions
 *
 *
 ******************************************************************/

void updateConditions()
{
	if(timerNotRunning != !timer.running) menu.refresh();
	timerNotRunning = !timer.running;
	modeTimelapse = (timer.current.Mode & TIMELAPSE);
	modeHDR = (timer.current.Mode & HDR);
	modeStandard = (!modeHDR && !modeRamp);
	modeRamp = (timer.current.Mode & RAMP);
	modeNoRamp = !modeRamp && modeTimelapse;
	modeRampKeyAdd = (modeRamp && (timer.current.Keyframes < MAX_KEYFRAMES));
	modeRampKeyDel = (modeRamp && (timer.current.Keyframes > 1));
	bulb1 = timer.current.Keyframes > 1 && modeRamp;
	bulb2 = timer.current.Keyframes > 2 && modeRamp;
	bulb3 = timer.current.Keyframes > 3 && modeRamp;
	bulb4 = timer.current.Keyframes > 4 && modeRamp;
	showGap = timer.current.Photos != 1 && modeTimelapse;
	showRemoteStart = (remote.connected && !remote.running && remote.model == REMOTE_MODEL_TLP);	
	showRemoteInfo = (remote.connected && (remote.model == REMOTE_MODEL_TLP || remote.model == REMOTE_MODEL_IPHONE));
	clock.sleepOk = timerNotRunning && !timer.cableIsConnected() && bt.state != BT_ST_CONNECTED && sleepOk;
}

/******************************************************************
 *
 *   firmwareUpdated
 *	 - shown once after firmware version changes
 *
 ******************************************************************/

volatile char firmwareUpdated(char key, char first)
{
	if(first)
	{
		uint8_t l, c;
		char* text;
		char buf[6];

		lcd.cls();
		menu.setTitle(TEXT("FIRMWARE"));

		lcd.writeStringTiny(13, 10, TEXT("Successfully"));
		lcd.writeStringTiny(25, 16, TEXT("Updated"));

		lcd.writeStringTiny(8, 28, TEXT("Version:"));
		uint32_t version = VERSION;

		l = 0;

		while(version)
		{
			c = (char)(version % 10);
			buf[0] = ((char)(c + '0'));
			buf[1] = 0;
			text = buf;
			l += lcd.measureStringTiny(text) + 1;
			lcd.writeStringTiny(75 - l, 28, text);

			version -= (uint32_t)c;
			version /= 10;
		}
		menu.setBar(TEXT("RETURN"), BLANK_STR);
		lcd.update();
	}

	switch(key)
	{
	   case FL_KEY:
	   case LEFT_KEY:
	   		if(settings_reset)
	   		{
	   			menu.spawn((void*)&firstSetup);
	   			return FN_JUMP;
	   		}
		    return FN_CANCEL;
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   firstSetup
 *	 - shown once after settings version changes
 *
 ******************************************************************/

volatile char firstSetup(char key, char first)
{
	if(first)
	{
		lcd.cls();
		menu.setTitle(TEXT("First Setup"));

		lcd.writeStringTiny(2, 8, TEXT(" Please go to the "));
		lcd.writeStringTiny(2, 14, TEXT(" Settings Menu and"));
		lcd.writeStringTiny(2, 20, TEXT(" set Camera Make  "));
		lcd.writeStringTiny(2, 26, TEXT(" and Camera FPS   "));
		lcd.writeStringTiny(2, 32, TEXT(" before using     "));

		menu.setBar(TEXT("OK"), BLANK_STR);
		lcd.update();
	}

	switch(key)
	{
	   case FL_KEY:
	   case LEFT_KEY:
		    return FN_CANCEL;
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   IRremote
 *
 *
 ******************************************************************/

volatile char IRremote(char key, char first)
{
	if(first)
	{
		lcd.cls();
		menu.setTitle(TEXT("IR Remote"));
		menu.setBar(TEXT("Delayed"), TEXT("Trigger"));
		lcd.update();
	}

	switch(key)
	{
	   case FL_KEY:
		   ir.shutterDelayed();
		   break;

	   case FR_KEY:
		   ir.shutterNow();
		   break;

	   case LEFT_KEY:
		   return FN_CANCEL;
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   shutterTest
 *
 *
 ******************************************************************/

volatile char shutterTest(char key, char first)
{
	static char status, cable;

	if(first)
	{
		status = 0;
		cable = 0;
		lcd.cls();
		menu.setTitle(TEXT("Shutter Test"));
		menu.setBar(TEXT("Half"), TEXT("Full"));
		lcd.update();
	}

	if(key == FL_KEY && status != 1)
	{
		status = 1;
		lcd.eraseBox(20, 18, 20 + 6 * 6, 26);
		lcd.writeString(20, 18, TEXT("(HALF)"));
		timer.half();
		lcd.update();
	}
	else if(key == FR_KEY && status != 2)
	{
		status = 2;
		lcd.eraseBox(20, 18, 20 + 6 * 6, 26);
		lcd.writeString(20, 18, TEXT("(FULL)"));
		timer.full();
		lcd.update();
	}
	else if(key != 0)
	{
		status = 0;
		lcd.eraseBox(20, 18, 20 + 6 * 6, 26);
		timer.off();
		lcd.update();
	}

	if(timer.cableIsConnected())
	{
		if(cable == 0)
		{
			cable = 1;
			lcd.writeStringTiny(6, 28, TEXT("Cable Connected"));
			lcd.update();
		}
	}
	else
	{
		if(cable == 1)
		{
			cable = 0;
			lcd.eraseBox(6, 28, 6 + 15 * 5, 36);
			lcd.update();
		}
	}

	if(key == LEFT_KEY)
		return FN_CANCEL;

	return FN_CONTINUE;
}


/******************************************************************
 *
 *   cableRelease
 *
 *
 ******************************************************************/

volatile char cableRelease(char key, char first)
{
	static char status; //, cable;

	if(first)
	{
		status = 0;
		//cable = 0;
		lcd.cls();
		menu.setTitle(TEXT("Cable Remote"));
		menu.setBar(TEXT("Bulb"), TEXT("Photo"));
		lcd.update();
		timer.half();
	}

	if(key == FL_KEY)
	{
		if(status != 1)
		{
			status = 1;
			lcd.eraseBox(8, 18, 8 + 6 * 11, 26);
			lcd.writeString(8, 18, TEXT("(BULB OPEN)"));
			timer.bulbStart();
			lcd.update();
		} else
		{
			status = 0;
			lcd.eraseBox(8, 18, 8 + 6 * 11, 26);
			timer.bulbEnd();
			lcd.update();
		}
	}
	else if(key == FR_KEY && status != 1)
	{
		status = 0;
		lcd.eraseBox(8, 18, 8 + 6 * 11, 26);
		timer.capture();
		lcd.update();
	}
	else if(key != 0)
	{
		status = 0;
		lcd.eraseBox(8, 18, 8 + 6 * 11, 26);
		timer.half();
		lcd.update();
	}
/*
	if(timer.cableIsConnected())
	{
		if(cable == 0)
		{
			cable = 1;
			lcd.writeStringTiny(6, 28, TEXT("Cable Connected"));
			lcd.update();
		}
	}
	else
	{
		if(cable == 1)
		{
			cable = 0;
			lcd.eraseBox(6, 28, 6 + 15 * 5, 36);
			lcd.update();
		}
	}
*/
	if(key == LEFT_KEY)
	{
		timer.off();
		return FN_CANCEL;
	}
	return FN_CONTINUE;
}

/******************************************************************
 *
 *   cableReleaseRemote
 *
 *
 ******************************************************************/

volatile char cableReleaseRemote(char key, char first)
{
	static char status; //, cable;

	if(first)
	{
		status = 0;
		lcd.cls();
		menu.setTitle(TEXT("BT Cable Remote"));
		menu.setBar(TEXT("Bulb"), TEXT("Photo"));
		lcd.update();
		remote.set(REMOTE_BULB_END);
	}

	if(status != 1)
	{
		if(key == FL_KEY)
		{
			status = 1;
			lcd.eraseBox(8, 18, 8 + 6 * 11, 26);
			lcd.writeString(8, 18, TEXT("(BULB OPEN)"));
			remote.set(REMOTE_BULB_START);
			lcd.update();
		}
		else if(key == FR_KEY)
		{
			status = 0;
			lcd.eraseBox(8, 18, 8 + 6 * 11, 26);
			remote.set(REMOTE_CAPTURE);
			lcd.update();
		}
	}
	else if(key != 0)
	{
		status = 0;
		lcd.eraseBox(8, 18, 8 + 6 * 11, 26);
		remote.set(REMOTE_BULB_END);
		lcd.update();
	}

	if(key == LEFT_KEY || !remote.connected)
	{
		return FN_CANCEL;
	}
	return FN_CONTINUE;
}

/******************************************************************
 *
 *   shutterLagTest
 *
 *
 ******************************************************************/

volatile char shutterLagTest(char key, char first)
{
//  static uint8_t cable;
	uint16_t start_lag, end_lag;

	if(first)
	{
		lcd.cls();
		menu.setTitle(TEXT("Calc BOffset"));
		menu.setBar(TEXT("Return"), TEXT("Test"));
		lcd.update();
	}

	if(key == FR_KEY)
	{
		lcd.eraseBox(10, 8, 80, 38);
		lcd.writeString(10,  8, TEXT("    In:"));
		lcd.writeString(10, 18, TEXT("   Out:"));
		lcd.writeString(10, 28, TEXT("Offset:"));

		ENABLE_SHUTTER;
		ENABLE_MIRROR;
		ENABLE_AUX_PORT;

		_delay_ms(100);

		if(key == FR_KEY)
		{
			MIRROR_UP;
			_delay_ms(1000);
		}

		SHUTTER_OPEN;
		clock.tare();

		while(!AUX_INPUT1)
		{
			if(clock.eventMs() >= 1000)
				break;
		}

		start_lag = (uint16_t)clock.eventMs();

		_delay_ms(50);

		SHUTTER_CLOSE;
		clock.tare();

		while(AUX_INPUT1)
		{
			if(clock.eventMs() > 1000)
				break;
		}

		end_lag = (uint16_t)clock.eventMs();

		lcd.writeNumber(56, 8, start_lag, 'U', 'L');
		lcd.writeNumber(56, 18, end_lag, 'U', 'L');
		lcd.writeNumber(56, 28, start_lag - end_lag, 'U', 'L');

		lcd.update();
	}

	if(key == FL_KEY || key == LEFT_KEY)
		return FN_CANCEL;

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   memoryFree
 *
 *
 ******************************************************************/

volatile char memoryFree(char key, char first)
{
	if(first)
	{
		unsigned int mem = hardware_freeMemory();

		lcd.cls();
		lcd.writeString(1, 18, TEXT("Free RAM:"));
		/*char x =*/lcd.writeNumber(55, 18, mem, 'U', 'L');
		//lcd.writeString(55 + x * 6, 18, TEXT("b"));
		menu.setTitle(TEXT("Memory"));
		menu.setBar(TEXT("RETURN"), BLANK_STR);
		lcd.update();
	}

	if(key == FL_KEY)
		return FN_CANCEL;

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   factoryReset
 *
 *
 ******************************************************************/

volatile char factoryReset(char key, char first)
{
	if(first)
	{
		lcd.cls();
		lcd.writeString(14, 12, TEXT("Reset all"));
		lcd.writeString(14, 22, TEXT("settings?"));
		menu.setTitle(TEXT("Reset"));
		menu.setBar(TEXT("CANCEL"), TEXT("RESET"));
		lcd.update();
	}

	switch(key)
	{
	   case FL_KEY:
		    return FN_CANCEL;

	   case FR_KEY:
	   		settings_default();
	   		timer.setDefault();
	   		menu.message(TEXT("Factory Reset"));
		    return FN_CANCEL;
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   viewSeconds
 *
 *
 ******************************************************************/

volatile char viewSeconds(char key, char first)
{
	if(first)
	{
		lcd.cls();
		lcd.writeString(1, 18, TEXT("Clock:"));
		menu.setTitle(TEXT("Clock"));
		menu.setBar(TEXT("TARE"), TEXT("RETURN"));
	}

	lcd.eraseBox(36, 18, 83, 18 + 8);
	/*char x =*/ lcd.writeNumber(83, 18, clock.Seconds(), 'F', 'R');
	lcd.update();

	switch(key)
	{
	   case FL_KEY:
		   clock.tare();
		   break;

	   case FR_KEY:
		   return FN_CANCEL;
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   getChargingStatus
 *
 *
 ******************************************************************/

char* getChargingStatus()
{
	switch(battery_status())
	{
	   case 1:
		   return TEXT("Charging");

	   case 2:
		   return TEXT("Charged");

	   case 0:
		   return TEXT("Unplugged");
	}

	return TEXT("ERROR");
}

/******************************************************************
 *
 *   batteryStatus
 *
 *
 ******************************************************************/

volatile char batteryStatus(char key, char first)
{
//	uint16_t batt_high = 645;
//	uint16_t batt_low = 540;
	static uint8_t charging;
	char stat = battery_status();

	if(first)
	{
		charging = (stat > 0);
	}

//	unsigned int batt_level = battery_read_raw();

#define BATT_LINES 36

//	uint8_t lines = ((batt_level - batt_low) * BATT_LINES) / (batt_high - batt_low);
	uint8_t lines = (uint8_t)((uint16_t)battery_percent * BATT_LINES / 100);

	if(lines > BATT_LINES - 1 && stat == 1)
		lines = BATT_LINES - 1;

	if(lines > BATT_LINES || stat == 2)
		lines = BATT_LINES;

	lcd.cls();

	char* text;

	text = getChargingStatus();

	char l = lcd.measureStringTiny(text) / 2;

	if(battery_status())
		lcd.writeStringTiny(41 - l, 31, text);

	// Draw Battery Outline //
	lcd.drawLine(20, 15, 60, 15);
	lcd.drawLine(20, 16, 20, 27);
	lcd.drawLine(21, 27, 60, 27);
	lcd.drawLine(60, 16, 60, 19);
	lcd.drawLine(60, 23, 60, 26);
	lcd.drawLine(61, 19, 61, 23);

	// Draw Battery Charge //
	for(uint8_t i = 0; i <= lines; i++)
	{
		lcd.drawLine(22 + i, 17, 22 + i, 25);
	}

	menu.setTitle(TEXT("Battery Status"));
	menu.setBar(TEXT("RETURN"), BLANK_STR);
	lcd.update();

	if(stat == 0 && charging)
	{
		clock.awake();
		return FN_CANCEL; // unplugged
	}
	if(key == FL_KEY)
		return FN_CANCEL;

	return FN_CONTINUE;
}

#define SY 2

/******************************************************************
 *
 *   sysStatus
 *
 *
 ******************************************************************/

volatile char sysStatus(char key, char first)
{
	char* text;

	if(first)
	{
	}

	lcd.cls();

	text = getChargingStatus();

	char l = lcd.measureStringTiny(text);

	lcd.writeStringTiny(80 - l, 6 + SY, text);
	lcd.writeStringTiny(3, 6 + SY, TEXT("USB:"));

	char buf[6];
	uint16_t val;

	val = (uint16_t)battery_percent;
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 12 + SY, text);
	lcd.writeStringTiny(3, 12 + SY, TEXT("Battery:"));

	val = hardware_freeMemory();
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 18 + SY, text);
	lcd.writeStringTiny(3, 18 + SY, TEXT("Free RAM:"));

	val = clock.seconds;
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 24 + SY, text);
	lcd.writeStringTiny(3, 24 + SY, TEXT("Clock s:"));

	val = clock.ms;
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 30 + SY, text);
	lcd.writeStringTiny(3, 30 + SY, TEXT("Clock ms:"));

	menu.setTitle(TEXT("Sys Status"));
	menu.setBar(TEXT("RETURN"), BLANK_STR);
	lcd.update();
	_delay_ms(10);

	if(key == FL_KEY || key == LEFT_KEY)
		return FN_CANCEL;

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   timerStatus
 *
 *
 ******************************************************************/

volatile char timerStatus(char key, char first)
{
	static uint8_t counter;
	if(first)
	{
		counter = 0;
	}

	if(counter++ > 3)
	{
		counter = 0;
		lcd.cls();

		displayTimerStatus(0);

		menu.setTitle(TEXT("Running"));
		menu.setBar(TEXT(""), TEXT("STOP"));
		lcd.update();
	}

	if(!timer.running) return FN_CANCEL;

	if(key == FR_KEY)
	{
		menu.push();
		menu.spawn((void*)timerStop);
		return FN_JUMP;
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   timerStatusRemote
 *
 *
 ******************************************************************/

volatile char timerStatusRemote(char key, char first)
{
	static uint32_t startTime = 0;
	static uint8_t init = 1;

	if(init)
	{
		startTime = 0;
		init = 0;

		remote.request(REMOTE_BATTERY);
		remote.watch(REMOTE_BATTERY);
		remote.request(REMOTE_START);
		remote.watch(REMOTE_START);
		remote.request(REMOTE_STATUS);
		remote.watch(REMOTE_STATUS);
	}

	if(clock.Ms() > startTime + 100 || first)
	{
		startTime = clock.Ms();
		lcd.cls();

		displayTimerStatus(1);

		menu.setTitle(TEXT("Remote"));

		if(remote.running)
			menu.setBar(TEXT("RETURN"), TEXT("STOP"));
		else
			menu.setBar(TEXT("RETURN"), BLANK_STR);

		lcd.update();
	}

	if(key == FR_KEY) remote.set(REMOTE_STOP);
	else if(!remote.connected || key == FL_KEY || key == LEFT_KEY)
	{
	   	init = 1;
		remote.unWatch(REMOTE_STATUS);
		remote.unWatch(REMOTE_START);
		remote.unWatch(REMOTE_BATTERY);
		return FN_CANCEL;
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   displayTimerStatus
 *
 *
 ******************************************************************/

void displayTimerStatus(uint8_t remote_system)
{
	timer_status stat;
	char buf[6], l, *text;
	uint16_t val;

	if(remote_system)
	{
		stat = remote.status;
		l = remote.running;
	}
	else
	{
		stat = timer.status;
		l = timer.running;
	}
	//
	//06 Time remaining
	//12 Time to next photo
	//18 Next bulb
	//24 Status
	//30 Battery %

	if(l)
	{
		if(stat.mode & TIMELAPSE)
		{
			val = stat.photosTaken;
			int_to_str(val, buf);
			text = buf;
			l = lcd.measureStringTiny(text);
			lcd.writeStringTiny(80 - l, 6 + SY, text);
			lcd.writeStringTiny(3, 6 + SY, TEXT("Photos:"));

			val = stat.photosRemaining;
			if(stat.infinitePhotos == 0)
			{
				int_to_str(val, buf);
				text = buf;
				l = lcd.measureStringTiny(text);
				lcd.writeStringTiny(80 - l, 12 + SY, text);
				lcd.writeStringTiny(3, 12 + SY, TEXT("Photos rem:"));
			}
			else
			{
				lcd.writeStringTiny(3, 12 + SY, TEXT("Infinite Photos"));
			}

			val = stat.nextPhoto;
			int_to_str(val, buf);
			text = buf;
			l = lcd.measureStringTiny(text);
			lcd.writeStringTiny(80 - l, 18 + SY, text);
			lcd.writeStringTiny(3, 18 + SY, TEXT("Next Photo:"));
		}
	}
	
	text = stat.textStatus;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 24 + SY, text);
	lcd.writeStringTiny(3, 24 + SY, TEXT("Status:"));

	if(remote_system)
		val = (uint16_t) remote.battery;
	else
		val = (uint16_t) battery_percent;
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 30 + SY, text);
	lcd.writeStringTiny(3, 30 + SY, TEXT("Battery Level:"));

}

/******************************************************************
 *
 *   sysInfo
 *
 *
 ******************************************************************/

volatile char sysInfo(char key, char first)
{
	if(first)
	{
		lcd.cls();

		char l;
		char* text;
		char buf[6];
		uint16_t val;

		// Lines (Y) = 6, 12, 18, 24, 30
		val = (uint16_t)bt.version();

		text = TEXT("TLP01");
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 6 + SY, text);
		lcd.writeStringTiny(3, 6 + SY, TEXT("Model:"));

		if(val > 1)
			text = TEXT("BTLE");
		else
			text = TEXT("KS99");

		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 12 + SY, text);
		lcd.writeStringTiny(3, 12 + SY, TEXT("Edition:"));

		lcd.writeStringTiny(3, 18 + SY, TEXT("Firmware:"));
		uint32_t version = VERSION;

		char c;

		l = 0;

		while(version)
		{
			c = (char)(version % 10);
			buf[0] = ((char)(c + '0'));
			buf[1] = 0;
			text = buf;
			l += lcd.measureStringTiny(text) + 1;
			lcd.writeStringTiny(80 - l, 18 + SY, text);

			version -= (uint32_t)c;
			version /= 10;
		}

		if(val > 1)
		{
			int_to_str(val, buf);
			text = buf;
			l = lcd.measureStringTiny(text);
			lcd.writeStringTiny(80 - l, 30 + SY, text);
			lcd.writeStringTiny(3, 30 + SY, TEXT("BT FW Version:"));
		}

		val = (uint16_t)battery_percent;
		int_to_str(val, buf);
		text = buf;
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 24 + SY, text);
		lcd.writeStringTiny(3, 24 + SY, TEXT("Battery:"));

		menu.setTitle(TEXT("System Info"));
		menu.setBar(TEXT("RETURN"), BLANK_STR);
		lcd.update();
	}

	if(key == FL_KEY || key == LEFT_KEY)
		return FN_CANCEL;

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   lightMeter
 *
 *
 ******************************************************************/

volatile char lightMeter(char key, char first)
{
	static char held = 0;

	if(first)
	{
		lcd.backlight(0);
		hardware_flashlight(0);
	}

	if(!held)
	{
		lcd.cls();

		menu.setTitle(TEXT("Light Meter"));

		if(key == FR_KEY)
		{
			held = 1;
			menu.setBar(TEXT("RETURN"), TEXT("RUN"));
		}
		else
		{
			menu.setBar(TEXT("RETURN"), TEXT("PAUSE"));
		}

		char buf[6] , l;
		uint16_t val;
		char* text;

		val = (uint16_t)hardware_readLight(0);
		int_to_str(val, buf);
		text = buf;
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 12 + SY, text);
		lcd.writeStringTiny(3, 12 + SY, TEXT("Level 1:"));

		val = (uint16_t)hardware_readLight(1);
		int_to_str(val, buf);
		text = buf;
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 18 + SY, text);
		lcd.writeStringTiny(3, 18 + SY, TEXT("Level 2:"));

		val = (uint16_t)hardware_readLight(2);
		int_to_str(val, buf);
		text = buf;
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 24 + SY, text);
		lcd.writeStringTiny(3, 24 + SY, TEXT("Level 3:"));

		lcd.update();
		_delay_ms(10);
	}
	else
	{
		if(key == FR_KEY)
			held = 0;
	}

	if(key == FL_KEY)
	{
		lcd.backlight(255);
		return FN_CANCEL;
	}

	return FN_CONTINUE;
}


/******************************************************************
 *
 *   motionTrigger
 *
 *
 ******************************************************************/

volatile char motionTrigger(char key, char first)
{
	uint8_t i;
	uint16_t val;
	static uint16_t lv[3];
	static uint8_t threshold = 2;

	if(key == LEFT_KEY)
	{
		if(threshold > 0) threshold--;
		first = 1;
	}
	if(key == RIGHT_KEY)
	{
		if(threshold < 4) threshold++;
		first = 1;
	}

	if(first)
	{
		sleepOk = 0;
		clock.tare();
		lcd.cls();
		menu.setTitle(TEXT("Motion Sensor"));
		menu.setBar(TEXT("RETURN"), BLANK_STR);

		lcd.drawLine(10, 22, 84-10, 22);
		lcd.drawLine(11, 21, 11, 23);
		lcd.drawLine(84-11, 21, 84-11, 23);
		lcd.drawLine(12, 20, 12, 24);
		lcd.drawLine(84-12, 20, 84-12, 24);
		lcd.drawLine(13, 20, 13, 24);
		lcd.drawLine(84-13, 20, 84-13, 24);
		lcd.setPixel(42, 21);
		lcd.setPixel(42+10, 21);
		lcd.setPixel(42-10, 21);
		lcd.setPixel(42+20, 21);
		lcd.setPixel(42-20, 21);

		i = threshold * 10;
		lcd.drawLine(42-3-20+i, 16, 42+3-20+i, 16);
		lcd.drawLine(42-2-20+i, 17, 42+2-20+i, 17);
		lcd.drawLine(42-1-20+i, 18, 42+1-20+i, 18);
		lcd.setPixel(42-20+i, 19);

		lcd.writeStringTiny(19, 25, TEXT("SENSITIVITY"));

		lcd.update();
		lcd.backlight(0);
		hardware_flashlight(0);
		_delay_ms(50);
		for(i = 0; i < 3; i++)
		{
			lv[i] = (uint16_t)hardware_readLight(i);
		}
	}

	uint8_t thres = 4 - threshold + 2;
	if((4 - threshold) > 2) thres += ((4 - threshold) - 1) * 2;

	for(i = 0; i < 3; i++)
	{
		val = (uint16_t)hardware_readLight(i);
		if(clock.eventMs() > 1000 && val > thres && (val < (lv[i] - thres) || val > (lv[i] + thres)))
		{
			clock.tare();
			shutter_capture();
		}
		lv[i] = val;
	}

	if(key == FL_KEY)
	{
		sleepOk = 1;
		lcd.backlight(255);
		return FN_CANCEL;
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   int_to_str
 *
 *
 ******************************************************************/

void int_to_str(uint16_t n, char buf[6] )
{
	char digits[6];

	// If it is zero just set and forget
	if(n == 0)
	{
		buf[0] = '0';
		buf[1] = 0;
		return;
	}

	// Value too high for this code
	if(n > 99999)
	{
		buf[0] = '!';
		buf[1] = '!';
		buf[2] = 0;
		return;
	}

	// calculate the digits for 5 places
	digits[0] = n / 10000;
	n -= digits[0] * 10000;
	digits[1] = n / 1000;
	n -= digits[1] * 1000;
	digits[2] = n / 100;
	n -= digits[2] * 100;
	digits[3] = n / 10;
	n -= digits[3] * 10;
	digits[4] = n;
	digits[5] = 0;

	// convert digits to ASCII range
	digits[0] += '0';
	digits[1] += '0';
	digits[2] += '0';
	digits[3] += '0';
	digits[4] += '0';

	uint8_t idx = 0;
	uint8_t bufidx = 0;

	// skip leading zeros
	while(idx < 5 && digits[idx] == '0')
		idx++;

	// Copy all the remaining digits to buf
	while(idx < 5)
	{
		buf[bufidx++] = digits[idx++];
	}

	// null terminate buf
	buf[bufidx] = 0;

	return;
}

/******************************************************************
 *
 *   notYet
 *
 *
 ******************************************************************/

volatile char notYet(char key, char first)
{
	if(first)
	{
		lcd.cls();
		lcd.writeString(3, 7, TEXT("Sorry, this  "));
		lcd.writeString(3, 15, TEXT("feature has  "));
		lcd.writeString(3, 23, TEXT("not yet been "));
		lcd.writeString(3, 31, TEXT("implemented  "));
		menu.setTitle(TEXT("Not Yet"));
		menu.setBar(TEXT("RETURN"), BLANK_STR);
		lcd.update();
	}

	if(key)
		return FN_CANCEL;

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   btConnect
 *
 *
 ******************************************************************/


volatile char btConnect(char key, char first)
{
	static uint8_t sfirst = 1;
	uint8_t i;
	static uint8_t menuSize;
	static uint8_t menuSelected;
	uint8_t c;
	uint8_t update, menuScroll;
	update = 0;

	if(sfirst)
	{
		menuScroll = 0;
		menuSelected = 0;
		sfirst = 0;

		update = 1;
		if(bt.state != BT_ST_CONNECTED)
		{
			debug(STR("BT Advertising!\r\n"));
			bt.advertise();
			bt.scan();
		}
	}

	switch(key)
	{
		case LEFT_KEY:
		case FL_KEY:
			sfirst = 1;
			if(bt.state != BT_ST_CONNECTED)
			{
				if(conf.btMode == BT_MODE_SLEEP) bt.sleep(); else bt.advertise();
			}
			return FN_CANCEL;

		case FR_KEY:
			if(bt.state == BT_ST_CONNECTED)
			{
				bt.disconnect();
			}
			else
			{
				bt.connect(bt.device[menuSelected].addr);
			}
			break;
	}

	update = 1;
	switch(bt.event)
	{
		case BT_EVENT_DISCOVERY:
			debug(STR("dicovery!\r\n"));
			break;
		case BT_EVENT_SCAN_COMPLETE:
			debug(STR("done!\r\n"));
			if(bt.state != BT_ST_CONNECTED)
			{
				bt.advertise();
				bt.scan();
			}
			break;
		case BT_EVENT_DISCONNECT:		
			bt.scan();
			break;
		default:
			update = 0;
	}

	bt.event = BT_EVENT_NULL; // clear event so we don't process it twice

	if(first)
	{
		update = 1;
	}

	if(key == UP_KEY && menuSelected > 0)
	{
		menuSelected--;
		update = 1;
	}
	else if(key == DOWN_KEY && menuSelected < menuSize - 1)
	{
		menuSelected++;
		update = 1;
	}

	if(update)
	{
		lcd.cls();

		if(bt.state == BT_ST_CONNECTED)
		{
			menu.setTitle(TEXT("Connect"));
			lcd.writeStringTiny(18, 20, TEXT("Connected!"));
			menu.setBar(TEXT("RETURN"), TEXT("DISCONNECT"));
		}
		else
		{
			if(menuSelected > 2)
				menuScroll = menuSelected - 2;

			menuSize = 0;

			for(i = 0; i < bt.devices; i++)
			{
				if(i >= menuScroll && i <= menuScroll + 4)
				{
					for(c = 0; c < MENU_NAME_LEN - 1; c++) // Write settings item text //
					{
							if(bt.device[i].name[c])
								lcd.writeChar(3 + c * 6, 8 + 9 * (menuSize - menuScroll), bt.device[i].name[c]);
					}
				}
				menuSize++;
			}

			if(bt.devices)
			{
				lcd.drawHighlight(2, 7 + 9 * (menuSelected - menuScroll), 81, 7 + 9 * (menuSelected - menuScroll) + 8);
				menu.setBar(TEXT("RETURN"), TEXT("CONNECT"));
			}
			else
			{
				lcd.writeStringTiny(6, 20, TEXT("No Devices Found"));
				menu.setBar(TEXT("RETURN"), BLANK_STR);
			}

			menu.setTitle(TEXT("Connect"));

			lcd.drawLine(0, 3, 0, 40);
			lcd.drawLine(83, 3, 83, 40);
		}

		lcd.update();
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   usbPlug
 *
 *
 ******************************************************************/

volatile char usbPlug(char key, char first)
{
	static char connected = 0;

	if(first || (PTP_Connected != connected) || (PTP_Ready))
	{
		connected = PTP_Connected;
		char exp_name[7];

		if(PTP_Connected)
		{
			if(PTP_Error)
			{
				lcd.cls();
				lcd.writeString(3, 7,  TEXT(" PTP Error!  "));

				lcd.writeChar(3+3*6, 15, '(');
				char b, *c = (char*)&PTP_Error;
				b = (c[1] >> 4) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+4*6, 15, b);
				b = (c[1] & 0x0F) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+5*6, 15, b);
				b = (c[0] >> 4) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+6*6, 15, b);
				b = (c[0] & 0x0F) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+7*6, 15, b);
				lcd.writeChar(3+8*6, 15, ')');

				lcd.writeString(3, 23, TEXT("Unplug camera"));
				lcd.writeString(3, 31, TEXT("to reset...  "));
				menu.setTitle(TEXT("Camera Info"));
				menu.setBar(TEXT("RETURN"), BLANK_STR);
				lcd.update();
				connectUSBcamera = 1;

			}
			else if(PTP_Ready)
			{
				lcd.cls();
				lcd.writeStringTiny(1, 7,  PTP_CameraModel);
				if(camera.shutterName(exp_name, camera.shutter()))
				{
					lcd.writeString(3+22-6, 15, exp_name);
				}
				if(camera.apertureName(exp_name, camera.aperture()))
				{
					lcd.writeString(3+22-6, 23, exp_name);
				}
				if(camera.isoName(exp_name, camera.iso()))
				{
					lcd.writeString(3, 31, exp_name);
					lcd.writeString(3+46, 31, TEXT("ISO"));
				}
				menu.setTitle(TEXT("Camera Info"));
				menu.setBar(TEXT("RETURN"), TEXT("PHOTO"));
				lcd.update();
				connectUSBcamera = 1;

			}
			else
			{
				lcd.cls();
				lcd.writeString(3, 7,  TEXT(" Connected!  "));
				lcd.writeString(3, 15, TEXT(" Retrieving  "));
				lcd.writeString(3, 23, TEXT("   Device    "));
				lcd.writeString(3, 31, TEXT("   Info...   "));
				menu.setTitle(TEXT("Camera Info"));
				menu.setBar(TEXT("RETURN"), BLANK_STR);
				lcd.update();
				connectUSBcamera = 1;

			}
		}
		else
		{
			lcd.cls();
			lcd.writeString(3, 7, TEXT("Plug camera  "));
			lcd.writeString(3, 15, TEXT("into left USB"));
			lcd.writeString(3, 23, TEXT("port...      "));
			lcd.writeString(3, 31, TEXT("             "));
			menu.setTitle(TEXT("Connect USB"));
			menu.setBar(TEXT("CANCEL"), BLANK_STR);
			lcd.update();
			connectUSBcamera = 1;
		}
	}

	if(key == FL_KEY || key == LEFT_KEY)
	{
		if(!PTP_Connected)
			connectUSBcamera = 0;

		return FN_CANCEL;
	}
	else if(key == FR_KEY)
	{
		if(PTP_Ready) camera.capture();
	}
	else if(key == UP_KEY)
	{
//		if(PTP_Ready) camera.moveFocus(1);
		if(PTP_Ready) camera.setISO(camera.isoUp(camera.iso()));
	}
	else if(key == DOWN_KEY)
	{
//		if(PTP_Ready) camera.moveFocus(0x8001);
		if(PTP_Ready) camera.setISO(camera.isoDown(camera.iso()));
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   lighteningTrigger
 *
 *
 ******************************************************************/

volatile char lighteningTrigger(char key, char first)
{
	if(first)
	{
		sleepOk = 0;
		hardware_lightening_enable();
		lcd.cls();
		menu.setTitle(TEXT("Lightening"));
		lcd.writeString(25, 20, TEXT("READY"));
		menu.setBar(TEXT("RETURN"), TEXT("CALIBRATE"));
		lcd.update();
	}

	if(key == FL_KEY || key == LEFT_KEY)
	{
		sleepOk = 1;
		hardware_lightening_disable();
		return FN_CANCEL;
	}
	if(key == FR_KEY)
	{
		menu.message(TEXT("Calibrating"));
		hardware_lightening_disable();
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   runHandler
 *
 *
 ******************************************************************/

volatile char runHandler(char key, char first)
{
	static char pressed;

	if(first)
	{
		pressed = key;
		key = 0;
	}

	if(pressed == FR_KEY)
	{
		menu.message(TEXT("Timer Started"));
		timer.begin();
		menu.spawn((void*)timerStatus);
		return FN_JUMP;
	}

	menu.push(0);
	menu.submenu((void*)menu_options);

	return FN_JUMP;
}

/******************************************************************
 *
 *   timerRemoteStart
 *
 *
 ******************************************************************/

volatile char timerRemoteStart(char key, char first)
{
	menu.message(TEXT("Started Remote"));
	remote.set(REMOTE_PROGRAM);
	remote.set(REMOTE_START);
	menu.spawn((void*)timerStatusRemote);
	
	return FN_JUMP;
}

/******************************************************************
 *
 *   btFloodTest
 *
 *
 ******************************************************************/

volatile char btFloodTest(char key, char first)
{
	if(first)
	{
		lcd.cls();
		menu.setTitle(TEXT("BT Flood"));
		menu.setBar(TEXT("CANCEL"), TEXT("FLOOD"));
		lcd.update();
	}

	if(key == LEFT_KEY)
	{
		return FN_CANCEL;
	}
	else if(key == FR_KEY)
	{
		remote.send(REMOTE_PROGRAM, REMOTE_TYPE_SEND);
		remote.send(REMOTE_PROGRAM, REMOTE_TYPE_SEND);
		remote.send(REMOTE_PROGRAM, REMOTE_TYPE_SEND);
		remote.send(REMOTE_PROGRAM, REMOTE_TYPE_SEND);
		remote.send(REMOTE_PROGRAM, REMOTE_TYPE_SEND);
		menu.message(TEXT("Sent 5 Packets"));
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   timerStop
 *		Stops intervalometer if currently running
 *
 ******************************************************************/

volatile char timerStop(char key, char first)
{
	if(first)
		timer.running = 0;

	menu.message(TEXT("Stopped"));
	menu.back();
	return FN_CANCEL;
}

/******************************************************************
 *
 *   menuBack
 *
 *
 ******************************************************************/

volatile char menuBack(char key, char first)
{
	if(key == FL_KEY)
	{
		menu.back();
		return FN_CANCEL;
	}

	return FN_CANCEL;
}

/******************************************************************
 *
 *   timerSaveDefault
 *
 *
 ******************************************************************/

volatile char timerSaveDefault(char key, char first)
{
	if(first)
	{
		timer.current.Name[0] = 'D';
		timer.current.Name[1] = 'E';
		timer.current.Name[2] = 'F';
		timer.current.Name[3] = 'A';
		timer.current.Name[4] = 'U';
		timer.current.Name[5] = 'L';
		timer.current.Name[6] = 'T';
		timer.current.Name[7] = '\0';
		timer.save(0);
	}

	menu.message(TEXT("Saved"));
	menu.back();
	return FN_CANCEL;
}

/******************************************************************
 *
 *   timerSaveCurrent
 *
 *
 ******************************************************************/

volatile char timerSaveCurrent(char key, char first)
{
	if(first)
		timer.save(timer.currentId);

	menu.message(TEXT("Saved"));
	menu.back();
	return FN_CANCEL;
}

/******************************************************************
 *
 *   timerRevert
 *
 *
 ******************************************************************/

volatile char timerRevert(char key, char first)
{
	if(first)
		timer.load(timer.currentId);

	menu.message(TEXT("Reverted"));
	menu.back();
    menu.select(0);
	return FN_CANCEL;
}

/******************************************************************
 *
 *   shutter_addKeyframe
 *
 *
 ******************************************************************/

volatile char shutter_addKeyframe(char key, char first)
{
	if(timer.current.Keyframes < MAX_KEYFRAMES)
	{
		if(timer.current.Keyframes < 1)
			timer.current.Keyframes = 1;

		timer.current.Key[timer.current.Keyframes] = timer.current.Key[timer.current.Keyframes - 1] + 3600;
		timer.current.Bulb[timer.current.Keyframes + 1] = timer.current.Bulb[timer.current.Keyframes];
		timer.current.Keyframes++;
	}

	menu.back();

	return FN_CANCEL;
}

/******************************************************************
 *
 *   shutter_removeKeyframe
 *
 *
 ******************************************************************/

volatile char shutter_removeKeyframe(char key, char first)
{
	if(timer.current.Keyframes > 1)
	{
		timer.current.Keyframes--;
	}

	menu.back();
	menu.select(0);

	return FN_CANCEL;
}

/******************************************************************
 *
 *   shutter_saveAs
 *
 *
 ******************************************************************/

volatile char shutter_saveAs(char key, char first)
{
	static char name[MENU_NAME_LEN - 1];
	static char newId;

	if(first)
	{
		newId = timer.nextId();

		if(newId < 0)
		{
			menu.message(TEXT("No Space"));
			return FN_CANCEL;
		}
	}

	char ret = menu.editText(key, name, TEXT("Save As"), first);

	if(ret == FN_SAVE)
	{
		name[MENU_NAME_LEN - 2] = 0;
		strcpy((char*)timer.current.Name, name);
		timer.save(newId);
		for(uint8_t i = 0; i < MENU_NAME_LEN - 1; i++) name[i] = 0;
		menu.message(TEXT("Saved"));
		menu.back();
	}
	else if(ret == FN_CANCEL)
	{
		for(uint8_t i = 0; i < MENU_NAME_LEN - 1; i++) name[i] = 0;
	}

	return ret;
}

/******************************************************************
 *
 *   shutter_load
 *
 *
 ******************************************************************/
static uint8_t itemSelected;

volatile char shutter_load(char key, char first)
{
	static char menuSize;
	static char menuSelected;

	uint8_t c;
	char ch, update, menuScroll;

	update = 0;

	if(first)
	{
		menuScroll = 0;
		update = 1;
	}

	if(key == UP_KEY && menuSelected > 0)
	{
		menuSelected--;
		update = 1;

	}
	else if(key == DOWN_KEY && menuSelected < menuSize - 1)
	{
		menuSelected++;
		update = 1;
	}

	if(update)
	{
		lcd.cls();

		if(menuSelected > 2)
			menuScroll = menuSelected - 2;

		menuSize = 0;
		char i = 0;

		for(char x = 1; x < MAX_STORED; x++)
		{
			i++;
			ch = eeprom_read_byte((uint8_t*)&stored[i - 1].Name[0]);

			if(ch == 0 || ch == 255)
				continue;

			for(c = 0; c < MENU_NAME_LEN - 1; c++) // Write settings item text //
			{
				if(i >= menuScroll && i <= menuScroll + 5)
				{
					ch = eeprom_read_byte((uint8_t*)&stored[i - 1].Name[c]);

					if(ch == 0) break;

					if((ch < 'A' || ch > 'Z') && (ch < '0' || ch > '9'))
						ch = ' ';

					lcd.writeChar(3 + c * 6, 8 + 9 * (menuSize - menuScroll), ch);

					if(menuSize == menuSelected)
						itemSelected = i - 1;
				}
			}
			menuSize++;
		}

		if(menuSelected > menuSize - 1 && menuSelected > 0) menuSelected--;

		lcd.drawHighlight(2, 7 + 9 * (menuSelected - menuScroll), 81, 7 + 9 * (menuSelected - menuScroll) + 8);

		menu.setTitle(TEXT("Load Saved"));
		if(itemSelected > 0)
			menu.setBar(TEXT("OPTIONS"), TEXT("LOAD"));
		else
			menu.setBar(BLANK_STR, TEXT("LOAD"));

		lcd.drawLine(0, 3, 0, 40);
		lcd.drawLine(83, 3, 83, 40);

		lcd.update();
	}

	switch(key)
	{
	   case FL_KEY:
	   		if(itemSelected > 0)
	   		{
		   		menu.push(1);
			   	menu.submenu((menu_item*)menu_saved_options);
			   	return FN_JUMP;
	   		}
	   		break;

	   case LEFT_KEY:
		   return FN_CANCEL;

	   case FR_KEY:
	   case RIGHT_KEY:
		   timer.load(itemSelected);
		   menu.message(TEXT("Loaded"));
		   menu.back();
		   menu.select(0);
		   return FN_SAVE;
	}

	return FN_CONTINUE;
}

volatile char shutter_delete(char key, char first)
{
	if(first)
	{
		lcd.cls();

		menu.setTitle(TEXT("Delete"));
		menu.setBar(TEXT("CANCEL"), TEXT("DELETE"));

		lcd.writeString(15, 8 + 5, TEXT("Delete"));
		uint8_t c, lastChar = 0;
		for(c = 0; c < MENU_NAME_LEN - 2; c++) // Write settings item text //
		{
			char ch = eeprom_read_byte((uint8_t*)&stored[itemSelected].Name[c]);

			if(ch == 0) break;

			if((ch < 'A' || ch > 'Z') && (ch < '0' || ch > '9'))
				ch = ' ';
			else
				lastChar = c;

			lcd.writeChar(8 + c * 6, 8 + 15, ch);
		}
		lcd.writeChar(8 + lastChar * 6 + 6, 8 + 15, '?');

		lcd.update();
	}

	if(key == FL_KEY)
	{
		return FN_CANCEL;
	}
	else if(key == FR_KEY)
	{
		eeprom_write_byte((uint8_t*)&stored[itemSelected].Name[0], 255);
		if(itemSelected > 0) itemSelected--;
    	menu.message(TEXT("Deleted"));
		menu.back();
		return FN_CANCEL;
	}
	else
	{
		return FN_CONTINUE;
	}
}


volatile char shutter_rename(char key, char first)
{
	static char name[MENU_NAME_LEN - 1];
	uint8_t c;

	if(first)
	{
		for(c = 0; c < MENU_NAME_LEN - 1; c++)
		{
			name[c] = eeprom_read_byte((uint8_t*)&stored[itemSelected].Name[c]);
		}
	}

	char ret = menu.editText(key, name, TEXT("Rename"), first);

	if(ret == FN_SAVE)
	{
		name[MENU_NAME_LEN - 2] = 0;
		for(c = 0; c < MENU_NAME_LEN - 1; c++)
		{
			eeprom_write_byte((uint8_t*)&stored[itemSelected].Name[c], name[c]);
		}
		menu.message(TEXT("Renamed"));
		menu.back();
	}

	return ret;
}

