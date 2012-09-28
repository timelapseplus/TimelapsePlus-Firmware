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

unsigned char I2C_Buf[4];
#define I2C_ADDR  0b1000100

char system_tested EEMEM;

volatile unsigned char showGap = 0;
volatile unsigned char showGapQuick = 0;
volatile unsigned char timerNotRunning = 1;
volatile unsigned char timerQuickNotRunning = 1;
volatile unsigned char modeHDR = 0;
volatile unsigned char modeTimelapse = 1;
volatile unsigned char modeStandard = 1;
volatile unsigned char modeRamp = 0;
volatile unsigned char modeRampKeyAdd = 0;
volatile unsigned char modeRampKeyDel = 0;
volatile unsigned char modeBulb = 0;
volatile unsigned char bulb1 = 0;
volatile unsigned char bulb2 = 0;
volatile unsigned char bulb3 = 0;
volatile unsigned char bulb4 = 0;

volatile unsigned char connectUSBcamera = 0;

extern settings conf;

extern char Camera_Connected;

shutter timer = shutter();
LCD lcd = LCD();
MENU menu = MENU();
Clock clock = Clock();
Button button = Button();
BT bt = BT();
IR ir = IR();

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

    _setIn(4, E);
    _setHigh(4, E);

    if(battery_status() == 0 && getPin(BUTTON_FL_PIN)) // Looks like it's connected to the programmer
    {
        lcd.init();
        termInit();
        termPrintStr(TEXT("\n  Programming\n  Successful\n\n  Move to test\n  station\n"));
        wdt_disable();
        hardware_off();
    }

    hardware_init();
    settings_init();

    clock.init();

    lcd.init();

    menu.lcd = &lcd;
    menu.button = &button;

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    menu.init((menu_item*)menu_main);

    lcd.update();

    VirtualSerial_Init();

    // Configure I2C //
    TWI_Master_Initialise();

    bt.init();
    bt.sleep();
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

    //eeprom_write_byte((uint8_t *) &system_tested, 0x01);

    if(eeprom_read_byte((const uint8_t*)&system_tested) == 0xFF || eeprom_read_byte((const uint8_t*)&system_tested) == 0x00 || button.get() == RIGHT_KEY)
    {
        if(test())
        {
            eeprom_write_byte((uint8_t*)&system_tested, 0x01);
        }
        wdt_enable(WDTO_8S);

        for(;;) ;
    }

    if(battery_status() > 0) 
        hardware_off(); // If it was just plugged in, show the charging screen
    
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

        timerNotRunning = !timer.running;
        timerQuickNotRunning = !timer.quick.running;
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
        showGapQuick = timer.quick.Photos > 1;

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
            }
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

        clock.sleepOk = timerNotRunning && timerQuickNotRunning && !timer.cableIsConnected();
        clock.sleep();
    }
}

/******************************************************************
 *
 *   IRTest
 *
 *
 ******************************************************************/

