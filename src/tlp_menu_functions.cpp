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
//#include "AVRS_logo.h"
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
#include "light.h"
#include "nmx.h"
#include "tlp_menu_functions.h"

volatile uint8_t showGap = 0;
volatile uint8_t timerNotRunning = 1;
volatile uint8_t modeHDR = 0;
volatile uint8_t modeTimelapse = 1;
volatile uint8_t modeStandard = 1;
volatile uint8_t modeStandardExp = 1;
volatile uint8_t modeStandardExpNikon = 0;
volatile uint8_t modeStandardExpArb = 0;
volatile uint8_t modeRamp = 0;
volatile uint8_t modeRampNormal = 0;
volatile uint8_t modeRampExtended = 0;
volatile uint8_t modeNoRamp = 1;
volatile uint8_t modeBulb = 0;
volatile uint8_t showRemoteStart = 0;
volatile uint8_t showRemoteInfo = 0;
volatile uint8_t brampKeyframe = 0;
volatile uint8_t brampGuided = 0;
volatile uint8_t brampAuto = 0;
volatile uint8_t showIntervalMaxMin = INTERVAL_MODE_FIXED;
volatile uint8_t rampISO = 0;
volatile uint8_t rampAperture = 0;
volatile uint8_t rampTargetCustom = 0;
volatile uint8_t showFocus = 0;

volatile uint8_t brampNotAuto = 0;
volatile uint8_t brampNotGuided = 0;

volatile uint8_t cameraMakeNikon = 0;

extern uint8_t battery_percent;
extern settings_t conf;
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
extern Light light;
extern NMX motor1;
extern NMX motor2;
extern NMX motor3;

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

	modeStandardExp = modeStandard && (conf.camera.cameraMake != NIKON) && !conf.arbitraryBulb;
	modeStandardExpNikon = modeStandard && (conf.camera.cameraMake == NIKON) && !conf.arbitraryBulb;
	modeStandardExpArb = modeStandard && conf.arbitraryBulb;

	modeRamp = (timer.current.Mode & RAMP);
	modeRampNormal = modeRamp && !conf.extendedRamp;
	modeRampExtended = modeRamp && conf.extendedRamp;
	modeNoRamp = !modeRamp && modeTimelapse;
	brampAuto = timer.current.brampMethod == BRAMP_METHOD_AUTO && modeRamp;
	brampGuided = timer.current.brampMethod == BRAMP_METHOD_GUIDED && modeRamp;
	brampKeyframe = timer.current.brampMethod == BRAMP_METHOD_KEYFRAME && modeRamp;
	showGap = timer.current.Photos != 1 && modeTimelapse && (timer.current.IntervalMode == INTERVAL_MODE_FIXED || modeNoRamp);
	showIntervalMaxMin = timer.current.Photos != 1 && modeTimelapse && modeRamp && timer.current.IntervalMode == INTERVAL_MODE_AUTO;
	showRemoteStart = (remote.connected && !remote.running && remote.model == REMOTE_MODEL_TLP);	
	showRemoteInfo = (remote.connected && (remote.model == REMOTE_MODEL_TLP || remote.model == REMOTE_MODEL_IPHONE));
	clock.sleepOk = timerNotRunning && !timer.cableIsConnected() && bt.state != BT_ST_CONNECTED && bt.state != BT_ST_CONNECTED_NMX && sleepOk;
	brampNotGuided = modeRamp && !brampGuided;
	brampNotAuto = modeRamp && !brampAuto && !light.underThreshold;
	rampISO = (conf.brampMode & BRAMP_MODE_ISO && camera.supports.iso);
	rampAperture = (conf.brampMode & BRAMP_MODE_APERTURE && camera.supports.aperture);
	rampTargetCustom = (timer.current.nightMode == BRAMP_TARGET_CUSTOM && brampAuto);
	cameraMakeNikon = conf.camera.cameraMake == NIKON;
	showFocus = conf.focusEnabled && camera.supports.focus;
	if(modeRamp && timer.current.Gap < BRAMP_INTERVAL_MIN)
	{
		timer.current.Gap = BRAMP_INTERVAL_MIN;
		menu.refresh();
	}
	if(modeRamp && (timer.current.GapMin < BRAMP_INTERVAL_VAR_MIN))
	{
		timer.current.GapMin = BRAMP_INTERVAL_VAR_MIN;
		menu.refresh();
	}
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

		lcd.writeStringTiny(13, 10, PTEXT("Successfully"));
		lcd.writeStringTiny(25, 16, PTEXT("Updated"));

		lcd.writeStringTiny(8, 28, PTEXT("Version:"));
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

		lcd.writeStringTiny(2, 8, PTEXT(" Please go to the "));
		lcd.writeStringTiny(2, 14, PTEXT(" Settings Menu and"));
		lcd.writeStringTiny(2, 20, PTEXT(" set Camera Make  "));
		lcd.writeStringTiny(2, 26, PTEXT(" and Camera FPS   "));
		lcd.writeStringTiny(2, 32, PTEXT(" before using     "));

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
 *   Videoremote
 *
 *
 ******************************************************************/

volatile char videoRemote(char key, char first)
{
	lcd.cls();
	menu.setTitle(TEXT("Video"));
	
	if(!camera.videoMode)
	{
		lcd.writeString(84 / 2 - (13 * 6) / 2, 14, PTEXT("Set Camera to"));
		lcd.writeString(84 / 2 - (10 * 6) / 2, 22, PTEXT("video mode"));
		menu.setBar(TEXT("Return"), BLANK_STR);
	}
	else if(camera.modeLiveView)
	{
		if(camera.recording)
		{
			lcd.writeString(84 / 2 - (9 * 6) / 2, 16, PTEXT("Recording"));
			menu.setBar(TEXT("Return"), TEXT("Stop"));
		}
		else
		{
			menu.setBar(TEXT("Return"), TEXT("Start"));
		}
	}
	else
	{
		menu.setBar(TEXT("Return"), TEXT("LiveView"));
	}
	lcd.update();

	switch(key)
	{
	   case FR_KEY:
	   		if(camera.modeLiveView)
	   		{
		   		if(camera.recording)
		   		{
		   			camera.videoStop();
		   		}
		   		else
		   		{
		   			camera.videoStart();
		   		}
	   		}
	   		else
	   		{
	   			if(camera.videoMode) camera.liveView(true);
	   		}
			break;

	   case FL_KEY:
	   case LEFT_KEY:
		   return FN_CANCEL;
	}

	return FN_CONTINUE;
}


/******************************************************************
 *
 *   Videoremote
 *
 *
 ******************************************************************/

