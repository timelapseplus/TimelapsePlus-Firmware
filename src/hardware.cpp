/*
 *  hardware.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */
 
#include <avr/io.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "hardware.h"
#include "button.h"
#include "5110LCD.h"
#include "Menu.h"
#include "clock.h"
#include "shutter.h"
#include "timelapseplus.h"
#include "tlp_menu_functions.h"
#include "tldefs.h"
#include "settings.h"
#include "debug.h"
#include "bluetooth.h"
#include "TWI_Master.h"
#include "math.h"
#include <LUFA/Common/Common.h>
#include <LUFA/Drivers/USB/USB.h>
#include <LUFA/Drivers/Peripheral/ADC.h>

extern unsigned int __data_start;
extern unsigned int __data_end;
extern unsigned int __bss_start;
extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

extern LCD lcd;
extern Button button;
extern MENU menu;
extern Clock clock;
extern BT bt;
extern shutter timer;
extern settings_t conf;

uint16_t battery_low_charging EEMEM;
uint16_t battery_high_charging EEMEM;
uint16_t battery_low EEMEM;
uint16_t battery_high EEMEM;

char backlightVal;




/******************************************************************
 *
 *   hardware_init
 *
 *
 ******************************************************************/

void hardware_init(void)
{
    setOut(POWER_HOLD_PIN); // Hold PWR on
    setLow(POWER_HOLD_PIN);

#ifdef POWER_AUX_PIN
    setOut(POWER_AUX_PIN); // Hold AUX PWR on
    setLow(POWER_AUX_PIN);
#endif

#ifdef USB_ENABLE_PIN
    setOut(USB_ENABLE_PIN); // Hold AUX PWR on
    setLow(USB_ENABLE_PIN);
#endif

    setOut(LED_PIN);  // Red Flashlight
    setHigh(LED_PIN); // off
    setOut(IR_PIN);  // IR Transmitter
    setHigh(IR_PIN); // off

    hardware_USB_Enable();

    hardware_USB_SetDeviceMode();
}

/******************************************************************
 *
 *   hardware_off
 *
 *
 ******************************************************************/

void hardware_off(void)
{
    hardware_flashlight(0);
    if(battery_status() == 0)
    {
        //if(timer.cableIsConnected())
        //{
        //    menu.message(STR("Error: Cable"));
        //}
        //else
        //{
            shutter_off();

            // Save the current time-lapse settings to nv-mem
            timer.saveCurrent();

            // If USB is used, detach from the bus
            USB_Detach();

            // Shutdown bluetooth
            bt.disconnect();
            bt.sleep();

            // Disable all interrupts
            cli();

            // Shutdown
            setHigh(POWER_HOLD_PIN);

            FOREVER;
        //}
    } 
    else // Plugged in
    {
        // Charging screen //
        clock.sleeping = 1;
        menu.spawn((void *) batteryStatus);
    }
}

/******************************************************************
 *
 *   hardware_flashlight
 *
 *
 ******************************************************************/

char hardware_flashlight(char on)
{
    setOut(LED_PIN);
    
    if(on)
    {
        setLow(LED_PIN);
        return 1;
    }
    
    setHigh(LED_PIN);
    return 0;
}

/******************************************************************
 *
 *   hardware_flashlightIsOn
 *
 *
 ******************************************************************/

char hardware_flashlightIsOn()
{
    if(isOut(LED_PIN) && !isHigh(LED_PIN)) 
        return 1;

    return 0;
}

/******************************************************************
 *
 *   hardware_flashlight_toggle
 *
 *
 ******************************************************************/

void hardware_flashlight_toggle()
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

/******************************************************************
 *
 *   hardware_freeMemory
 *
 *
 ******************************************************************/

int hardware_freeMemory()
{
    int free_memory;

    if((int)__brkval == 0) 
        free_memory = ((int)&free_memory) - ((int)&__bss_end);
    else 
        free_memory = ((int)&free_memory) - ((int)__brkval);

    return free_memory;
}