volatile char IRtest(char key, char first)
{
    if(first)
    {
        lcd.cls();
        menu.setTitle(TEXT("IR Test"));
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
//    cable = 0;
        lcd.cls();
        menu.setTitle(TEXT("Shutter Lag Test"));
        menu.setBar(TEXT("Test 1"), TEXT("Test 2 "));
        lcd.update();
    }
    
    if(key == FL_KEY || key == FR_KEY)
    {
        lcd.eraseBox(10, 18, 80, 38);
        lcd.writeString(10, 18, TEXT("Result:"));

        ENABLE_SHUTTER;
        ENABLE_MIRROR;
        ENABLE_AUX_INPUT;

        _delay_ms(100);

        if(key == FR_KEY)
        {
            MIRROR_UP;
            _delay_ms(1000);
        }

        SHUTTER_OPEN;
        clock.tare();

        while (!AUX_INPUT)
        {
            if(clock.eventMs() >= 1000) break;
        }

        start_lag = (uint16_t)clock.eventMs();

        _delay_ms(50);

        SHUTTER_CLOSE;
        clock.tare();

        while (AUX_INPUT)
        {
            if(clock.eventMs() > 1000) break;
        }

        end_lag = (uint16_t)clock.eventMs();

        lcd.writeNumber(56, 18, start_lag, 'U', 'L');
        lcd.writeNumber(56, 28, end_lag, 'U', 'L');

        lcd.update();
    }

    if(key == LEFT_KEY) 
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
    uint16_t batt_high = 645;
    uint16_t batt_low = 500;

    unsigned int batt_level = battery_read_raw();

#define BATT_LINES 36

    char stat = battery_status();
    
    uint8_t lines = ((batt_level - batt_low) * BATT_LINES) / (batt_high - batt_low);

    if(lines > BATT_LINES - 1 && stat == 1) 
        lines = BATT_LINES - 1;
    
    if(lines > BATT_LINES || stat == 2) 
        lines = BATT_LINES;

    lcd.cls();

    char *text;
    
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
    char *text;

    if(first)
    {}

    lcd.cls();
    
    text = getChargingStatus();

    char l = lcd.measureStringTiny(text);
    
    lcd.writeStringTiny(80 - l, 6 + SY, text);
    lcd.writeStringTiny(3, 6 + SY, TEXT("USB:"));

    char buf[6];
    uint16_t val;

    val = (uint16_t)battery_read();
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
/*
volatile char timerStatus(char key, char first)
{
  if(first)
  {
  }

  lcd.cls();

  char buf[6];
  uint16_t val;

//
//06 Time remaining
//12 Time to next photo
//18 Next bulb
//24 Status
//30 Battery %


  val = clock.seconds;
  int_to_str(val, buf);
  text = buf;
  l = lcd.measureStringTiny(text);
  lcd.writeStringTiny(80 - l, 6 + SY, text);
  lcd.writeStringTiny(3, 6 + SY, TEXT("Time Left:"));

  val = clock.seconds;
  int_to_str(val, buf);
  text = buf;
  l = lcd.measureStringTiny(text);
  lcd.writeStringTiny(80 - l, 12 + SY, text);
  lcd.writeStringTiny(3, 12 + SY, TEXT("Next Photo:"));

  val = clock.seconds;
  int_to_str(val, buf);
  text = buf;
  l = lcd.measureStringTiny(text);
  lcd.writeStringTiny(80 - l, 18 + SY, text);
  lcd.writeStringTiny(3, 18 + SY, TEXT("Next Exp:"));

  text = TEXT("Waiting");
  l = lcd.measureStringTiny(text);
  lcd.writeStringTiny(80 - l, 24 + SY, text);
  lcd.writeStringTiny(3, 24 + SY, TEXT("Status:"));

  val = (uint16_t) battery_read();
  int_to_str(val, buf);
  text = buf;
  l = lcd.measureStringTiny(text);
  lcd.writeStringTiny(80 - l, 30 + SY, text);
  lcd.writeStringTiny(3, 30 + SY, TEXT("Battery Level:"));

  menu.setTitle(TEXT("Running"));
  menu.setBar(TEXT("OPTIONS"), TEXT("LIVE EDIT"));
  lcd.update();
  _delay_ms(10);

  if(key == FL_KEY || key == LEFT_KEY) return FN_CANCEL; else return FN_CONTINUE;
}
*/

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
        char *text;
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

        while (version)
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
        
        char buf[6], l;
        uint16_t val;
        char *text;

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
 *   int_to_str
 *
 *
 ******************************************************************/

void int_to_str(uint16_t n, char buf[6])
{
	char digits[6];

    // If it is zero just set and forget
	if(n == 0)
	{
		buf[0] = '0';
		buf[1] = 0;
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
 *   usbPlug
 *
 *
 ******************************************************************/

volatile char usbPlug(char key, char first)
{
    static char connected = 0;

    if(first || (Camera_Connected != connected))
    {
        connected = Camera_Connected;
        
        if(Camera_Connected)
        {
            lcd.cls();
            lcd.writeString(3, 7, TEXT(" Connected!  "));
            lcd.writeString(3, 15, TEXT("             "));
            lcd.writeString(3, 23, TEXT("             "));
            lcd.writeString(3, 31, TEXT("             "));
            menu.setTitle(TEXT("Camera Info"));
            menu.setBar(TEXT("RETURN"), BLANK_STR);
            lcd.update();
            connectUSBcamera = 1;
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
    
    if(key)
    {
        if(!Camera_Connected) 
            connectUSBcamera = 0;
        
        return FN_CANCEL;
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
        menu.message(TEXT("Timer Started"), first);
        _delay_ms(800);
        timer.begin();
        
        return FN_CANCEL;
    } 

    menu.push();
    menu.select(0);
    menu.init((menu_item*)menu_options);
    lcd.update();
    
    return FN_CANCEL;
}

/******************************************************************
 *
 *   runHandlerQuick
 *
 *
 ******************************************************************/

volatile char runHandlerQuick(char key, char first)
{
    if(first)
    {
        if(key != FR_KEY) 
            return FN_CANCEL;
        
        key = 0;
    }
    
    return notYet(key, first);
}

/******************************************************************
 *
 *   timerStop
 *
 *
 ******************************************************************/

volatile char timerStop(char key, char first)
{
    if(first) 
        timer.running = 0;
    
    if(menu.message(TEXT("Stopped"), first))
    {
        menu.back();
        return FN_CANCEL;
    }
    
    return FN_CONTINUE;
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
        menu.back();
    
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
        timer.save(0);
    
    if(menu.message(TEXT("Saved"), first))
    {
        menu.back();
        return FN_CANCEL;
    }
    
    return FN_CONTINUE;
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
    
    if(menu.message(TEXT("Saved"), first))
    {
        menu.back();
        return FN_CANCEL;
    }
    
    return FN_CONTINUE;
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
    
    if(menu.message(TEXT("Reverted"), first))
    {
        menu.back();
        return FN_CANCEL;
    }

    return FN_CONTINUE;
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
            menu.message(TEXT("No Space"), 1);
            _delay_ms(500);
            
            return FN_CANCEL;
        }
    }
    
    char ret = menu.editText(key, name, TEXT("Save As"), first);

    if(ret == FN_SAVE)
    {
        name[MENU_NAME_LEN - 2] = 0;
        strcpy((char*)timer.current.Name, name);
        timer.save(newId);
        menu.message(TEXT("Saved"), 1);
        _delay_ms(200);
        menu.back();
    }
    
    return ret;
}

/******************************************************************
 *
 *   shutter_load
 *
 *
 ******************************************************************/

volatile char shutter_load(char key, char first)
{
    static char menuSize;
    static char menuSelected;
    static char itemSelected;
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
                if(i >= menuScroll && i <= menuScroll + 4)
                {
                    ch = eeprom_read_byte((uint8_t*)&stored[i - 1].Name[c]);

                    if(ch < 'A' || ch > 'Z') 
                        ch = ' ';

                    lcd.writeChar(3 + c * 6, 8 + 9 * (menuSize - menuScroll), ch);

                    if(menuSize == menuSelected) 
                        itemSelected = i - 1;
                }
            }
            menuSize++;
        }

        lcd.drawHighlight(2, 7 + 9 * (menuSelected - menuScroll), 81, 7 + 9 * (menuSelected - menuScroll) + 8);

        menu.setTitle(TEXT("Load Saved"));
        menu.setBar(TEXT("CANCEL"), TEXT("LOAD"));

        lcd.drawLine(0, 3, 0, 40);
        lcd.drawLine(83, 3, 83, 40);

        lcd.update();
    }

    switch(key)
    {
       case FL_KEY:
       case LEFT_KEY:
           return FN_CANCEL;
           
       case FR_KEY:
       case RIGHT_KEY:
            timer.load(itemSelected);
            menu.message(TEXT("Loaded"), 1);
            _delay_ms(200);
            menu.back();
            menu.select(0);
            return FN_SAVE;
    }

    return FN_CONTINUE;
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