volatile char videoRemoteBT(char key, char first)
{
	if(first)
	{
		remote.watch(REMOTE_VIDEO);
		remote.watch(REMOTE_LIVEVIEW);
	}

	lcd.cls();
	menu.setTitle(TEXT("Remote Video"));
	
	if(remote.modeLiveView)
	{
		if(remote.recording)
		{
			lcd.writeString(84 / 2 - (9 * 6) / 2, 16, PTEXT("Recording"));
			menu.setBar(TEXT("Return"), TEXT("Stop"));
		}
		else
		{
			menu.setBar(TEXT("Return"), TEXT("Start"));
		}
	}
	else
	{
		menu.setBar(TEXT("Return"), TEXT("LiveView"));
	}
	lcd.update();

	switch(key)
	{
	   case FR_KEY:
	   		if(remote.modeLiveView)
	   		{
		   		if(remote.recording)
		   		{
		   			remote.set(REMOTE_VIDEO, false);
		   		}
		   		else
		   		{
		   			remote.set(REMOTE_VIDEO, true);
		   		}
	   		}
	   		else
	   		{
	   			remote.set(REMOTE_LIVEVIEW, true);
	   		}
			break;

	   case FL_KEY:
	   case LEFT_KEY:
	   {
			remote.unWatch(REMOTE_VIDEO);
			remote.unWatch(REMOTE_LIVEVIEW);
			return FN_CANCEL;

	   }
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
		lcd.writeString(20, 18, PTEXT("(HALF)"));
		timer.half();
		lcd.update();
	}
	else if(key == FR_KEY && status != 2)
	{
		status = 2;
		lcd.eraseBox(20, 18, 20 + 6 * 6, 26);
		lcd.writeString(20, 18, PTEXT("(FULL)"));
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
			lcd.writeStringTiny(6, 28, PTEXT("Cable Connected"));
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
			lcd.writeString(8, 18, PTEXT("(BULB OPEN)"));
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
			lcd.writeStringTiny(6, 28, PTEXT("Cable Connected"));
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
			lcd.writeString(8, 18, PTEXT("(BULB OPEN)"));
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
 *   autoConfigureCameraTiming
 *
 *
 ******************************************************************/

volatile char autoConfigureCameraTiming(char key, char first)
{
	uint16_t start_lag, end_lag;

	if(first)
	{
		lcd.cls();
		menu.setTitle(TEXT("Auto Configure"));

		if(!conf.camera.autoConfigured && camera.ready)
		{
			lcd.writeStringTiny(2, 8,  PTEXT(" Camera requires  "));
			lcd.writeStringTiny(2, 14, PTEXT(" calibration for  "));
			lcd.writeStringTiny(2, 20, PTEXT(" Bramping. Please "));
			lcd.writeStringTiny(2, 26, PTEXT(" Connect PC sync  "));
			lcd.writeStringTiny(2, 32, PTEXT(" and press run... "));
			menu.setBar(TEXT("Later"), TEXT("Run"));
		}
		else
		{
			lcd.writeStringTiny(2, 10, PTEXT(" Connect PC Sync  "));
			lcd.writeStringTiny(2, 16, PTEXT(" cable and USB and"));
			lcd.writeStringTiny(2, 22, PTEXT(" or shutter cable "));
			lcd.writeStringTiny(2, 28, PTEXT(" before continuing"));
			menu.setBar(TEXT("Cancel"), TEXT("Continue"));
		}
		lcd.update();
	}

	if(key == FR_KEY)
	{
		lcd.cls();
		menu.setTitle(TEXT("Auto Configure"));
		lcd.writeStringTiny(2, 14, PTEXT(" Running Test...  "));
		lcd.writeStringTiny(2, 20, PTEXT("   Please Wait    "));

		menu.setBar(BLANK_STR, BLANK_STR);
		lcd.update();

		uint8_t pass = 1;
		ENABLE_SHUTTER;
		ENABLE_MIRROR;
		ENABLE_AUX_PORT1;
		ENABLE_AUX_PORT2;

		_delay_ms(100);

		if(key == FR_KEY)
		{
			MIRROR_UP;
			_delay_ms(1000);
		}
		
		#define SHUTTER_TEST_COUNT 8
		int16_t bOffsetArray[SHUTTER_TEST_COUNT];
		uint16_t eLagArray[SHUTTER_TEST_COUNT];

		if(AUX_INPUT1) pass = 0;
		for(uint8_t i = 0; i < SHUTTER_TEST_COUNT && pass; i++)
		{
			wdt_reset();
			while(AUX_INPUT1)
			{
				if(clock.eventMs() > 1000)
				{
					pass = 0;
					//menu.message(STR("1"));
					break;
				}
			}

			if(pass) // start bulb
			{
				_delay_ms(1000);
				do {
					shutter_bulbStart();  // start bulb
					clock.tare();
				} while(lastShutterError);

				while(!AUX_INPUT1)
				{
					if(clock.eventMs() > 1000)
					{
						pass = 0;
						//menu.message(STR("2"));
						break;
					}
				}

				start_lag = (uint16_t)clock.eventMs();
				DEBUG(PSTR("SL:"));
				DEBUG(start_lag);
				DEBUG_NL();
			}
			if(pass) // end bulb
			{
				clock.tare();
				shutter_bulbEnd();

				while(AUX_INPUT1)
				{
					if(clock.eventMs() > 1000)
					{
						pass = 0;
						//menu.message(STR("3"));
						break;
					}
				}

				end_lag = (uint16_t)clock.eventMs();
			}		

			if(pass)
			{
				lcd.cls();
				menu.setTitle(TEXT("Auto Configure"));
				lcd.writeStringTiny(2, 14, PTEXT(" Running Test...  "));
				lcd.writeStringTiny(2, 20, PTEXT(" Waiting on Camera"));
				menu.setBar(BLANK_STR, BLANK_STR);
				lcd.update();
		
				camera.checkEvent();
				if(camera.busy)
				{
					clock.tare();
					camera.checkEvent();
					while(camera.busy)
					{
						camera.checkEvent();
						wdt_reset();
						if(clock.eventMs() > 12000)
						{
							break;
						}
					}
					uint16_t bGap = (uint16_t)((clock.eventMs() + 500) / 1000) + 1;
					if(i == 0 || conf.camera.brampGap < bGap) conf.camera.brampGap = bGap;
				}
				_delay_ms(1000);
			}

			uint32_t bulbMinMs;
			int8_t bulbMin;
			// retest at bulb min
			if(pass)
			{
				lcd.cls();
				menu.setTitle(TEXT("Auto Configure"));
				lcd.writeStringTiny(2, 14, PTEXT(" Running Test...  "));
				lcd.writeStringTiny(2, 20, PTEXT(" Testing bulb min "));
				menu.setBar(BLANK_STR, BLANK_STR);
				lcd.update();

				// find bulb min
				bulbMin = camera.bulbMinStatic();
				float minTime = (float)start_lag - (float)end_lag;
				if((float)end_lag > minTime) minTime = (float)end_lag;
				while(minTime >= ((float)camera.bulbTime(bulbMin)) * 1.05)
				{
					bulbMin = camera.bulbDown(bulbMin);
				}
				bulbMinMs = camera.bulbTime(bulbMin) - 8;

				uint32_t tmpMs;
				do {
					shutter_bulbStart();  // start bulb
					clock.tare();
				} while(lastShutterError);
				tmpMs = clock.eventMs();

				uint16_t bGap = (uint16_t)((tmpMs + 500) / 1000) + 1;
				if(conf.camera.brampGap < bGap) conf.camera.brampGap = bGap;

				while(!AUX_INPUT1)
				{
					if(clock.eventMs() > 1000)
					{
						pass = 0;
						//menu.message(STR("4"));
						break;
					}
				}

				start_lag = (uint16_t)clock.eventMs();
			}
			if(pass) // end bulb
			{
				while(clock.eventMs() < bulbMinMs);
				clock.tare();
				shutter_bulbEnd();

				while(AUX_INPUT1)
				{
					if(clock.eventMs() > 1000)
					{
						pass = 0;
						//menu.message(STR("5"));
						break;
					}
				}

				end_lag = (uint16_t)clock.eventMs();
			}

			// check busy time for bramp gap

			if(pass)
			{
				lcd.cls();
				menu.setTitle(TEXT("Auto Configure"));
				lcd.writeStringTiny(2, 14, PTEXT(" Running Test...  "));
				lcd.writeStringTiny(2, 20, PTEXT(" Waiting on Camera"));
				menu.setBar(BLANK_STR, BLANK_STR);
				lcd.update();
				if(camera.busy)
				{
					clock.tare();
					camera.checkEvent();
					while(camera.busy)
					{
						camera.checkEvent();
						wdt_reset();
						if(clock.eventMs() > 12000)
						{
							break;
						}
					}
					uint16_t bGap = (uint16_t)((clock.eventMs() + 500) / 1000) + 1;
					if(conf.camera.brampGap < bGap) conf.camera.brampGap = bGap;
				}

				if(i == 0 || conf.camera.bulbMin > bulbMin) conf.camera.bulbMin = bulbMin;

				bOffsetArray[i] = (int16_t)start_lag - (int16_t)end_lag;
				eLagArray[i] = end_lag;
			}
			else
			{
				shutter_bulbEnd();
			}
		} // end of retry loop

		int16_t tmp_offset = arrayMedian50Int(bOffsetArray, SHUTTER_TEST_COUNT);
		if(tmp_offset < 0)
		{
			conf.camera.negBulbOffset = 1;
			conf.camera.bulbOffset = (uint16_t) (0 - tmp_offset);
		}
		else
		{
			conf.camera.negBulbOffset = 0;
			conf.camera.bulbOffset = tmp_offset;
		}
		conf.camera.bulbEndOffset = arrayMedian50UInt(eLagArray, SHUTTER_TEST_COUNT);

		uint16_t eLagMax = eLagArray[0], eLagMin = eLagArray[0];
		for(uint8_t i = 0; i < SHUTTER_TEST_COUNT && pass; i++)
		{
			if(eLagMax < eLagArray[i]) eLagMax = eLagArray[i];
			if(eLagMin > eLagArray[i]) eLagMin = eLagArray[i];
		}
		uint16_t eRange = eLagMax - eLagMin;
		float errorF = (float)eRange / camera.bulbTime((int8_t)conf.camera.bulbMin);
		eRange = (uint16_t) (errorF * 100.0);

		lcd.cls();
		if(pass)
		{
			conf.camera.autoConfigured = 1;
			settings_update();
		
			menu.setTitle(TEXT("Configuration"));

			lcd.eraseBox(10, 8, 80, 38);
			lcd.writeString(2,  8, PTEXT("BrampGap:"));
			lcd.writeString(2, 18, PTEXT(" BulbMin:"));
			lcd.writeString(2, 28, PTEXT(" Error %:"));

			char buf[8];
			camera.shutterName(buf, conf.camera.bulbMin);
			lcd.writeString(58, 18, &buf[3]); // Bulb Length

			lcd.writeNumber(58, 8, conf.camera.brampGap, 'U', 'L', false);
			//lcd.writeNumber(58, 18, conf.camera.bulbMin, 'U', 'L', false);
			lcd.writeNumber(58, 28, eRange, 'U', 'L', false);

			menu.setBar(TEXT("Done"), TEXT("Retest"));
		}
		else
		{
			menu.setTitle(TEXT("Test Failed"));
			lcd.writeStringTiny(2, 12,  PTEXT(" Failed to read PC"));
			lcd.writeStringTiny(2, 18, PTEXT(" sync cable. Check"));
			lcd.writeStringTiny(2, 24, PTEXT(" cables and try"));
			lcd.writeStringTiny(2, 30, PTEXT(" again."));
			//lcd.writeStringTiny(2, 32, PTEXT("                  "));
			menu.setBar(TEXT("Cancel"), TEXT("Retest"));
		}
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
		lcd.writeString(1, 18, PTEXT("Free RAM:"));
		/*char x =*/lcd.writeNumber(55, 18, mem, 'U', 'L',false);   //J.R.
		//lcd.writeString(55 + x * 6, 18, PTEXT("b"));
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
		lcd.writeString(14, 12, PTEXT("Reset all"));
		lcd.writeString(14, 22, PTEXT("settings?"));
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
		lcd.writeString(1, 18, PTEXT("Clock:"));
		menu.setTitle(TEXT("Clock"));
		menu.setBar(TEXT("TARE"), TEXT("RETURN"));
	}

	lcd.eraseBox(36, 18, 83, 18 + 8);
	/*char x =*/ lcd.writeNumber(83, 18, clock.Seconds(), 'F', 'R',false);  //J.R.
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
	lcd.writeStringTiny(3, 6 + SY, PTEXT("USB:"));

	char buf[6];
	uint16_t val;

	val = (uint16_t)battery_percent;
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 12 + SY, text);
	lcd.writeStringTiny(3, 12 + SY, PTEXT("Battery:"));

	val = hardware_freeMemory();
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 18 + SY, text);
	lcd.writeStringTiny(3, 18 + SY, PTEXT("Free RAM:"));

	val = clock.seconds;
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 24 + SY, text);
	lcd.writeStringTiny(3, 24 + SY, PTEXT("Clock s:"));

	val = clock.ms;
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 30 + SY, text);
	lcd.writeStringTiny(3, 30 + SY, PTEXT("Clock ms:"));

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
	//static uint8_t counter;

	if(modeRamp)
	{
		return bramp_monitor(key, first);
	}
	else
	{
		//if(first)
		//{
		//	counter = 0;
		//}

		//if(counter++ > 3)
		//{
			//counter = 0;
			lcd.cls();

			displayTimerStatus(0);

			menu.setTitle(TEXT("Running"));
			menu.setBar(TEXT("OPTIONS"), TEXT("STOP"));
			lcd.update();
		//}

		if(!timer.running) return FN_CANCEL;

		if(key == FR_KEY)
		{
			menu.push();
			menu.spawn((void*)timerStop);
			return FN_JUMP;
		}
		if(key == FL_KEY)
		{
			menu.push(1);
			menu.submenu((void*)menu_timelapse_options);
		}
		if(key == LEFT_KEY)
		{
			return backToMain(key, first);
		}

		return FN_CONTINUE;
	}
}