/******************************************************************
 *
 *   hardware_readLight
 *
 *
 ******************************************************************/

unsigned int hardware_readLight(uint8_t r)
{
    // Need to power off lights //
    backlightVal = lcd.getBacklight();
    lcd.backlight(0);
    
    if(backlightVal > 0) 
        _delay_ms(50);
    
    hardware_light_enable(r);
    _delay_ms(50);
    
    uint16_t light = hardware_analogRead(0);

    hardware_light_disable();
    
    if(backlightVal > lcd.getBacklight()) 
        lcd.backlight(backlightVal);
    
    return light;
}

/******************************************************************
 *
 *   hardware_light_enable
 *
 *
 ******************************************************************/

void hardware_light_enable(uint8_t level)
{
    hardware_flashlight(0);
        
    if(level > 2) 
        level = 2;
    
    DDRA &= ~0b00000111; // clear all //
    PORTA &= ~0b00000111; // clear all //
    
    setBit(level, DDRA); // Powers Sensor //
    clrBit(level, PORTA);
}

/******************************************************************
 *
 *   hardware_light_disable
 *
 *
 ******************************************************************/

void hardware_light_disable()
{
    DDRA &= ~0b00000111; // clear all //
    PORTA &= ~0b00000111; // clear all //
}

/******************************************************************
 *
 *   hardware_readLightAll
 *
 *
 ******************************************************************/

void hardware_readLightAll(void *result)
{
    light_reading *res = (light_reading*)result;
    res->level1 = hardware_readLight(0);
    res->level2 = hardware_readLight(1);
    res->level3 = hardware_readLight(2);
}

/******************************************************************
 *
 *   battery_read
 *
 *
 ******************************************************************/

uint8_t battery_read() // Returns 0-100 //
{
    uint16_t raw = battery_read_raw();
    uint16_t high = 645, low = 555, percent;
    char status = battery_status();

    if(status == 0)
    {
        /*
        eeprom_read_block((void*)&low, &battery_low, sizeof(uint16_t));
        
        if(raw < low) 
            eeprom_write_block((const void*)&raw, &battery_low, sizeof(uint16_t));
        
        eeprom_read_block((void*)&high, &battery_high, sizeof(uint16_t));
        
        if(high == 0xFFFF) 
            high = 0;
        
        if(raw > high) 
            eeprom_write_block((const void*)&raw, &battery_high, sizeof(uint16_t));
        */
        percent = ((raw - low) * 100) / (high - low);
    } 
    else
    {
        /*
        eeprom_read_block((void*)&low, &battery_low_charging, sizeof(uint16_t));
        
        if(raw < low) 
            eeprom_write_block((const void*)&raw, &battery_low, sizeof(uint16_t));
        
        eeprom_read_block((void*)&high, &battery_high_charging, sizeof(uint16_t));
        
        if(raw > high) 
            eeprom_write_block((const void*)&raw, &battery_high, sizeof(uint16_t));
        */

        percent = ((raw - low) * 100) / (high - low);
        
        if(status == 1 && percent > 99) 
            percent = 99;
        
        if(status == 2) 
            percent = 100;
    }

    if(percent > 100) percent = 100;

    return percent;
}

/******************************************************************
 *
 *   battery_read_raw
 *
 *
 ******************************************************************/

uint16_t battery_read_raw()
{
    setBit(PF1, DDRF); // Powers Sensor //
    clrBit(PF1, PORTF);
    _delay_ms(10);
    uint16_t battery = hardware_analogRead(2);
    clrBit(PF1, DDRF); // Shuts down Sensor //
    
    return battery;
}

/******************************************************************
 *
 *   battery_status
 *
 *
 ******************************************************************/