volatile char backToMain(char key, char first)
{
	menu.clearStack();
	menu.init((menu_item*)menu_main);
	menu.refresh();
	lcd.update();
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
			lcd.writeStringTiny(3, 6 + SY, PTEXT("Photos:"));

			val = stat.photosRemaining;
			if(stat.infinitePhotos == 0)
			{
				int_to_str(val, buf);
				text = buf;
				l = lcd.measureStringTiny(text);
				lcd.writeStringTiny(80 - l, 12 + SY, text);
				lcd.writeStringTiny(3, 12 + SY, PTEXT("Photos rem:"));
			}
			else
			{
				lcd.writeStringTiny(3, 12 + SY, PTEXT("Infinite Photos"));
			}

			val = stat.nextPhoto;
			int_to_str(val, buf);
			text = buf;
			l = lcd.measureStringTiny(text);
			lcd.writeStringTiny(80 - l, 18 + SY, text);
			lcd.writeStringTiny(3, 18 + SY, PTEXT("Next Photo:"));
		}
	}
	
	text = stat.textStatus;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 24 + SY, text);
	lcd.writeStringTiny(3, 24 + SY, PTEXT("Status:"));

	if(remote_system)
		val = (uint16_t) remote.battery;
	else
		val = (uint16_t) battery_percent;
	int_to_str(val, buf);
	text = buf;
	l = lcd.measureStringTiny(text);
	lcd.writeStringTiny(80 - l, 30 + SY, text);
	lcd.writeStringTiny(3, 30 + SY, PTEXT("Battery Level:"));

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
		lcd.writeStringTiny(3, 6 + SY, PTEXT("Model:"));

		if(val > 1)
			text = TEXT("BTLE");
		else
			text = TEXT("KS99");

		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 12 + SY, text);
		lcd.writeStringTiny(3, 12 + SY, PTEXT("Edition:"));

		lcd.writeStringTiny(3, 18 + SY, PTEXT("Firmware:"));
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
			lcd.writeStringTiny(3, 30 + SY, PTEXT("BT FW Version:"));
		}

		val = (uint16_t)battery_percent;
		int_to_str(val, buf);
		text = buf;
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 24 + SY, text);
		lcd.writeStringTiny(3, 24 + SY, PTEXT("Battery:"));

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
		light.start();
		light.integrationStart(1);
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
		lcd.writeStringTiny(80 - l, 6 + SY, text);
		lcd.writeStringTiny(3, 6 + SY, PTEXT("Level 1:"));

		val = (uint16_t)hardware_readLight(1);
		int_to_str(val, buf);
		text = buf;
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 12 + SY, text);
		lcd.writeStringTiny(3, 12 + SY, PTEXT("Level 2:"));

		val = (uint16_t)hardware_readLight(2);
		int_to_str(val, buf);
		text = buf;
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 18 + SY, text);
		lcd.writeStringTiny(3, 18 + SY, PTEXT("Level 3:"));

		val = (uint16_t)(light.readEv());
		int_to_str(val, buf);
		text = buf;
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 24 + SY, text);
		lcd.writeStringTiny(3, 24 + SY, PTEXT("    I2C:"));

		val = (uint16_t)(light.slope);
		int_to_str(val, buf);
		text = buf;
		l = lcd.measureStringTiny(text);
		lcd.writeStringTiny(80 - l, 30 + SY, text);
		lcd.writeStringTiny(3, 30 + SY, PTEXT("Slope:"));

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
		light.stop();
		lcd.backlight(255);
		return FN_CANCEL;
	}

	return FN_CONTINUE;
}


/******************************************************************
 *
 *   lightTrigger
 *
 *
 ******************************************************************/

#define YOFF (-6)