char battery_status()
{
    char stat = 0;
    
    setIn(CHARGE_STATUS_PIN);
    setHigh(CHARGE_STATUS_PIN);
    _delay_ms(10);
    
    if(!getPin(CHARGE_STATUS_PIN)) 
        stat = 1;
    
    setLow(CHARGE_STATUS_PIN);
    _delay_ms(10);

    if(getPin(CHARGE_STATUS_PIN)) 
        stat = 2;
    
    return stat;
}

#define ADC_READINGS 100

/******************************************************************
 *
 *   hardware_analogRead
 *
 *
 ******************************************************************/

uint16_t hardware_analogRead(uint8_t ch)
{
    uint32_t buf = 0;

    // Initialize the ADC driver before first use
    ADC_Init(ADC_FREE_RUNNING | ADC_PRESCALE_32);

    // Must setup the ADC channel to read beforehand
    if(ch < 15) 
        ADC_SetupChannel(ch);
    
    if(ch > 7) 
        ch = (1 << 8 | ((ch - 8) << MUX0));
    else 
        ch = ch << MUX0;

    // Perform a single conversion of the ADC channel 1
//  buf = ADC_GetChannelReading(ADC_REFERENCE_AVCC | ADC_RIGHT_ADJUSTED | ch);

    // Start reading ADC channel 1 in free running (continuous conversion) mode
    ADC_StartReading(ADC_REFERENCE_AVCC | ADC_RIGHT_ADJUSTED | ch);
    
    for(uint8_t i = 0; i < ADC_READINGS; i++)
    {
        while (!(ADC_IsReadingComplete()));
        
        buf += ADC_GetResult();
    }

    // Leave ADC Disabled to save power
    ADC_Disable();

    return (uint16_t)(buf / ADC_READINGS);
}

/******************************************************************
 *
 *   hardware_bootloader
 *
 *
 ******************************************************************/

void hardware_bootloader(void)
{
    typedef void (*AppPtr_t)(void) __attribute__((noreturn));

    AppPtr_t BootStartPtr = (AppPtr_t)0xF000;

    // If USB is used, detach from the bus
    USB_Detach();
    USB_Disable();

    shutter_off();
    lcd.off();

    clock.disable();

    // Disable all interrupts
    cli();

    wdt_disable();

    BootStartPtr();

    FOREVER;
}

/******************************************************************
 *
 *   hardware_lightning_enable
 *
 *
 ******************************************************************/

void hardware_lightning_enable()
{
    // Need to power off lights //
    backlightVal = lcd.getBacklight();
    lcd.backlight(0);
    
    if(backlightVal > 0) 
        _delay_ms(50);

    setIn(LIGHT_SENSE_PIN); // set to input
    setLow(LIGHT_SENSE_PIN); // no pull-up

    uint8_t i = 3;
    while(i--)
    {
        hardware_light_enable(i);
        _delay_ms(50);
        uint16_t reading = hardware_analogRead(0);
        //DEBUG('(');
        //DEBUG(reading);
        //DEBUG(')');
        //DEBUG(':');
        //DEBUG(' ');
        //DEBUG_NL();
        if(reading < 256) break;
    }
    //DEBUG(i);
    //if(getPin(LIGHT_SENSE_PIN)) {
    //    DEBUG(STR("+"));
    //}
    //else
    //{
    //    DEBUG(STR("-"));
    //}

    shutter_half();
    
    EIMSK &= ~_BV(INT6);      // Interrupt disable 
    EICRB |= (1<<ISC61)|(1<<ISC60); // Rising edge
    EIMSK |= _BV(INT6);      // Interrupt enable 
}

/******************************************************************
 *
 *   hardware_lightning_disable
 *  Sets up an interrupt to trigger from the analog light sensor
 *
 ******************************************************************/

void hardware_lightning_disable()
{
    EIMSK &= ~_BV(INT6);      // Interrupt disable
    shutter_off();
    hardware_light_disable();
    if(backlightVal > lcd.getBacklight()) 
        lcd.backlight(backlightVal);
}