volatile char lightTrigger(char key, char first)
{
	uint16_t val;
	static uint16_t lv;
	static uint8_t threshold = 2, mode = LIGHT_TRIGGER_MODE_ANY, i = 0;

	if(!mode) mode = LIGHT_TRIGGER_MODE_ANY;

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
	if(key == UP_KEY)
	{
		if(mode == LIGHT_TRIGGER_MODE_FALL)
			mode = LIGHT_TRIGGER_MODE_RISE;
		else if(mode == LIGHT_TRIGGER_MODE_RISE)
			mode = LIGHT_TRIGGER_MODE_ANY;
		else
			mode = LIGHT_TRIGGER_MODE_FALL;
	}
	if(key == DOWN_KEY)
	{
		if(mode == LIGHT_TRIGGER_MODE_ANY)
			mode = LIGHT_TRIGGER_MODE_RISE;
		else if(mode == LIGHT_TRIGGER_MODE_RISE)
			mode = LIGHT_TRIGGER_MODE_FALL;
		else
			mode = LIGHT_TRIGGER_MODE_ANY;
	}

	if(first || key)
	{
		sleepOk = 0;
		clock.tare();
		lcd.cls();
		if(mode == LIGHT_TRIGGER_MODE_ANY)
			menu.setTitle(TEXT("Motion Sensor"));
		else if(mode == LIGHT_TRIGGER_MODE_RISE)
			menu.setTitle(TEXT("Light Trigger"));
		else
			menu.setTitle(TEXT("Light Trigger"));

		menu.setBar(TEXT("RETURN"), BLANK_STR);

		lcd.drawLine(10, 22+YOFF, 84-10, 22+YOFF);
		lcd.drawLine(11, 21+YOFF, 11, 23+YOFF);
		lcd.drawLine(84-11, 21+YOFF, 84-11, 23+YOFF);
		lcd.drawLine(12, 20+YOFF, 12, 24+YOFF);
		lcd.drawLine(84-12, 20+YOFF, 84-12, 24+YOFF);
		lcd.drawLine(13, 20+YOFF, 13, 24+YOFF);
		lcd.drawLine(84-13, 20+YOFF, 84-13, 24+YOFF);
		lcd.setPixel(42, 21+YOFF);
		lcd.setPixel(42+10, 21+YOFF);
		lcd.setPixel(42-10, 21+YOFF);
		lcd.setPixel(42+20, 21+YOFF);
		lcd.setPixel(42-20, 21+YOFF);

		i = threshold * 10;
		lcd.drawLine(42-3-20+i, 16+YOFF, 42+3-20+i, 16+YOFF);
		lcd.drawLine(42-2-20+i, 17+YOFF, 42+2-20+i, 17+YOFF);
		lcd.drawLine(42-1-20+i, 18+YOFF, 42+1-20+i, 18+YOFF);
		lcd.setPixel(42-20+i, 19+YOFF);

		lcd.writeStringTiny(19, 25+YOFF, PTEXT("SENSITIVITY"));

		lcd.setPixel(13, 29);
		lcd.drawLine(12, 30, 14, 30);
		lcd.drawLine(11, 31, 15, 31);

		lcd.setPixel(13, 35);
		lcd.drawLine(12, 34, 14, 34);
		lcd.drawLine(11, 33, 15, 33);

		if(mode == LIGHT_TRIGGER_MODE_FALL)
		{
			lcd.drawBox(17, 28, 18 + 2 + lcd.measureStringTiny(PTEXT("FALLING EDGE")), 36);
			lcd.writeStringTiny(19, 30, PTEXT("FALLING EDGE"));
		}
		else if (mode == LIGHT_TRIGGER_MODE_RISE)
		{
			lcd.drawBox(17, 28, 18 + 2 + lcd.measureStringTiny(PTEXT("RISING EDGE")), 36);
			lcd.writeStringTiny(19, 30, PTEXT("RISING EDGE"));
		}
		else
		{
			lcd.drawBox(17, 28, 18 + 2 + lcd.measureStringTiny(PTEXT("ANY CHANGE")), 36);
			lcd.writeStringTiny(19, 30, PTEXT("ANY CHANGE"));
		}


		lcd.update();
		lcd.backlight(0);
		hardware_flashlight(0);
		_delay_ms(50);
		for(i = 3; i > 0; i--)
		{
			lv = (uint16_t)hardware_readLight(i - 1);
			if(lv < 512) break;
		}
		i--;
	}

	uint8_t thres = 4 - threshold + 2;
	if((4 - threshold) > 2) thres += ((4 - threshold) - 1) * 2;

	if(key == FL_KEY)
	{
		sleepOk = 1;
		lcd.backlight(255);
		return FN_CANCEL;
	}

	while(!button.pressed())
	{
		val = (uint16_t)hardware_readLight(i);
		if(clock.eventMs() > 1000 && val > thres && ((val < (lv - thres) && (mode & LIGHT_TRIGGER_MODE_FALL)) || (val > (lv + thres) && (mode & LIGHT_TRIGGER_MODE_RISE))))
		{
			shutter_capture();
			clock.tare();
		}
		lv = val;
		wdt_reset();
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
		lcd.writeString(3, 7, PTEXT("Sorry, this  "));
		lcd.writeString(3, 15, PTEXT("feature has  "));
		lcd.writeString(3, 23, PTEXT("not yet been "));
		lcd.writeString(3, 31, PTEXT("implemented  "));
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
 *   focusStack
 *
 *
 ******************************************************************/

volatile char focusStack(char key, char first)
{
	static uint8_t state, photos = 3, photosTaken, wait, fine = 0;
	static int8_t start = 0, end = 0, pos = 0;

	if(state == 0)
	{
		state = 1;
		wait = 0;
		pos = start;
		if(camera.modeLiveView) first = true;
		//camera.liveView(true);
	}

	if(key) first = 1; //redraw

	if(photosTaken >= photos)
	{
		if(cameraMakeNikon) camera.liveView(false);	
		menu.message(TEXT("Done!"));
		photosTaken = 0;
		first = 1;
		state = 1;
	}

	if(!camera.modeLiveView && !cameraMakeNikon)
	{
		if(first || state > 0)
		{
			lcd.cls();
			lcd.writeStringTiny(2, 15, PTEXT("Please set camera"));
			lcd.writeStringTiny(5, 23, PTEXT("to LiveView mode"));

			menu.setTitle(TEXT("Focus Stack"));
			menu.setBar(TEXT("RETURN"), TEXT("LIVEVIEW"));
			lcd.update();
		}
		state = 0;
		if(key == FR_KEY)
		{
			camera.liveView(true);
		}
		if(key == LEFT_KEY)
		{
			state = 0;
			return FN_CANCEL;
		}
	}
	else if(state < 7)
	{
		if(key == LEFT_KEY)
		{
			if(state > 1) state--;
		}
		if(key == RIGHT_KEY)
		{
			if(state < 6) state++;
		}
		if(key == UP_KEY)
		{
			if(state == 1 && photos < 90) photos += 10;
			if(state == 2 && photos < 99) photos += 1;

			if(state == 3 && start < 90) start += 10;
			if(state == 4 && start < 99) start += 1;

			if(state == 5 && end < 90) end += 10;
			if(state == 6 && end < 99) end += 1;
		}
		if(key == DOWN_KEY)
		{
			if(state == 1 && photos > 10) photos -= 10;
			if(state == 2 && photos > 2) photos -= 1;

			if(state == 3 && start > -90) start -= 10;
			if(state == 4 && start > -99) start -= 1;

			if(state == 5 && end > -90) end -= 10;
			if(state == 6 && end > -99) end -= 1;
		}
		if(key == FR_KEY)
		{
			if(state < 3)
			{
				if(fine) fine = 0; else fine = 1;
			}
			else
			{
				key = 0;
				photosTaken = 0;
				state = 7; // start
			}
		}

		if(first)
		{
			lcd.cls();
			lcd.writeStringTiny(9, 12, PTEXT("Steps Start End"));

			lcd.writeChar(12, 25-4, '0' + photos / 10);
			lcd.writeChar(19, 25-4, '0' + photos % 10);

			int8_t tmp = start;
			if(tmp < 0)
			{
				tmp = 0 - tmp;
				lcd.setPixel(32, 28-4);
				lcd.setPixel(33, 28-4);
				lcd.setPixel(34, 28-4);
			}
			lcd.writeChar(36, 25-4, '0' + tmp / 10);
			lcd.writeChar(43, 25-4, '0' + tmp % 10);

			tmp = end;
			if(tmp < 0)
			{
				tmp = 0 - tmp;
				lcd.setPixel(56, 28-4);
				lcd.setPixel(57, 28-4);
				lcd.setPixel(58, 28-4);
			}
			lcd.writeChar(60, 25-4, '0' + tmp / 10);
			lcd.writeChar(67, 25-4, '0' + tmp % 10);

			if(state == 1) lcd.drawHighlight(12, 24-4, 18, 32-4);
			if(state == 2) lcd.drawHighlight(19, 24-4, 25, 32-4);
			if(state == 3) lcd.drawHighlight(36, 24-4, 42, 32-4);
			if(state == 4) lcd.drawHighlight(43, 24-4, 49, 32-4);
			if(state == 5) lcd.drawHighlight(60, 24-4, 66, 32-4);
			if(state == 6) lcd.drawHighlight(67, 24-4, 73, 32-4);

			menu.setTitle(TEXT("Focus Stack")); // 9204:A00B
			uint8_t len;
			if(fine)
			{
				len = lcd.measureStringTiny(PTEXT("Fine Steps"));
				lcd.writeStringTiny(3, 35-3, PTEXT("Fine Steps"));
			}
			else
			{
				len = lcd.measureStringTiny(PTEXT("Course Steps"));
				lcd.writeStringTiny(3, 35-3, PTEXT("Course Steps"));
			}
			if(state < 3)
			{
				if(fine)
				{
					len = lcd.measureStringTiny(PTEXT("Fine Steps"));
					lcd.writeStringTiny(3, 35-3, PTEXT("Fine Steps"));
					menu.setBar(TEXT("RETURN"), TEXT("COURSE"));
				}
				else
				{
					len = lcd.measureStringTiny(PTEXT("Course Steps"));
					lcd.writeStringTiny(3, 35-3, PTEXT("Course Steps"));
					menu.setBar(TEXT("RETURN"), TEXT("FINE"));
				}
				lcd.drawHighlight(2, 31, 3 + len, 37);
			}
			else
			{

				menu.setBar(TEXT("RETURN"), TEXT("START"));
			}
			lcd.update();
		}
	}
	if(state == 7) // Running
	{
		if(cameraMakeNikon) camera.liveView(true);

		float percent = (float) photosTaken /  (float) photos;
		uint8_t bar = (uint8_t) (56.0 * percent) + 12;

		lcd.cls();

		lcd.drawBox(12, 12, 84 - 12, 20);
		lcd.drawHighlight(14, 14, bar, 18);

		lcd.writeStringTiny(25, 25, PTEXT("Running"));
		menu.setTitle(TEXT("Focus Stack"));
		menu.setBar(BLANK_STR, TEXT("CANCEL"));
		lcd.update();
	}

	int16_t dest;
	if(state < 5)
	{
		dest = start;
	}
	else if(state < 7)
	{
		dest = end;
	}
	else // running
	{
		float length = (float) (start - end);
		float percent = (float) photosTaken /  (float) photos;
		dest = start - (int8_t) (length * percent);
		if(photosTaken == photos - 1) dest = end;
	}
	//dest *= 10;

	if(!camera.busy && wait <= 0)
	{
		if(pos != dest)
		{
			uint8_t move;
			int16_t steps = dest - pos;
			if(steps < 0)
			{
				steps = 0 - steps;
				if(fine) move = +1; else move = +2;
			}
			else
			{
				if(fine) move = -1; else move = -2;
			}
			//camera.setFocus(true);
			_delay_ms(100);
			camera.moveFocus(move, steps);
			pos = dest;
		}

		if(state == 7)
		{
			camera.setFocus(false);
			_delay_ms(100);
			camera.capture();
			photosTaken++;
			wait = 10;
		}
	}
	else if(!camera.busy && wait > 0)
	{
		_delay_ms(50);
		wait--;
	}

	if(key == FR_KEY && state == 7)
	{
		if(cameraMakeNikon) camera.liveView(false);	
		menu.message(TEXT("Cancelled"));
		state = 1;
	}
	if(key == FL_KEY && state < 7)
	{
		if(cameraMakeNikon) camera.liveView(false);	
		state = 0;
		//camera.liveView(false);
		return FN_CANCEL;
	}

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
		if(bt.state != BT_ST_CONNECTED && bt.state != BT_ST_CONNECTED_NMX && !remote.nmx)
		{
			DEBUG(STR("BT Advertising!\r\n"));
			bt.advertise();
			bt.scan();
		}
	}

	switch(key)
	{
		case UP_KEY:
			//if(bt.state == BT_ST_CONNECTED_NMX)
			//{
			//	motor.enable();
			//	motor.moveForward();
			//}
			//break;
		case DOWN_KEY:
			//if(bt.state == BT_ST_CONNECTED_NMX)
			//{
			//	motor.enable();
			//	motor.moveBackward();
			//}
			break;
		case LEFT_KEY:
		case FL_KEY:
			sfirst = 1;
			if(bt.state != BT_ST_CONNECTED && bt.state != BT_ST_CONNECTED_NMX && !remote.nmx)
			{
				if(conf.btMode == BT_MODE_SLEEP) bt.sleep(); else bt.advertise();
			}
			return FN_CANCEL;

		case FR_KEY:
			if(bt.state == BT_ST_CONNECTED || bt.state == BT_ST_CONNECTED_NMX || remote.nmx)
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
			DEBUG(STR("dicovery!\r\n"));
			break;
		case BT_EVENT_SCAN_COMPLETE:
			DEBUG(STR("done!\r\n"));
			if(bt.state != BT_ST_CONNECTED && bt.state != BT_ST_CONNECTED_NMX && !remote.nmx)
			{
				bt.advertise();
				bt.scan();
			}
			break;
		case BT_EVENT_DISCONNECT:		
			bt.scan();
		default:
			update = 1;
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

		if(bt.state == BT_ST_CONNECTED || bt.state == BT_ST_CONNECTED_NMX || remote.nmx)
		{
			menu.setTitle(TEXT("Connect"));
			lcd.writeStringTiny(18, 20, PTEXT("Connected!"));
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
				lcd.writeStringTiny(6, 20, PTEXT("No Devices Found"));
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
extern uint32_t modePTP;
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
				lcd.writeString(3, 7,  PTEXT(" USB Error!  "));

				lcd.writeChar(3+1*6, 15, '(');
				char b, *c = (char*)&PTP_Error;
				b = (c[1] >> 4) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+2*6, 15, b);
				b = (c[1] & 0x0F) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+3*6, 15, b);
				b = (c[0] >> 4) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+4*6, 15, b);
				b = (c[0] & 0x0F) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+5*6, 15, b);

				lcd.writeChar(3+6*6, 15, ':');

				c = (char*)&PTP_Response_Code;
				b = (c[1] >> 4) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+7*6, 15, b);
				b = (c[1] & 0x0F) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+8*6, 15, b);
				b = (c[0] >> 4) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+9*6, 15, b);
				b = (c[0] & 0x0F) + '0'; if(b > '9') b += 7;
				lcd.writeChar(3+10*6, 15, b);

				lcd.writeChar(3+11*6, 15, ')');

				if(timer.running)
				{
					lcd.writeString(3, 23, PTEXT("Attempting to"));
					lcd.writeString(3, 31, PTEXT("self-correct..."));
					menu.setTitle(TEXT("Alert"));
					menu.setBar(TEXT("RETURN"), BLANK_STR);
				}
				else
				{
					lcd.writeString(3, 23, PTEXT("Unplug camera"));
					lcd.writeString(3, 31, PTEXT("to reset...  "));
					menu.setTitle(TEXT("Camera Info"));
					menu.setBar(TEXT("RETURN"), TEXT("RESET"));
					connectUSBcamera = 1;
				}
				lcd.update();
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
					lcd.writeString(3+46, 31, PTEXT("ISO"));
				}

				//uint8_t tmp = (uint8_t) modePTP;
				//lcd.writeCharTiny(84 - 8 + 4, 48-13, '0' + tmp % 10);
				//tmp /= 10;
				//lcd.writeCharTiny(84 - 8, 48-13, '0' + tmp % 10);

				//tmp = camera.isInBulbMode();
				//lcd.writeCharTiny(84 - 8 - 4, 48-13, '0' + tmp % 10);

				menu.setTitle(TEXT("Camera Info"));
				menu.setBar(TEXT("RETURN"), TEXT("PHOTO"));
				lcd.update();
				connectUSBcamera = 1;

			}
			else
			{
				lcd.cls();
				lcd.writeString(3, 7,  PTEXT(" Connected!  "));
				lcd.writeString(3, 15, PTEXT(" Retrieving  "));
				lcd.writeString(3, 23, PTEXT("   Device    "));
				lcd.writeString(3, 31, PTEXT("   Info...   "));
				menu.setTitle(TEXT("Camera Info"));
				menu.setBar(TEXT("RETURN"), BLANK_STR);
				lcd.update();
				connectUSBcamera = 1;

			}
		}
		else
		{
			lcd.cls();
			lcd.writeString(3, 7, PTEXT("Plug camera  "));
			lcd.writeString(3, 15, PTEXT("into left USB"));
			lcd.writeString(3, 23, PTEXT("port...      "));
			lcd.writeString(3, 31, PTEXT("             "));
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
		if(PTP_Ready)
		{
			camera.capture();
		}
		else if(PTP_Error && !timer.running)
		{
			camera.resetConnection();
		}
	}
	else if(key == UP_KEY)
	{
//		camera.setFocus(false);
//		if(PTP_Ready) camera.moveFocus(1);
		if(PTP_Ready) camera.setISO(camera.isoUp(camera.iso()));
//		if(PTP_Ready) camera.setShutter(camera.shutterUp(camera.shutter()));
	}
	else if(key == DOWN_KEY)
	{
//		camera.setFocus(true);
//		if(PTP_Ready) camera.moveFocus(0x8001);
		if(PTP_Ready) camera.setISO(camera.isoDown(camera.iso()));
//		if(PTP_Ready) camera.setShutter(camera.shutterDown(camera.shutter()));
	}
	else if(key == RIGHT_KEY)
	{
		//remote.send(REMOTE_THUMBNAIL, REMOTE_TYPE_SEND);
		uint8_t *file = (uint8_t *) STR("Test file contents");
		char *name = STR("test.txt");
		camera.writeFile(name, file, 19);
	}

	return FN_CONTINUE;
}

/******************************************************************
 *
 *   lightningTrigger
 *
 *
 ******************************************************************/

volatile char lightningTrigger(char key, char first)
{
	if(first)
	{
		sleepOk = 0;
		hardware_lightning_enable();
		lcd.cls();
		menu.setTitle(TEXT("Lightning"));
		lcd.writeString(25, 20, PTEXT("READY"));
		menu.setBar(TEXT("RETURN"), TEXT("CALIBRATE"));
		lcd.update();
	}

	if(key == FL_KEY || key == LEFT_KEY)
	{
		sleepOk = 1;
		hardware_lightning_disable();
		return FN_CANCEL;
	}
	if(key == FR_KEY)
	{
		menu.message(TEXT("Calibrating"));
		hardware_lightning_disable();
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
		//menu.message(TEXT("Timer Started"));
		timer.begin();
		menu.spawn((void*)timerStatus);
	}
	else
	{
		menu.push(0);
		menu.submenu((void*)menu_options);
	}

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

	light.paused = 1;
	menu.message(TEXT("Stopped"));
	menu.back();
	return FN_CANCEL;
}

/******************************************************************
 *
 *   timerStop
 *		Stops intervalometer if currently running
 *
 ******************************************************************/

volatile char timerToGuided(char key, char first)
{
	menu.message(TEXT("Guided Bramp"));
	timer.switchToGuided();
	menu.back();
	return FN_CANCEL;
}

/******************************************************************
 *
 *   timerStop
 *		Stops intervalometer if currently running
 *
 ******************************************************************/

volatile char timerToAuto(char key, char first)
{
	menu.message(TEXT("Auto Bramp"));
	timer.switchToAuto();	
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
 ******************************************************************

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

 ******************************************************************
 *
 *   shutter_removeKeyframe
 *
 *
 ******************************************************************

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

 ******************************************************************
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

		lcd.writeString(15, 8 + 5, PTEXT("Delete"));
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

volatile char system_name(char key, char first)
{
	static char name[MENU_NAME_LEN - 1];
	if(first) memcpy(name, conf.sysName, MENU_NAME_LEN - 1);
	char ret = menu.editText(key, name, TEXT("Sys Name"), first);

	if(ret == FN_SAVE)
	{
		for(uint8_t i = MENU_NAME_LEN - 2; i > 0; i--)
		{
			if((name[i-1] >= '0' && name[i-1] <= '9') || (name[i-1] >= 'A' && name[i-1] <= 'Z'))
			{
				name[i] = '\0';
				break;
			}
		}
		memcpy(conf.sysName, name, MENU_NAME_LEN - 1);
		settings_update();
		menu.message(TEXT("Saved"));
	}
	else
	{
		settings_load();
	}

	return ret;
}


volatile char bramp_monitor(char key, char first)
{
	#define CHART_Y_TOP (18 + 1)
	#define CHART_Y_BOTTOM (39 - 1)
	#define CHART_Y_SPAN (CHART_Y_BOTTOM - CHART_Y_TOP)
	#define CHART_X_TOP 2
	#define CHART_X_BOTTOM 51 + 1
	#define CHART_X_SPAN (CHART_X_BOTTOM - CHART_X_TOP)

	#define LOCK_AFTER 4

    static uint8_t rampHistory[CHART_X_SPAN], skip_message = 0;
    static uint32_t lock_time;

    if(clock.Seconds() - lock_time > LOCK_AFTER && !timer.paused && timer.status.preChecked == 0)
    {
    	light.paused = 0;
		lcd.backlight(0);
    	lock_time = clock.Seconds();
    }
    if(key)
    {
    	lock_time = clock.Seconds();
    }

	char buf[8];
	uint8_t waiting = strcmp(timer.status.textStatus, STR("Delay")) == 0;
	if(timer.status.preChecked == 0)
	{
		lcd.cls();

		lcd.writeStringTiny(2, 1, PTEXT("BULB RAMP"));

		lcd.drawHighlight(0, 0, 53, 6);

		// Outline //
		lcd.drawLine(0, 0, 0, 47);
		lcd.drawLine(0, 0, 83, 0);
		lcd.drawLine(83, 0, 83, 47);
		lcd.drawLine(0, 47, 83, 47);

		// Grid //
		lcd.drawLine(28, 7, 28, 15);
		lcd.drawLine(1, 16, 53, 16);
		lcd.drawLine(53, 7, 53, 41);
		lcd.drawLine(61, 1, 61, 41);
		lcd.drawLine(61, 20, 83, 20);
		lcd.drawLine(61, 28, 83, 28);
		lcd.drawLine(61, 29, 83, 29);

		// Battery //
		lcd.drawLine(30, 9, 30, 14);
		lcd.drawLine(31, 8, 31, 14);
		lcd.drawLine(32, 8, 32, 14);
		lcd.drawLine(33, 9, 33, 14);

		int16_t b = (uint16_t)battery_percent;
		if(b > 99) b = 99;
		buf[1] = b % 10 + '0';
		b /= 10;
		buf[0] = b % 10 + '0';
		buf[2] = '%';
		buf[3] = '\0';
		lcd.writeString(34, 8, buf); // Battery Level

		if(timer.current.brampMethod == BRAMP_METHOD_GUIDED)// || timer.rampRate != 0)
		{
			b = (int16_t)timer.rampRate;
			if(b > 99) b = 99;
			if(b < -99) b = -99;

			if(b > 0)
			{
				buf[0] = '+';
			}
			else if(b < 0)
			{
				b = 0 - b;
				buf[0] = '-';
			}
			else
			{
				buf[0] = ' ';
			}

			buf[2] = '0' + b % 10;
			b /= 10;
			buf[1] = '0' + b;
			if(buf[1] == '0') buf[1] = ' ';
			buf[3] = '\0';

			lcd.writeString(1, 8, buf); // Current Bramp Rate (stops/hour)

			// Up/Down //
			if(timer.status.rampStops < timer.status.rampMax || timer.rampRate < 0)
			{
				lcd.setPixel(23, 8);
				lcd.drawLine(22, 9, 24, 9);
				lcd.drawLine(21, 10, 25, 10);
			}
			if(timer.status.rampStops > timer.status.rampMin || timer.rampRate > 0)
			{
				lcd.drawLine(21, 12, 25, 12);
				lcd.drawLine(22, 13, 24, 13);
				lcd.setPixel(23, 14);
			}
		}
		else if(timer.current.brampMethod == BRAMP_METHOD_KEYFRAME)
		{
			lcd.writeString(1, 8, STR("KeyF"));
		}
		else if(timer.current.brampMethod == BRAMP_METHOD_AUTO)
		{
			lcd.writeString(1, 8, STR("Auto"));
		}

        if((conf.brampMode & BRAMP_MODE_APERTURE) && camera.supports.aperture)
		{
			camera.apertureName(buf, camera.aperture());
			for(b = 0; b < (int16_t)sizeof(buf); b++)
			{
				if(buf[b] == 'f')
				{
					buf[b + 1] = 'f';
					break;
				}
			}
			lcd.writeStringTiny(63, 2, &buf[b + 1]); // Aperture
		}
        if((conf.brampMode & BRAMP_MODE_ISO) && camera.supports.iso)
		{
			camera.isoName(buf, camera.iso());
			lcd.writeStringTiny(63, 2+12, &buf[2]); // ISO
		}
		if(clock.usingSync)
		{
			lcd.writeStringTiny(56, 2, STR("S"));
		}
		camera.bulbName(buf, timer.status.bulbLength);
		lcd.writeStringTiny(63, 2+6, &buf[3]); // Bulb Length

		float f = timer.status.rampStops;
		if(f > 0.0)
		{
			buf[0] = '+';
		}
		else if(f < 0.0)
		{
			f = 0.0 - f;
			buf[0] = '-';
		}
		else
		{
			buf[0] = ' ';
		}
		b = (uint16_t) (f * 10.0 / 3.0);
		if(b > 999) b = 999;
		if(b < -999) b = -999;
		buf[4] = '0' + b % 10;
		b /= 10;
		buf[3] = '.';
		buf[2] = '0' + b % 10;
		b /= 10;
		buf[1] = '0' + b % 10;
		buf[5] = '\0';
		lcd.writeStringTiny(63, 22, buf); // Ramp Stops from start

		b = timer.status.photosTaken;
		if(b > 999) b = 999;
		buf[2] = '0' + b % 10;
		b /= 10;
		buf[1] = '0' + b % 10;
		b /= 10;
		buf[0] = '0' + b % 10;
		buf[3] = '\0';
		lcd.writeString(63, 32, buf); // Photos Taken

		// Interval Bar //
		#define INT_BAR_TOP 1
		#define INT_BAR_BOTTOM 40
		#define INT_BAR_SPAN (INT_BAR_BOTTOM - INT_BAR_TOP)

		uint8_t top = INT_BAR_BOTTOM - (uint8_t)((float)INT_BAR_SPAN / ((float)timer.status.interval / (float)(timer.status.bulbLength / 100))); // Exposure Bar

		lcd.drawLine(56, top, 56, 41);
		lcd.drawLine(57, top, 57, 41);
		lcd.drawLine(58, top, 58, 41);

		// Interval Position //
		top = INT_BAR_BOTTOM - (uint8_t)((float)INT_BAR_SPAN / ((float)timer.status.interval / (float)((clock.Ms() - timer.last_photo_ms) / 100))); // Position Markers

		if(timer.running)
		{
			lcd.drawLine(54, top, 54, top+2);
			lcd.drawLine(60, top, 60, top+2);
			lcd.setPixel(55, top+1);
			lcd.setPixel(59, top+1);
			lcd.xorPixel(57, top+1);
		}



		// Plot Chart //
		if(timer.current.brampMethod == BRAMP_METHOD_KEYFRAME)
		{
			for(uint8_t x = 0; x < CHART_X_SPAN; x++)
			{
				uint32_t s = (uint32_t)(((float)timer.current.Duration / (float)CHART_X_SPAN) * (float)x * 60.0); //J.R.
	            float ev = interpolateKeyframe(&timer.current.kfExposure, s);

	            int8_t y = ((ev / (float)(timer.status.rampMax - timer.status.rampMin)) * (float)CHART_Y_SPAN);

				if(y >= 0 && y <= CHART_Y_SPAN) lcd.setPixel(x + CHART_X_TOP, CHART_Y_SPAN + CHART_Y_TOP - y);
			}
		}
		else if(timer.current.brampMethod == BRAMP_METHOD_GUIDED || timer.current.brampMethod == BRAMP_METHOD_AUTO)
		{
			uint8_t x = 0, x2;
			uint32_t s, completedS = 0; //J.R.
			
			if(!waiting)
			{
				for(x = 0; x < CHART_X_SPAN; x++)
				{
					s = (uint32_t)(((float)timer.current.Duration / (float)CHART_X_SPAN) * (float)x * 60.0); //J.R.

					if(s >= clock.Seconds()) rampHistory[x] = ((((float)timer.status.rampStops - (float)timer.status.rampMin) / (float)(timer.status.rampMax - timer.status.rampMin)) * (float)CHART_Y_SPAN);

		            lcd.setPixel(x + CHART_X_TOP, CHART_Y_SPAN + CHART_Y_TOP - rampHistory[x]);
					
					if(s >= clock.Seconds()) break; 
				}
				completedS = s;
			}
			x2 = x;
			for(x++; x < CHART_X_SPAN; x++)
			{
				s = (uint32_t)(((float)timer.current.Duration / (float)CHART_X_SPAN) * (float)x * 60.0); //J.R.

				s -= completedS;

				float futureRamp = timer.status.rampStops + ((float)timer.rampRate / (3600.0 / 3)) * (float)s;

				//if(timer.current.brampMethod == BRAMP_METHOD_AUTO && futureRamp > timer.status.rampTarget) break;

				int16_t y = ((((float)futureRamp - (float)timer.status.rampMin) / (float)(timer.status.rampMax - timer.status.rampMin)) * (float)CHART_Y_SPAN);

				if(y < 0) y = 0;
				if(y > CHART_Y_SPAN) y = CHART_Y_SPAN;

				lcd.setPixel(x + CHART_X_TOP, CHART_Y_SPAN + CHART_Y_TOP - y);
			}
			x = x2;
			float intSlope = 0.0 - light.readIntegratedSlope();
			if(timer.running)
			{
				for(x++; x < CHART_X_SPAN; x += 2)
				{
					s = (uint32_t)(((float)timer.current.Duration / (float)CHART_X_SPAN) * (float)x * 60.0); //J.R.

					s -= completedS;

					float futureRamp = timer.status.rampStops + ((intSlope) / (3600.0 / 3)) * (float)s;

					int16_t y = ((((float)futureRamp - (float)timer.status.rampMin) / (float)(timer.status.rampMax - timer.status.rampMin)) * (float)CHART_Y_SPAN);

					if(y < 0) y = 0;
					if(y > CHART_Y_SPAN) y = CHART_Y_SPAN;

					lcd.setPixel(x + CHART_X_TOP, CHART_Y_SPAN + CHART_Y_TOP - y);
				}
			}
		}
		// Progress Bar //
		if(!waiting)
		{
			static float lastSec;
			if(timer.running) lastSec = (float)clock.Seconds();
			uint8_t x = (uint8_t)(((float)lastSec / ((float)timer.current.Duration * 60.0)) * (float)(CHART_X_SPAN + 1));  //J.R.
			if(x > CHART_X_SPAN + 1) x = CHART_X_SPAN + 1;
			lcd.drawHighlight(CHART_X_TOP - 1, CHART_Y_TOP - 1, x + CHART_X_TOP, CHART_Y_BOTTOM + 1);
		}

		if(!timer.running)
		{
			if(!skip_message)
			{
		        uint8_t l = strlen(timer.status.textStatus) * 6 / 2;
		        lcd.eraseBox(41 - l - 2, 12, 41 + l + 2, 24);
		        lcd.drawBox(41 - l - 1, 13, 41 + l + 1, 23);
		        lcd.writeString(41 - l, 15, timer.status.textStatus);
			}
			menu.setBar(TEXT("RETURN"), BLANK_STR);
		}
		else 
		{
			skip_message = 0;
			if(waiting)
			{
				menu.setBar(TEXT("CANCEL"), BLANK_STR);
			}
			else if(!light.paused)
			{
				menu.setBar(TEXT("PRESS ANY KEY"), BLANK_STR);
			}
			else
			{
				if(timer.paused)
				{
					menu.setBar(TEXT("OPTIONS"), TEXT("RUN"));
				}
				else
				{
					menu.setBar(TEXT("OPTIONS"), TEXT("PAUSE"));
				}
			}
			char message_text[13];
			if(timer.paused)
			{
		        if(!timer.apertureReady)
		        {
		        	if(key == UP_KEY)
		        	{
		        		timer.apertureEvShift--;
		        	}
		        	else if(key == DOWN_KEY)
		        	{
		        		timer.apertureEvShift++;
		        	}
		        }
				if(timer.apertureReady || timer.apertureEvShift)
				{
					strcpy(message_text, STR("Aperture +/-"));
			        uint8_t l = strlen(message_text) * 6 / 2;
			        lcd.eraseBox(41 - l - 2, 12, 41 + l + 2, 24 + 10);
			        lcd.drawBox(41 - l - 1, 13, 41 + l + 1, 23 + 10);
			        lcd.writeString(41 - l, 15, message_text);
			        stopName(message_text, (0 - timer.apertureEvShift));
			        l = 8 * 6 / 2;
			        lcd.writeString(41 - l, 15 + 10, message_text);
				}
				else if(timer.pausing)
				{
					strcpy(message_text, STR("Starting..."));
			        uint8_t l = strlen(message_text) * 6 / 2;
			        lcd.eraseBox(41 - l - 2, 12, 41 + l + 2, 24);
			        lcd.drawBox(41 - l - 1, 13, 41 + l + 1, 23);
			        lcd.writeString(41 - l, 15, message_text);
				}
				else
				{
					strcpy(message_text, STR("PAUSED"));
			        uint8_t l = strlen(message_text) * 6 / 2;
			        lcd.eraseBox(41 - l - 2, 12, 41 + l + 2, 24);
			        lcd.drawBox(41 - l - 1, 13, 41 + l + 1, 23);
			        lcd.writeString(41 - l, 15, message_text);
				}
			}
			else if(waiting)
			{
				strcpy(message_text, STR("Waiting ("));
				if(timer.status.nextPhoto < 100) int_to_str(timer.status.nextPhoto, buf); else buf[0] = '\0';
				strcat(message_text, buf);
				strcat(message_text, STR(")"));
		        uint8_t l = strlen(message_text) * 6 / 2;
		        lcd.eraseBox(41 - l - 2, 12, 41 + l + 2, 24);
		        lcd.drawBox(41 - l - 1, 13, 41 + l + 1, 23);
		        lcd.writeString(41 - l, 15, message_text);
			}
			else if(timer.pausing)
			{
				strcpy(message_text, STR("WAIT"));
		        uint8_t l = strlen(message_text) * 6 / 2;
		        lcd.eraseBox(41 - l - 2, 12, 41 + l + 2, 24);
		        lcd.drawBox(41 - l - 1, 13, 41 + l + 1, 23);
		        lcd.writeString(41 - l, 15, message_text);
			}
			else if(camera.ready && conf.extendedRamp && !camera.isInBulbMode() && timer.status.bulbLength > camera.bulbTime((int8_t)camera.bulbMin()))
			{
				strcpy(message_text, STR("Turn to BULB"));
		        uint8_t l = strlen(message_text) * 6 / 2;
		        lcd.eraseBox(41 - l - 2, 12, 41 + l + 2, 24);
		        lcd.drawBox(41 - l - 1, 13, 41 + l + 1, 23);
		        lcd.writeString(41 - l, 15, message_text);
			}
			else if(camera.ready && conf.extendedRamp && camera.isInBulbMode() && timer.status.bulbLength <= camera.bulbTime((int8_t)camera.bulbMin()) - camera.bulbTime((int8_t)camera.bulbMin()) / 3)
			{
				strcpy(message_text, STR("Turn to M"));
		        uint8_t l = strlen(message_text) * 6 / 2;
		        lcd.eraseBox(41 - l - 2, 12, 41 + l + 2, 24);
		        lcd.drawBox(41 - l - 1, 13, 41 + l + 1, 23);
		        lcd.writeString(41 - l, 15, message_text);
			}
		}

		lcd.update();
	}
	else
	{
	    if(timer.status.preChecked == 2 && (timer.current.Mode & RAMP) && (timer.current.brampMethod == BRAMP_METHOD_AUTO))
	    {
			light.paused = 1;

	        lcd.cls();
	        menu.setTitle(TEXT("BRAMP TARGET"));
	        menu.setBar(TEXT("BACK"), TEXT("START"));

	        #define XALIGN 50
	        uint8_t l;
	        char *s;
	        s = TEXT("Aperture:");
	        l = lcd.measureStringTiny(s);
	        lcd.writeStringTiny(XALIGN - l, 7, s);
	        s = TEXT("Shutter:");
	        l = lcd.measureStringTiny(s);
	        lcd.writeStringTiny(XALIGN - l, 14, s);
	        s = TEXT("ISO:");
	        l = lcd.measureStringTiny(s);
	        lcd.writeStringTiny(XALIGN - l, 21, s);
	        s = TEXT("OTHER:");
	        l = lcd.measureStringTiny(s);
	        lcd.writeStringTiny(XALIGN - l, 28, s);

	        uint32_t bulb_length;
	        float otherEv = 0;

	        if(timer.status.rampTarget > timer.status.rampMax)
	        {
		        bulb_length = camera.bulbTime((float)timer.current.BulbStart - timer.status.rampMax);
		        otherEv = timer.status.rampTarget - timer.status.rampMax;
	        }
	        else
	        {
		        bulb_length = camera.bulbTime((float)timer.current.BulbStart - timer.status.rampTarget);
	        }

            uint8_t nextAperture = camera.aperture();
            uint8_t nextISO = camera.iso();
            int8_t evShift = 0;

            timer.calculateExposure(&bulb_length, &nextAperture, &nextISO, &evShift);


            if((conf.brampMode & BRAMP_MODE_APERTURE) && camera.supports.aperture)
            {
				camera.apertureName(buf, nextAperture);
            	uint8_t b;
				for(b = 0; b < (int16_t)sizeof(buf); b++)
				{
					if(buf[b] == 'f')
					{
						buf[b + 1] = 'f';
						break;
					}
				}
		        lcd.writeStringTiny(XALIGN + 2, 7, &buf[b + 1]); // aperture
            }
            else
            {
		        buf[0] = buf[1] = '-';
		        buf[2] = '\0';
		        lcd.writeStringTiny(XALIGN + 2, 7, buf); // no aperture
            }


			camera.bulbName(buf, bulb_length);
	        lcd.writeStringTiny(XALIGN + 2, 14, &buf[3]); // shutter

            if((conf.brampMode & BRAMP_MODE_ISO) && camera.supports.iso)
            {
				camera.isoName(buf, nextISO);
		        lcd.writeStringTiny(XALIGN + 2, 21, &buf[2]); // ISO
            }
            else
            {
		        buf[0] = buf[1] = '-';
		        buf[2] = '\0';
		        lcd.writeStringTiny(XALIGN + 2, 21, buf); // no ISO
            }
	        
	        if(otherEv != 0)
	        {
				float f = otherEv;
				if(f > 0.0)
				{
					buf[0] = '+';
				}
				else if(f < 0.0)
				{
					f = 0.0 - f;
					buf[0] = '-';
				}
				else
				{
					buf[0] = ' ';
				}
				int16_t b = (int16_t) (f * 10.0 / 3.0);
				if(b > 999) b = 999;
				if(b < -999) b = -999;
				buf[4] = '0' + b % 10;
				b /= 10;
				buf[3] = '.';
				buf[2] = '0' + b % 10;
				b /= 10;
				buf[1] = '0' + b % 10;
				buf[5] = '\0';
	        }
	        else
	        {
		        buf[0] = buf[1] = '-';
		        buf[2] = '\0';
	        }
	        lcd.writeStringTiny(XALIGN + 2, 28, buf);

	        #define RSTART 1
	        #define REND 82
	        #define RSPAN (REND - RSTART)
	        #define RLINE_Y 35

	        uint8_t xPos;

	        float chartMaxEv = ((timer.status.rampTarget > timer.status.rampMax) ? timer.status.rampTarget : timer.status.rampMax);
	        float chartRangeEv = chartMaxEv - timer.status.rampMin;
	        float p;

	        xPos = (uint8_t) (RSPAN * ((timer.status.rampMax - timer.status.rampMin)  / chartRangeEv)) + RSTART;
	        lcd.drawLine(RSTART + 0, RLINE_Y, xPos, RLINE_Y);
	        lcd.setPixel(xPos, RLINE_Y + 1);

	        for(uint8_t i = 0; i <= (uint8_t) chartRangeEv / 3; i++)
	        {
		        xPos = (uint8_t) (RSPAN * ((float) i / (chartRangeEv / 3))) + RSTART;
		        if(xPos > RSTART) lcd.xorPixel(xPos, RLINE_Y);
	        }

	        p = (0 - timer.status.rampMin) / chartRangeEv;
	        xPos = (uint8_t) (RSPAN * p) + RSTART;
	        lcd.drawLine(xPos + 0, RLINE_Y + 2, xPos + 0, RLINE_Y + 4);
	        lcd.drawLine(xPos + 1, RLINE_Y + 3, xPos + 1, RLINE_Y + 4);
	        lcd.setPixel(xPos + 2, RLINE_Y + 4);

	        p = (timer.status.rampTarget - timer.status.rampMin) / chartRangeEv;
	        xPos = (uint8_t) (RSPAN * p) + RSTART;
	        lcd.drawLine(xPos - 0, RLINE_Y + 2, xPos - 0, RLINE_Y + 4);
	        lcd.drawLine(xPos - 1, RLINE_Y + 3, xPos - 1, RLINE_Y + 4);
	        lcd.setPixel(xPos - 2, RLINE_Y + 4);

	        lcd.update();

	        if(key == FL_KEY)
	        {
				menu.push(0);
				menu.spawn((void*)timerStop);
				return FN_JUMP;		
	        }
	        else if(key == FR_KEY)
	        {
				light.paused = 0;
				timer.status.preChecked = 3;	        	
	        }

	        return FN_CONTINUE;
	    }
	    else
	    {
	        timer.status.preChecked = 3;
	    }
	}

	if(!timer.running && key != 0) skip_message = 1;

	if(waiting)
	{
		if(key == FL_KEY)
		{
			menu.push(0);
			menu.spawn((void*)timerStop);
			return FN_JUMP;		
		}
	}
	else if(!light.paused && ((key && timer.running) || timer.paused) )
	{
		light.paused = 1;
	}
	else if(key == FL_KEY)
	{
		if(!timer.running)
		{
			return FN_CANCEL;
		}
		else
		{
			menu.push(1);
			menu.submenu((void*)menu_timelapse_options);
		}
	}
	else if(key == FR_KEY && timer.running)
	{
		if(timer.paused)
		{
			timer.pause(0);	
		}
		else
		{
			timer.pause(1);	
		}
	}
	else if(key == UP_KEY && timer.running && timer.current.brampMethod == BRAMP_METHOD_GUIDED)
	{
		if(timer.rampRate < 50 && (timer.status.rampStops < timer.status.rampMax || timer.rampRate < 0)) timer.rampRate++;
	}
	else if(key == DOWN_KEY && timer.running && timer.current.brampMethod == BRAMP_METHOD_GUIDED)
	{
		if(timer.rampRate > -50 && (timer.status.rampStops > timer.status.rampMin || timer.rampRate > 0)) timer.rampRate--;
	}

	return FN_CONTINUE;
}

