/*
 *  shutter.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *	Licensed under GPLv3
 *
 */

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/version.h>
#include <avr/eeprom.h>
#include <string.h>

#include "tldefs.h"
#include "shutter.h"
#include "clock.h"
#include "hardware.h"
#include "IR.h"
#include "bluetooth.h"
#include "debug.h"
#include "math.h"
#include "settings.h"
#include "PTP_Driver.h"
#include "PTP.h"
#include "timelapseplus.h"
#include "light.h"
#include "5110LCD.h"
#include "button.h"
#include "menu.h"

#define RUN_DELAY 0
#define RUN_BULB 1
#define RUN_PHOTO 2
#define RUN_GAP 3
#define RUN_NEXT 4
#define RUN_END 5
#define RUN_ERROR 6

extern IR ir;
extern PTP camera;
extern settings conf;
extern shutter timer;
extern BT bt;
extern Clock clock;
extern Light light;
extern MENU menu;
extern LCD lcd;

volatile unsigned char state;
const uint16_t settings_warn_time = 0;
const uint16_t settings_mirror_up_time = 3;
volatile char cable_connected; // 1 = cable connected, 0 = disconnected

char shutter_state, ir_shutter_state; // used only for momentary toggle mode //
uint32_t BulbMax; // calculated during bulb ramp mode
program stored[MAX_STORED+1]EEMEM;
uint8_t BulbMaxEv;

uint8_t lastShutterError = 0;


const char STR_BULBMODE_ALERT[]PROGMEM = "Please make sure the camera is in bulb mode before continuing";
const char STR_BULBSUPPORT_ALERT[]PROGMEM = "Bulb mode not supported via USB. Use an adaptor cable with the USB";
const char STR_MEMORYSPACE_ALERT[]PROGMEM = "Please confirm there is enough space on the memory card in the camera";
const char STR_APERTURE_ALERT[]PROGMEM = "Note that using an aperture other than the maximum without lens-twist can cause flicker";
const char STR_DEVMODE_ALERT[]PROGMEM = "DEV Mode can interfere with the light sensor. Please disable before continuing";
const char STR_ZEROLENGTH_ALERT[]PROGMEM = "Length greater than zero required in bulb ramp mode. Set the length before running";
const char STR_NOCAMERA_ALERT[]PROGMEM = "Camera not connected. Connect a camera or press ok to use the IR interface";


/******************************************************************
 *
 *   shutter::
 *
 *
 ******************************************************************/

shutter::shutter()
{
    if(eeprom_read_byte((const uint8_t*)&stored[0].Name[0]) == 255) 
        setDefault();
    
    restoreCurrent(); // Default Program //
    ENABLE_MIRROR;
    ENABLE_SHUTTER;
    CHECK_CABLE;
    shutter_state = 0;
    ir_shutter_state = 0;
}

/******************************************************************
 *
 *   shutter::setDefault
 *
 *
 ******************************************************************/

void shutter::setDefault()
{
    current.Name[0] = 'D';
    current.Name[1] = 'E';
    current.Name[2] = 'F';
    current.Name[3] = 'A';
    current.Name[4] = 'U';
    current.Name[5] = 'L';
    current.Name[6] = 'T';
    current.Name[7] = '\0';
    current.Delay = 5;
    current.Photos = 10;
    current.Gap = 20;
    current.GapMin = BRAMP_INTERVAL_MIN;
    current.IntervalMode = INTERVAL_MODE_FIXED;
    current.Exps = 3;
    current.Mode = MODE_TIMELAPSE;
    current.Exp = 46;
    current.ArbExp = 100;
    current.Bracket = 6;
    current.Keyframes = 1;
    current.Duration = 3600;
    current.BulbStart = 53;
    current.Bulb[0] = 36;
    current.Key[0] = 3600;
    current.brampMethod = BRAMP_METHOD_AUTO;
    current.nightMode = BRAMP_TARGET_AUTO;
    current.nightISO = 31;
    current.nightShutter = 31;
    current.nightAperture = 9;
    save(0);

    uint8_t i;
    for(i = 1; i < MAX_STORED; i++)
    {
        eeprom_write_byte((uint8_t*)&stored[i].Name[0],  255);
    }

    saveCurrent();
}

/******************************************************************
 *
 *   shutter::off
 *
 *
 ******************************************************************/

void shutter::off(void)
{
    shutter_off();
}
void shutter_off(void)
{
    if(conf.devMode) 
        hardware_flashlight(0);
    
    SHUTTER_CLOSE;
    MIRROR_DOWN; 
    clock.in(20, &check_cable);
    ir_shutter_state = 0;
    shutter_state = 0;
}
void shutter_off_quick(void)
{
    SHUTTER_CLOSE;
    MIRROR_DOWN;
    ir_shutter_state = 0;
    shutter_state = 0;
}

/******************************************************************
 *
 *   shutter::half
 *
 *
 ******************************************************************/

void shutter::half(void)
{
    shutter_half();
}
void shutter_half(void)
{
    shutter_off(); // first we completely release the shutter button since some cameras need this to release the bulb

    if(conf.halfPress == HALF_PRESS_ENABLED) clock.in(30, &shutter_half_delayed);
}
void shutter_half_delayed(void)
{
    if(conf.devMode) 
        hardware_flashlight(0);
    
    SHUTTER_CLOSE;
    MIRROR_UP;
}

/******************************************************************
 *
 *   shutter::full
 *
 *
 ******************************************************************/

void shutter::full(void)
{
    shutter_full();
}
void shutter_full(void)
{
    if(conf.devMode) 
        hardware_flashlight(1);

    MIRROR_UP;
    SHUTTER_OPEN;
}

/******************************************************************
 *
 *   shutter::bulbStart
 *
 *
 ******************************************************************/

void shutter::bulbStart(void)
{
    shutter_bulbStart();
}
void shutter_bulbStart(void)
{
    lastShutterError = 0;
    if((cable_connected == 0 || !(conf.interface & INTERFACE_CABLE)) && ir_shutter_state != 1)
    {
        if(camera.supports.bulb && (conf.interface & INTERFACE_USB))
        {
            lastShutterError = camera.bulbMode();
            if(lastShutterError) return;
            lastShutterError = camera.bulbStart();
        }
        else if(conf.interface & INTERFACE_IR)
        {
            ir_shutter_state = 1;
            ir.bulbStart();
        }
    }
    else
    {
        //if(camera.supports.capture) camera.busy = true;
    }

    if(conf.interface & INTERFACE_CABLE)
    {
        if(conf.bulbMode == 0)
        {
            shutter_full();
        } 
        else if(conf.bulbMode == 1 && shutter_state != 1)
        {
            shutter_full();
            shutter_state = 1;
            if(camera.ready)
                clock.in(SHUTTER_PRESS_TIME, &shutter_off);
            else
                clock.in(SHUTTER_PRESS_TIME, &shutter_half);
        }
    }
}

/******************************************************************
 *
 *   shutter::bulbEnd
 *
 *
 ******************************************************************/

void shutter::bulbEnd(void)
{
    shutter_bulbEnd();
}
void shutter_bulbEnd(void)
{
    DEBUG(PSTR("Bulb End: "));
    if(camera.bulb_open || ir_shutter_state == 1)
    {
        if(camera.bulb_open && (conf.interface & INTERFACE_USB))
        {
            DEBUG(PSTR("USB "));
            camera.bulbEnd();
        }
        else if(conf.interface & INTERFACE_IR)
        {
            DEBUG(PSTR("IR "));
            ir.bulbEnd();
        }
        ir_shutter_state = 0;
    } 
    if(conf.interface & INTERFACE_CABLE)
    {
        if(conf.bulbMode == 0)
        {
            DEBUG(PSTR("CABLE "));
            shutter_off();
        }
        else if(conf.bulbMode == 1 && shutter_state == 1)
        {
            shutter_full();
            shutter_state = 0;
            clock.in(SHUTTER_PRESS_TIME, &shutter_off);
        }
    }
    DEBUG_NL();
}

/******************************************************************
 *
 *   shutter::capture
 *
 *
 ******************************************************************/

void shutter::capture(void)
{
    shutter_capture();
}
void shutter_capture(void)
{
    lastShutterError = 0;
    if(conf.interface & (INTERFACE_CABLE | INTERFACE_USB))
    {
        shutter_full();
        clock.in(SHUTTER_PRESS_TIME, &shutter_off);
        ir_shutter_state = 0;
        shutter_state = 0;
        if(cable_connected == 0)
        {
            if(camera.supports.capture)
            {
                    lastShutterError = camera.capture();
            }
            else
            {
                if(conf.interface & INTERFACE_IR) ir.shutterNow();
            }
        }
        else
        {
            if(camera.supports.capture) camera.busy = true;
        }
    }
    else if(conf.interface == INTERFACE_IR)
    {
        ir.shutterNow();
    }
}

/******************************************************************
 *
 *   shutter::cableIsConnected
 *
 *
 ******************************************************************/

char shutter::cableIsConnected(void)
{
    if(MIRROR_IS_DOWN)
    {
        CHECK_CABLE;
    }
    return cable_connected;
}

/******************************************************************
 *
 *   shutter::save
 *
 *
 ******************************************************************/

void shutter::save(char id)
{
    eeprom_write_block((const void*)&current, &stored[(uint8_t)id], sizeof(program));
    currentId = id;
}

void shutter::saveCurrent()
{
    eeprom_write_block((const void*)&current, &stored[MAX_STORED], sizeof(program));
}
void shutter::restoreCurrent()
{
    eeprom_read_block((void*)&current, &stored[MAX_STORED], sizeof(program));
}

/******************************************************************
 *
 *   shutter::load
 *
 *
 ******************************************************************/

void shutter::load(char id)
{
    eeprom_read_block((void*)&current, &stored[(uint8_t)id], sizeof(program));
    currentId = id;
}

/******************************************************************
 *
 *   shutter::nextId
 *
 *
 ******************************************************************/

int8_t shutter::nextId(void)
{
    int8_t id = -1, i;
    for(i = 1; i < MAX_STORED; i++)
    {
        if(eeprom_read_byte((uint8_t*)&stored[i].Name[0]) == 255)
        {
            id = i;
            break;
        }
    }
    return id;
}

/******************************************************************
 *
 *   shutter::begin
 *
 *
 ******************************************************************/

void shutter::begin()
{
    saveCurrent();
    running = 1;
}

/******************************************************************
 *
 *   shutter::begin
 *
 *
 ******************************************************************/

void shutter::pause(uint8_t p)
{
    if(p)
    {
        if(paused) return;
        pausing = 1;
    }
    else
    {
        if(pausing > 1) return;
        pausing = 1;
        paused = 1;
    }
}

/******************************************************************
 *
 *   shutter::run
 *
 *
 ******************************************************************/

char shutter::task()
{
    char cancel = 0;
    static uint8_t enter, exps, run_state = RUN_DELAY, old_state = 255, preChecked = false;
    static int8_t evShift;
    static uint16_t photos;
    static uint32_t last_photo_end_ms;

    if(MIRROR_IS_DOWN)
    {
        CHECK_CABLE;
    }
    
    if(!running)
    {
        if(enter) 
            cancel = 1;
        else 
            return 0;
    }

    if(enter == 0 || preChecked) // Initialize variables and setup I/O pins
    {
        paused = 0;
        pausing = 0;
        if(current.Mode & RAMP)
        {
            uint32_t tmp = (uint32_t)current.Duration * 10;
            tmp /= (uint32_t) current.Gap;
            current.Photos = (uint16_t) tmp;
        }
        status.interval = current.Gap;


        ////////////////////////////// pre check ////////////////////////////////////
        CHECK_ALERT(STR_NOCAMERA_ALERT, conf.interface != INTERFACE_IR && !camera.supports.capture && !cableIsConnected());
        CHECK_ALERT(STR_APERTURE_ALERT, camera.ready && camera.supports.aperture && camera.aperture() != camera.apertureWideOpen() && (current.Mode & TIMELAPSE) && !((current.Mode & RAMP) && (conf.brampMode & BRAMP_MODE_APERTURE)));
        CHECK_ALERT(STR_BULBSUPPORT_ALERT, (current.Mode & RAMP) && camera.ready && !camera.supports.bulb && !cable_connected);
        CHECK_ALERT(STR_BULBMODE_ALERT, (current.Mode & RAMP) && !camera.isInBulbMode());
        CHECK_ALERT(STR_MEMORYSPACE_ALERT, (current.Photos > 0) && camera.ready && camera.photosRemaining && camera.photosRemaining < current.Photos);
        CHECK_ALERT(STR_DEVMODE_ALERT, (current.Mode & RAMP) && (current.brampMethod == BRAMP_METHOD_AUTO) && conf.devMode);
        CHECK_ALERT(STR_ZEROLENGTH_ALERT, (current.Mode & RAMP) && (current.Duration == 0));

        if(!preChecked && menu.waitingAlert())
        {
            menu.blink();
        }

        preChecked = true;
        if(menu.waitingAlert()) return CONTINUE; //////////////////////////////////////////////

        menu.message(TEXT("Loading"));
        menu.task();

        enter = 1;
        preChecked = false;


        if(current.Mode & RAMP)
        {
            calcBulbMax();
            status.rampMax = calcRampMax();
            status.rampMin = calcRampMin();
            rampRate = 0;
            status.rampStops = 0;
            internalRampStops = 0;
            light.integrationStart(conf.lightIntegrationMinutes);
            lightReading = lightStart = light.readIntegratedEv();

            if(current.nightMode == BRAMP_TARGET_AUTO)
            {
                status.nightTarget = 0;
            }
            else if(current.nightMode == BRAMP_TARGET_CUSTOM)
            {
                status.nightTarget = calcRampTarget(current.nightShutter, current.nightISO, current.nightAperture);
            }
            else
            {
                status.nightTarget = ((int8_t)current.nightMode) - BRAMP_TARGET_OFFSET;
            }

            DEBUG(STR(" -----> starting light reading: "));
            DEBUG(lightStart);
            DEBUG_NL(); 
            if(light.underThreshold && current.nightMode != BRAMP_TARGET_AUTO)
            {
                DEBUG(STR(" -----> starting at night target\r\n"));
                lightStart = status.nightTarget;
            }
        }

        run_state = RUN_DELAY;
        clock.tare();
        photos = 0;
        exps = 0;

        if(camera.supports.aperture) aperture = camera.aperture();
        if(camera.supports.iso) iso = camera.iso();

        current.infinitePhotos = current.Photos == 0 ? 1 : 0;
        status.infinitePhotos = current.infinitePhotos;
        status.photosRemaining = current.Photos;
        status.photosTaken = 0;
        status.mode = (uint8_t) current.Mode;
        last_photo_end_ms = 0;
        last_photo_ms = 0;
        evShift = 0;

        ENABLE_MIRROR;
        ENABLE_SHUTTER;
        SHUTTER_CLOSE;
        MIRROR_DOWN;
    }


    if(pausing > 0 && paused == 1) // delays the restart to prevent camera shake
    {
        if(pausing > 10)
        {
            paused = 0;
            pausing = 0;
        }
        else
        {
            pausing++;
        }
    }

    if(paused) return CONTINUE;
    if(pausing && run_state != RUN_BULB)
    {
        pausing = 0;
        paused = 1;
    }


    /////// RUNNING PROCEDURE ///////
    if(run_state == RUN_DELAY)
    {
        if(old_state != run_state)
        {
            if(conf.debugEnabled)
            {
                DEBUG(PSTR("State: RUN_DELAY"));
                DEBUG_NL();
            }
            strcpy((char *) status.textStatus, TEXT("Delay"));
            old_state = run_state;
        }


        if(((unsigned long)clock.event_ms / 1000) > current.Delay)
        {
            clock.tare();
            clock.reset();
            last_photo_ms = 0;
            run_state = RUN_PHOTO;
        } 
        else
        {
            status.nextPhoto = (unsigned int) (current.Delay - (unsigned long)clock.event_ms / 1000);
            if(((unsigned long)clock.event_ms / 1000) + settings_mirror_up_time >= current.Delay)
            {
                // Mirror Up //
                shutter_half(); // This is to wake up the camera (even if USB is connected)
            }

            if((settings_warn_time > 0) && (((unsigned long)clock.event_ms / 1000) + settings_warn_time >= current.Delay))
            {
                // Flash Light //
                _delay_ms(50);
            }
        }
    }

    if(run_state == RUN_PHOTO && (exps + photos == 0 || (uint8_t)((clock.Ms() - last_photo_end_ms) / 10) >= conf.cameraFPS))
    {
        last_photo_end_ms = 0;
        if(old_state != run_state)
        {
            if(conf.debugEnabled)
            {
                DEBUG(PSTR("State: RUN_PHOTO"));
                DEBUG_NL();
            }
            strcpy((char *) status.textStatus, TEXT("Photo"));
            old_state = run_state;
        }
        if(current.Exp > 0 || (current.Mode & RAMP) || conf.arbitraryBulb)
        {
            clock.tare();
            run_state = RUN_BULB;
        } 
        else
        {
            exps++;
            capture();
            
            if(status.interval <= settings_mirror_up_time && !camera.ready) 
                shutter_half(); // Mirror Up //

            run_state = RUN_NEXT;
        }
    }
    
    if(run_state == RUN_BULB)
    {
        static uint32_t bulb_length, exp;
        static uint8_t m = SHUTTER_MODE_BULB;

        if(old_state != run_state)
        {
            old_state = run_state;

            if(conf.debugEnabled)
            {
                DEBUG(PSTR("State: RUN_BULB"));
                DEBUG_NL();
            }
            strcpy((char *) status.textStatus, TEXT("Bulb"));

            m = camera.shutterType(current.Exp);
            if(m & SHUTTER_MODE_BULB || conf.arbitraryBulb)
            {
                if(conf.arbitraryBulb)
                {
                    exp = current.ArbExp * 100;
                    m = SHUTTER_MODE_BULB;
                }
                else
                {
                    exp = camera.bulbTime((int8_t)current.Exp);
                }
            }

            if(current.Mode & RAMP)
            {
                float key1 = 1, key2 = 1, key3 = 1, key4 = 1;
                char found = 0;
                uint8_t i;
                m = SHUTTER_MODE_BULB;
                shutter_off();


                if(current.brampMethod == BRAMP_METHOD_KEYFRAME) //////////////////////////////// KEYFRAME RAMP /////////////////////////////////////
                {
                    // Bulb ramp algorithm goes here
                    for(i = 0; i < current.Keyframes; i++)
                    {
                        if(clock.Seconds() <= current.Key[i])
                        {
                            found = 1;
                            if(i == 0)
                            {
                                key2 = key1 = (float)(current.BulbStart);
                            }
                            else if(i == 1)
                            {
                                key1 = (float)(current.BulbStart);
                                key2 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i - 1]));
                            }
                            else
                            {
                                key1 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i - 2]));
                                key2 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i - 1]));
                            }
                            key3 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i]));
                            key4 = (float)((int8_t)current.BulbStart - *((int8_t*)&current.Bulb[i < (current.Keyframes - 1) ? i + 1 : i]));
                            break;
                        }
                    }
                    
                    if(found)
                    {
                        uint32_t var1 = clock.Seconds();
                        uint32_t var2 = (i > 0 ? current.Key[i - 1] : 0);
                        uint32_t var3 = current.Key[i];

                        float t = (float)(var1 - var2) / (float)(var3 - var2);
                        float curveEv = curve(key1, key2, key3, key4, t);
                        status.rampStops = (float)current.BulbStart - curveEv;
                        exp = camera.bulbTime(curveEv - (float)evShift);

                        if(conf.debugEnabled)
                        {
                            DEBUG(PSTR("   Keyframe: "));
                            DEBUG(i);
                            DEBUG_NL();
                            DEBUG(PSTR("    Percent: "));
                            DEBUG(t);
                            DEBUG_NL();
                            DEBUG(PSTR("    CurveEv: "));
                            DEBUG(curveEv);
                            DEBUG_NL();
                            DEBUG(PSTR("CorrectedEv: "));
                            DEBUG(curveEv - (float)evShift);
                            DEBUG_NL();
                            DEBUG(PSTR("   Exp (ms): "));
                            DEBUG(exp);
                            DEBUG_NL();
                            DEBUG(PSTR("    evShift: "));
                            DEBUG(evShift);
                            DEBUG_NL();
                        }
                    }
                    else
                    {
                        status.rampStops = (float)((int8_t)(current.BulbStart - (current.BulbStart - *((int8_t*)&current.Bulb[current.Keyframes - 1]))));
                        exp = camera.bulbTime((int8_t)(current.BulbStart - *((int8_t*)&current.Bulb[current.Keyframes - 1]) - (float)evShift));
                    }
                }

                else if(current.brampMethod == BRAMP_METHOD_GUIDED || current.brampMethod == BRAMP_METHOD_AUTO) //////////////////////////////// GUIDED / AUTO RAMP /////////////////////////////////////
                {

//#################### AUTO BRAMP ####################
                    if(current.brampMethod == BRAMP_METHOD_AUTO)
                    {
                        if(light.underThreshold && current.nightMode != BRAMP_TARGET_AUTO)
                        {
                            DEBUG(STR(" -----> under night threshold\r\n"));
                            if(current.nightMode == BRAMP_TARGET_CUSTOM)
                            {
                                status.rampTarget = (float)status.nightTarget;
                            }
                            else
                            {
                                if(light.slope < 0 - ((NIGHT_THRESHOLD / 3) / BRAMP_RATE_FACTOR) && lightStart == status.nightTarget) // respond quickly during night-to-day once we see light
                                {
                                    DEBUG(STR(" -----> ramping toward sunrise\r\n"));
                                    status.rampTarget = BRAMP_RATE_MAX; //lightStart - (float)(NIGHT_THRESHOLD - status.nightTarget);
                                }
                                else
                                {
                                    DEBUG(STR(" -----> holding night exposure\r\n"));
                                    status.rampTarget = lightStart - (float)status.nightTarget; // hold at night exposure
                                }
                            }
                        }
                        else
                        {
                            DEBUG(STR(" -----> using light sensor target\r\n"));
                            status.rampTarget = (lightStart - lightReading);
                        }
                        DEBUG(STR(" -----> TARGET: "));
                        DEBUG(status.rampTarget);
                        DEBUG_NL();
                        if(status.rampTarget > status.rampMax) status.rampTarget = status.rampMax;
                        if(status.rampTarget < status.rampMin) status.rampTarget = status.rampMin;
                        float delta = status.rampTarget - status.rampStops;
                        delta *= BRAMP_RATE_FACTOR; // 2 = aim to meet the goal in 30 minutes
                        if(light.lockedSlope > 0.0 && current.nightMode != BRAMP_TARGET_AUTO)
                        {
                            if(light.lockedSlope < delta) delta = light.lockedSlope; // hold the last valid slope reading from the light sensor
                        }
                        else
                        {
                            if((light.slope > delta && delta > 0) || (light.slope < delta && delta < 0)) delta = (delta + light.slope) / 2; // factor in the integrated slope
                        }

                        // coerce to limits
                        if(delta > BRAMP_RATE_MAX) delta = BRAMP_RATE_MAX;
                        if(delta < -BRAMP_RATE_MAX) delta = -BRAMP_RATE_MAX;

                        DEBUG(STR(" -----> delta: "));
                        DEBUG(delta);
                        DEBUG_NL();

                        if(delta < BRAMP_RATE_MIN && delta > -BRAMP_RATE_MIN && (rampRate >= BRAMP_RATE_MIN || rampRate <= -BRAMP_RATE_MIN)) // keep momentum
                        {
                            if(delta > 0)
                            {
                                DEBUG(STR(" -----> coercing rate to min (+)\r\n"));
                                rampRate = BRAMP_RATE_MIN;
                            }
                            else if(delta < 0)
                            {
                                DEBUG(STR(" -----> coercing rate to min (-)\r\n"));
                                rampRate = -BRAMP_RATE_MIN;
                            }
                            else
                            {
                                DEBUG(STR(" -----> setting rate to zero\r\n"));
                                rampRate = 0;
                            }
                        }
                        else
                        {
                            //rampRate = (((int8_t) delta) + rampRate) / 2; // ease the change a little
                            rampRate = (int8_t) delta;
                        }
                        DEBUG(STR(" -----> RATE: "));
                        DEBUG(rampRate);
                        DEBUG_NL();
                    }
//####################################################

                    status.rampStops += ((float)rampRate / (3600.0 / 3)) * ((float)status.interval / 10.0);

                    if(status.rampStops >= status.rampMax)
                    {
                        rampRate = 0;
                        status.rampStops = status.rampMax;
                    }
                    else if(status.rampStops <= status.rampMin)
                    {
                        rampRate = 0;
                        status.rampStops = status.rampMin;
                    }
                    exp = camera.bulbTime(current.BulbStart - status.rampStops - (float)evShift);
                }
/*
                else if(current.brampMethod == BRAMP_METHOD_AUTO) //////////////////////////////// AUTO RAMP /////////////////////////////////////
                {
                    //                    
                    //internalRampStops += ((float)rampRate / (3600.0 / 3)) * ((float)status.interval / 10.0);
                    //status.rampStops = (lightStart - lightReading) + internalRampStops;
                    //

                    // EXPERIMENTAL //
                    float target = (lightStart - lightReading);// + internalRampStops;
                    if(target > status.rampMax) target = status.rampMax;
                    if(target < status.rampMin) target = status.rampMin;
                    float delta = target - status.rampStops;
                    if(delta > 16.0) delta = 16.0;
                    if(delta < -16.0) delta = -16.0;
                    rampRate = (int8_t) delta;
                    status.rampStops += ((float)rampRate / (3600.0 / 3)) * ((float)status.interval / 10.0);
                    //////////////////

                    if(status.rampStops > status.rampMax)
                    {
                        DEBUG(PSTR("   (ramp max)\n"));
                        status.rampStops = status.rampMax;
                    }
                    else if(status.rampStops < status.rampMin)
                    {
                        DEBUG(PSTR("   (ramp min)\n"));
                        status.rampStops = status.rampMin;
                    }
                    //                                   56        -     0            -    -6 
                    float tmp_ev = (float)current.BulbStart - status.rampStops - (float)evShift;
                    exp = camera.bulbTime(tmp_ev);

                    if(conf.debugEnabled)
                    {
                        DEBUG(PSTR("     lightStart: "));
                        DEBUG(lightStart);
                        DEBUG_NL();
                        DEBUG(PSTR("   lightReading: "));
                        DEBUG(lightReading);
                        DEBUG_NL();
                        DEBUG(PSTR("  bulbStart Ev: "));
                        DEBUG(current.BulbStart);
                        DEBUG_NL();
                        DEBUG(PSTR("   bulbTime Ev: "));
                        DEBUG(tmp_ev);
                        DEBUG_NL();
                        DEBUG(PSTR("   Exp (ms): "));
                        DEBUG(exp);
                        DEBUG_NL();                        
                    }
                }
*/
                bulb_length = exp;

                if(current.IntervalMode == INTERVAL_MODE_AUTO)
                {
                    status.interval = bulb_length / 100 + BRAMP_INTERVAL_MIN;
                    if(status.interval + BRAMP_INTERVAL_MIN > current.Gap) status.interval = current.Gap;
                    if(status.interval < current.GapMin) status.interval = current.GapMin;
                }
                else
                {
                    status.interval = current.Gap;
                }



                if(camera.supports.iso || camera.supports.aperture)
                {
                    uint8_t nextAperture = aperture;
                    uint8_t nextISO = iso;

                    int8_t tmpShift = 0;

                    if((conf.brampMode & BRAMP_MODE_APERTURE) && camera.supports.aperture)
                    {
                        DEBUG(PSTR("Aperture Close: BEGIN\r\n\r\n"));
                        // Check for too long bulb time and adjust Aperture //
                        while(bulb_length > BulbMax)
                        {
                            nextAperture = camera.apertureDown(aperture);
                            if(nextAperture != aperture)
                            {
                                evShift += nextAperture - aperture;
                                tmpShift += nextAperture - aperture;
                                aperture = nextAperture;

                                DEBUG(PSTR("   Aperture UP:"));
                                DEBUG(evShift);
                                DEBUG(PSTR("   Aperture Val:"));
                                DEBUG(nextAperture);
                            }
                            else
                            {
                                DEBUG(PSTR("   Reached Aperture Max!!!\r\n"));
                                break;
                            }
                            bulb_length = camera.shiftBulb(exp, tmpShift);
                        }
                        DEBUG(PSTR("Aperture Close: DONE\r\n\r\n"));
                    }

                    if((conf.brampMode & BRAMP_MODE_ISO) && camera.supports.iso)
                    {
                        DEBUG(PSTR("ISO Up: BEGIN\r\n\r\n"));
                        // Check for too long bulb time and adjust ISO //
                        while(bulb_length > BulbMax)
                        {
                            nextISO = camera.isoUp(iso);
                            if(nextISO != iso)
                            {
                                evShift += nextISO - iso;
                                tmpShift += nextISO - iso;
                                iso = nextISO;

                                DEBUG(PSTR("   ISO UP:"));
                                DEBUG(evShift);
                                DEBUG(PSTR("   ISO Val:"));
                                DEBUG(nextISO);
                            }
                            else
                            {
                                DEBUG(PSTR("   Reached ISO Max!!!\r\n"));
                                break;
                            }
                            bulb_length = camera.shiftBulb(exp, tmpShift);
                        }
                        DEBUG(PSTR("ISO Up: DONE\r\n\r\n"));
                    }


                    if((conf.brampMode & BRAMP_MODE_ISO) && camera.supports.iso)
                    {
                        // Check for too short bulb time and adjust ISO //
                        for(;;)
                        {
                            DEBUG(PSTR("ISO Down: BEGIN\r\n\r\n"));
                            nextISO = camera.isoDown(iso);
                            if(nextISO != iso && nextISO < 127)
                            {
                                uint32_t bulb_length_test = camera.shiftBulb(exp, tmpShift + (nextISO - iso));
                                if(bulb_length_test < BulbMax)
                                {
                                    evShift += nextISO - iso;
                                    tmpShift += nextISO - iso;
                                    iso = nextISO;
                                    DEBUG(PSTR("   ISO DOWN:"));
                                    DEBUG(evShift);
                                    DEBUG(PSTR("   ISO Val:"));
                                    DEBUG(nextISO);
                                    bulb_length = bulb_length_test;
                                    continue;
                                }
                            }
                            nextISO = iso;
                            break;
                        }
                        DEBUG(PSTR("ISO Down: DONE\r\n\r\n"));
                    }


                    if((conf.brampMode & BRAMP_MODE_APERTURE) && camera.supports.aperture)
                    {
                        // Check for too short bulb time and adjust Aperture //
                        for(;;)
                        {
                            DEBUG(PSTR("Aperture Open: BEGIN\r\n\r\n"));
                            nextAperture = camera.apertureUp(aperture);
                            if(nextAperture != aperture)
                            {
                                if(nextAperture >= 127) break;
                                uint32_t bulb_length_test = camera.shiftBulb(exp, tmpShift + (nextAperture - aperture));
                                if(bulb_length_test < BulbMax)
                                {
                                    evShift  += nextAperture - aperture;
                                    tmpShift += nextAperture - aperture;
                                    aperture = nextAperture;
                                    DEBUG(PSTR("Aperture DOWN:"));
                                    DEBUG(evShift);
                                    DEBUG(PSTR(" Aperture Val:"));
                                    DEBUG(nextAperture);
                                    bulb_length = camera.shiftBulb(exp, tmpShift);
                                    continue;
                                }
                            }
                            nextAperture = aperture;
                            break;
                        }
                        DEBUG(PSTR("Aperture Open: DONE\r\n\r\n"));
                    }
                    
                    if(conf.extendedRamp)
                    {
                        if(bulb_length < camera.bulbTime((int8_t)MAX_EXTENDED_RAMP_SHUTTER))
                        {
                            DEBUG(PSTR("   Reached Max Shutter!!!\r\n"));
                            bulb_length = camera.bulbTime((int8_t)MAX_EXTENDED_RAMP_SHUTTER);
                        }
                    }
                    else
                    {
                        if(bulb_length < camera.bulbTime((int8_t)camera.bulbMin()))
                        {
                            DEBUG(PSTR("   Reached Bulb Min!!!\r\n"));
                            bulb_length = camera.bulbTime((int8_t)camera.bulbMin());
                        }
                    }

                    shutter_off_quick(); // Can't change parameters when half-pressed
                    if((conf.brampMode & BRAMP_MODE_APERTURE) && camera.supports.aperture)
                    {
                        // Change the Aperture //
                        if(camera.aperture() != nextAperture)
                        {
                            DEBUG(PSTR("Setting Aperture..."));
                            if(camera.setAperture(nextAperture) == PTP_RETURN_ERROR)
                            {
                                DEBUG(PSTR("ERROR!!!\r\n"));
                                run_state = RUN_ERROR;
                                return CONTINUE;
                            }
                            DEBUG_NL();
                        }
                    }
                    if((conf.brampMode & BRAMP_MODE_ISO) && camera.supports.iso)
                    {
                        // Change the ISO //
                        if(camera.iso() != nextISO)
                        {
                            DEBUG(PSTR("Setting ISO..."));
                            if(camera.setISO(nextISO) == PTP_RETURN_ERROR)
                            {
                                DEBUG(PSTR("ERROR!!!\r\n"));
                                run_state = RUN_ERROR;
                                return CONTINUE;
                            }
                            DEBUG_NL();
                        }
                    }
                }
                
                if(conf.debugEnabled)
                {
                    DEBUG(PSTR("   Seconds: "));
                    DEBUG((uint16_t)clock.Seconds());
                    DEBUG_NL();
                    DEBUG(PSTR("   evShift: "));
                    DEBUG(evShift);
                    DEBUG_NL();
                    DEBUG(PSTR("BulbLength: "));
                    DEBUG((uint16_t)bulb_length);
                    if(found) DEBUG(PSTR(" (calculated)"));
                    DEBUG_NL();

                    /*
                    DEBUG(PSTR("i: "));
                    DEBUG(current.Bulb[i]);
                    DEBUG_NL();
                    DEBUG(PSTR("Key1: "));
                    DEBUG((int16_t)key1);
                    DEBUG_NL();
                    DEBUG(PSTR("Key2: "));
                    DEBUG((int16_t)key2);
                    DEBUG_NL();
                    DEBUG(PSTR("Key3: "));
                    DEBUG((int16_t)key3);
                    DEBUG_NL();
                    DEBUG(PSTR("Key4: "));
                    DEBUG((int16_t)key4);
                    DEBUG_NL();
                    */
                }
            }

            if(current.Mode & HDR)
            {
                uint8_t tv_offset = ((current.Exps - 1) / 2) * current.Bracket - exps * current.Bracket;
                if(current.Mode & RAMP)
                {
                    bulb_length = camera.shiftBulb(bulb_length, tv_offset);
                }
                else
                {
                    shutter_off_quick(); // Can't change parameters when half-pressed
                    m = camera.shutterType(current.Exp - tv_offset);
                    if(m == 0) m = SHUTTER_MODE_PTP;

                    if(m & SHUTTER_MODE_PTP)
                    {
                        DEBUG(PSTR("Shutter Mode PTP\r\n"));
                        camera.setShutter(current.Exp - tv_offset);
                        bulb_length = camera.bulbTime((int8_t)(current.Exp - tv_offset));
                    }
                    else
                    {
                        camera.bulbMode();
                        DEBUG(PSTR("Shutter Mode BULB\r\n"));
                        m = SHUTTER_MODE_BULB;
                        bulb_length = camera.bulbTime((int8_t)(current.Exp - tv_offset));
                    }
                }

                if(conf.debugEnabled)
                {
                    DEBUG_NL();
                    DEBUG(PSTR("Mode: "));
                    DEBUG(m);
                    DEBUG_NL();
                    DEBUG(PSTR("Tv: "));
                    DEBUG(current.Exp - tv_offset);
                    DEBUG_NL();
                    DEBUG(PSTR("Bulb: "));
                    DEBUG((uint16_t)bulb_length);
                    DEBUG_NL();
                }
            }
            
            if((current.Mode & (HDR | RAMP)) == 0)
            {
                if(conf.debugEnabled)
                {
                    DEBUG(PSTR("***Using exp: "));
                    DEBUG(exp);
                    DEBUG(PSTR(" ("));
                    DEBUG(current.Exp);
                    DEBUG(PSTR(")"));
                    DEBUG_NL();
                }
                bulb_length = exp;
                if(m & SHUTTER_MODE_PTP)
                {
                    shutter_off_quick(); // Can't change parameters when half-pressed
                    camera.manualMode();
                    camera.setShutter(current.Exp);
                }
            }

            status.bulbLength = bulb_length;

            if(current.Mode & RAMP && (!camera.isInBulbMode() && camera.ready))
            {
                DEBUG(PSTR("\r\n-->Using Extended Ramp\r\n"));
                DEBUG(PSTR("    ms: "));
                DEBUG(bulb_length);
                DEBUG_NL();
                DEBUG(PSTR("    ev: "));
                DEBUG(camera.bulbToShutterEv(bulb_length));
                DEBUG_NL();
                DEBUG_NL();
                camera.setShutter(camera.bulbToShutterEv(bulb_length));
                shutter_capture();
                _delay_ms(100);
                if(lastShutterError)
                {
                    DEBUG(PSTR("USB Error - trying again\r\n"));
                    old_state = 0;
                    clock.cancelJob();
                    return CONTINUE; // try it again
                }
            }
            else if(m & SHUTTER_MODE_BULB)
            {
                //DEBUG(PSTR("Running BULB\r\n"));
                camera.bulb_open = true;
                clock.job(&shutter_bulbStart, &shutter_bulbEnd, bulb_length + conf.bulbOffset);
                _delay_ms(100);
                if(lastShutterError)
                {
                    DEBUG(PSTR("USB Error - trying again\r\n"));
                    old_state = 0;
                    clock.cancelJob();
                    return CONTINUE; // try it again
                }
            }
            else
            {
                //DEBUG(PSTR("Running Capture\r\n"));
                shutter_capture();
            }
        }
        else if(!clock.jobRunning && !camera.busy)
        {
            exps++;

            lightReading = light.readIntegratedEv();

            _delay_ms(50);

            if(status.interval <= settings_mirror_up_time && !camera.ready) 
                shutter_half(); // Mirror Up //

            run_state = RUN_NEXT;
        }
        else
        {
            //if(clock.jobRunning) DEBUG(PSTR("Waiting on clock  "));
            //if(camera.busy) DEBUG(PSTR("Waiting on camera  "));
        }
    }
    
    if(run_state == RUN_NEXT)
    {
        last_photo_end_ms = clock.Ms();

        if(PTP_Connected && PTP_Error)
        {
            run_state = RUN_ERROR;
            return CONTINUE;
        }

        if(old_state != run_state)
        {
            if(conf.debugEnabled)
            {
                DEBUG(PSTR("State: RUN_NEXT"));
                DEBUG_NL();
            }
            old_state = run_state;
        }

        if(camera.busy && (photos < current.Photos || ((current.Mode & RAMP) && (clock.Seconds() < current.Duration))))
        {
            run_state = RUN_NEXT;
        }
        else if((exps >= current.Exps && (current.Mode & HDR)) || (current.Mode & HDR) == 0)
        {
            exps = 0;
            photos++;
            clock.tare();
            run_state = RUN_GAP;
        }
        else
        {
            clock.tare();
            run_state = RUN_PHOTO;
        }
    
        if(!current.infinitePhotos)
        {
            if(current.Mode & RAMP)
            {
                if(clock.Seconds() >= current.Duration)
                {
                    run_state = RUN_END;
                }
            }
            else
            {
                if(photos >= current.Photos || (((current.Mode & TIMELAPSE) == 0) && photos >= 1))
                {
                    run_state = RUN_END;
                }
            }
        }
        else
        {
            run_state = RUN_GAP;
        }

        if(camera.ready && current.Mode & RAMP && conf.extendedRamp && !camera.isInBulbMode() && conf.modeSwitch == USB_CHANGE_MODE_ENABLED && timer.status.bulbLength > camera.bulbTime((int8_t)camera.bulbMin()))
        {
            camera.bulbMode();
        }
        else if(camera.ready && current.Mode & RAMP && conf.extendedRamp && camera.isInBulbMode() && conf.modeSwitch == USB_CHANGE_MODE_ENABLED && timer.status.bulbLength < camera.bulbTime((int8_t)camera.bulbMin()))
        {
            camera.manualMode();
        }

        status.photosRemaining = current.Photos - photos;
        status.photosTaken = photos;
    }
    
    if(run_state == RUN_GAP)
    {
        if(old_state != run_state)
        {
            if(conf.auxPort == AUX_MODE_DOLLY) aux_pulse();
            if(conf.debugEnabled)
            {
                DEBUG(PSTR("State: RUN_GAP"));
                DEBUG_NL();
            }
            strcpy((char *) status.textStatus, TEXT("Waiting"));
            old_state = run_state;
        }
        uint32_t cms = clock.Ms();

        if((cms - last_photo_ms) / 100 >= status.interval)
        {
            last_photo_ms = cms;
            clock.tare();
            run_state = RUN_PHOTO;
        } 
        else
        {
            status.nextPhoto = (unsigned int) ((status.interval - (cms - last_photo_ms) / 100) / 10);
            if((cms - last_photo_ms) / 100 + (uint32_t)settings_mirror_up_time * 10 >= status.interval)
            {
                // Mirror Up //
                if(!camera.ready) shutter_half(); // Don't do half-press if the camera is connected by USB
            }
        }
    }
    
    if(run_state == RUN_ERROR)
    {
        if(old_state != run_state)
        {
            if(conf.debugEnabled)
            {
                DEBUG(PSTR("State: RUN_ERROR"));
                DEBUG_NL();
            }
            menu.blink();
            strcpy((char *) status.textStatus, TEXT("Error"));
            old_state = run_state;

            if(PTP_Connected && PTP_Error)
            {
                camera.resetConnection();
            }
        }

        //enter = 0;
        //running = 0;
        shutter_off();
        camera.bulbEnd();
        //light.stop();

        //hardware_flashlight((uint8_t) clock.Seconds() % 2);

        if(camera.ready) run_state = RUN_NEXT;

        return CONTINUE;
    }

    if(run_state == RUN_END)
    {
        if(old_state != run_state)
        {
            if(conf.debugEnabled)
            {
                DEBUG(PSTR("State: RUN_END"));
                DEBUG_NL();
            }
            strcpy((char *) status.textStatus, TEXT("Done"));
            old_state = run_state;
        }

        enter = 0;
        running = 0;
        shutter_off();
        camera.bulbEnd();
        hardware_flashlight(0);
        light.stop();
        aux1_off();
        aux2_off();
        clock.awake();

        return DONE;
    }

    /////////////////////////////////////////

    if(cancel)
    {
        run_state = RUN_END;
    }

    return CONTINUE;
}

void shutter::switchToGuided()
{
    rampRate = 0;
    current.brampMethod = BRAMP_METHOD_GUIDED;
}

void shutter::switchToAuto()
{
    lightReading = lightStart = light.readIntegratedEv();
    internalRampStops = status.rampStops;
    current.brampMethod = BRAMP_METHOD_AUTO;
}

void check_cable()
{
    CHECK_CABLE;
}

void aux_pulse()
{
    aux1_on();
    aux2_on();
    if(conf.dollyPulse == 65535) conf.dollyPulse = 100;
    if(conf.dollyPulse2 == 65535) conf.dollyPulse2 = 100;
    clock.in(conf.dollyPulse, &aux1_off);
    clock.in(conf.dollyPulse2, &aux2_off);
}

void aux1_on()
{
    ENABLE_AUX_PORT1;
    AUX_OUT1_ON;
}

void aux1_off()
{
    ENABLE_AUX_PORT1;
}

void aux2_on()
{
    ENABLE_AUX_PORT2;
    AUX_OUT2_ON;
}

void aux2_off()
{
    ENABLE_AUX_PORT2;
}

uint8_t stopName(char name[8], uint8_t stop)
{
    name[0] = ' ';
    name[1] = ' ';
    name[2] = ' ';
    name[3] = ' ';
    name[4] = ' ';
    name[5] = ' ';
    name[6] = ' ';
    name[7] = '\0';

    int8_t ev = *((int8_t*)&stop);

    uint8_t sign = 0;
    if(ev < 0)
    {
        sign = 1;
        ev = 0 - ev;
    }
    uint8_t mod = ev % 3;
    ev /= 3;
    if(mod == 0)
    {
        if(sign) name[5] = '-'; else name[5] = '+';
        if(ev > 9)
        {
            name[4] = name[5];
            name[5] = '0' + ev / 10;
            ev %= 10;
            name[6] = '0' + ev;
        }
        else
        {
            name[6] = '0' + ev;
        }
    }
    else
    {
        if(sign) name[ev > 0 ? 1 : 3] = '-'; else name[ev > 0 ? 1 : 3] = '+';
        if(ev > 9)
        {
            name[0] = name[1];
            name[1] = '0' + ev / 10;
            ev %= 10;
            name[2] = '0' + ev;
        }
        else if(ev > 0)
        {
            name[2] = '0' + ev;
        }
        name[4] = '0' + mod;
        name[5] = '/';
        name[6] = '3';
    }
    return 1;
}

void calcBulbMax()
{
    BulbMax = (timer.current.Gap / 10 - BRAMP_GAP_PADDING) * 1000;

    BulbMaxEv = 1;
    for(uint8_t i = camera.bulbMax(); i < camera.bulbMin(); i++)
    {
        if(BulbMax > camera.bulbTime((int8_t)i))
        {
            if(i < 1) i = 1;
            BulbMaxEv = i - 1;
            break;
        }
    }

    BulbMax = camera.bulbTime((int8_t)BulbMaxEv);
}

uint8_t stopUp(uint8_t stop)
{
    int8_t ev = *((int8_t*)&stop);

    int8_t evMax = calcRampMax();

    if(ev < 30*3) ev++; else ev = 30*3;

    if(ev <= evMax) return ev; else return evMax;
}

uint8_t stopDown(uint8_t stop)
{
    int8_t ev = *((int8_t*)&stop);

    int8_t evMin = calcRampMin();

    if(ev > -30*3) ev--; else ev = -30*3;

    if(ev >= evMin) return ev; else return evMin;
}

int8_t calcRampMax()
{
    calcBulbMax();

    int8_t bulbRange = 0;
    int8_t isoRange = 0;
    int8_t apertureRange = 0;

    if(conf.brampMode & BRAMP_MODE_BULB) bulbRange = (int8_t)timer.current.BulbStart - (int8_t)BulbMaxEv;
    if(conf.brampMode & BRAMP_MODE_ISO) isoRange = (int8_t)camera.iso() - (int8_t)camera.isoMax();
    if(conf.brampMode & BRAMP_MODE_APERTURE) apertureRange = (int8_t)camera.aperture() - (int8_t)camera.apertureMin();

    return isoRange + apertureRange + bulbRange;
}

int8_t calcRampTarget(int8_t targetShutter, int8_t targetISO, int8_t targetAperture)
{
    calcBulbMax();

    int8_t bulbRange = 0;
    int8_t isoRange = 0;
    int8_t apertureRange = 0;

    if(conf.brampMode & BRAMP_MODE_BULB) bulbRange = (int8_t)timer.current.BulbStart - targetShutter;
    if(conf.brampMode & BRAMP_MODE_ISO) isoRange = (int8_t)camera.iso() - targetISO;
    if(conf.brampMode & BRAMP_MODE_APERTURE) apertureRange = (int8_t)camera.aperture() - targetAperture;

    return isoRange + apertureRange + bulbRange;
}

int8_t calcRampMin()
{
    int8_t bulbRange = 0;
    int8_t isoRange = 0;
    int8_t apertureRange = 0;

    if(conf.brampMode & BRAMP_MODE_BULB)
    {
        if(conf.extendedRamp)
        {
            uint8_t shutterMax = PTP::shutterMax();
            if(shutterMax > MAX_EXTENDED_RAMP_SHUTTER) shutterMax = MAX_EXTENDED_RAMP_SHUTTER;
            bulbRange = (int8_t)MAX_EXTENDED_RAMP_SHUTTER - (int8_t)timer.current.BulbStart;
        }
        else
        {
            bulbRange = (int8_t)camera.bulbMin() - (int8_t)timer.current.BulbStart;
        }        
    }
    if((conf.brampMode & BRAMP_MODE_ISO) && camera.supports.iso) isoRange = (int8_t)camera.isoMin() - (int8_t)camera.iso();
    if((conf.brampMode & BRAMP_MODE_APERTURE) && camera.supports.aperture) apertureRange = (int8_t)camera.apertureMax() - (int8_t)camera.aperture();

    return 0 - (isoRange + apertureRange + bulbRange);
}

uint8_t checkHDR(uint8_t exps, uint8_t mid, uint8_t bracket)
{
    uint8_t up = mid - (exps / 2) * bracket;
    uint8_t down = mid + (exps / 2) * bracket;

    DEBUG(PSTR("up: "));
    DEBUG(up);
    DEBUG_NL();
    DEBUG(PSTR("down: "));
    DEBUG(down);
    DEBUG_NL();
    DEBUG(PSTR("max: "));
    DEBUG(camera.shutterMax());
    DEBUG_NL();
    DEBUG(PSTR("min: "));
    DEBUG(camera.shutterMin());
    DEBUG_NL();

    if(up < camera.shutterMax() || down > camera.shutterMin()) return 1; else return 0;
}

uint8_t hdrTvUp(uint8_t ev)
{
    if(checkHDR(timer.current.Exps, ev, timer.current.Bracket))
    {
        uint8_t mid = (camera.shutterMin() - camera.shutterMax()) / 2 + camera.shutterMax();
        if(mid > ev)
        {
            for(uint8_t i = ev; i <= mid; i++)
            {
                if(!checkHDR(timer.current.Exps, i, timer.current.Bracket))
                {
                    ev = i;
                    break;
                }
            }
        }
        else
        {
            for(uint8_t i = ev; i >= mid; i--)
            {
                if(!checkHDR(timer.current.Exps, i, timer.current.Bracket))
                {
                    ev = i;
                    break;
                }
            }
        }
    }
    else
    {
        uint8_t tmp = camera.shutterUp(ev);
        if(!checkHDR(timer.current.Exps, tmp, timer.current.Bracket)) ev = tmp;
    }
    return ev;
}

uint8_t hdrTvDown(uint8_t ev)
{
    if(checkHDR(timer.current.Exps, ev, timer.current.Bracket))
    {
        uint8_t mid = (camera.shutterMin() - camera.shutterMax()) / 2 + camera.shutterMax();
        if(mid > ev)
        {
            for(uint8_t i = ev; i <= mid; i++)
            {
                if(!checkHDR(timer.current.Exps, i, timer.current.Bracket))
                {
                    ev = i;
                    break;
                }
            }
        }
        else
        {
            for(uint8_t i = ev; i >= mid; i--)
            {
                if(!checkHDR(timer.current.Exps, i, timer.current.Bracket))
                {
                    ev = i;
                    break;
                }
            }
        }
    }
    else
    {
        uint8_t tmp = camera.shutterDown(ev);
        if(!checkHDR(timer.current.Exps, tmp, timer.current.Bracket)) ev = tmp;
    }
    return ev;
}

uint8_t bracketUp(uint8_t ev)
{
    uint8_t max = 1;
    for(uint8_t i = 1; i < 255; i++)
    {
        if(checkHDR(timer.current.Exps, timer.current.Exp, i))
        {
            max = i - 1;
            break;
        }
    }
    if(ev < max) ev++; else ev = max;
    return ev;
}

uint8_t bracketDown(uint8_t ev)
{
    if(ev > 1) ev--; else ev = 1;
    return ev;
}

uint8_t hdrExpsUp(uint8_t hdr_exps)
{
    uint8_t max = 1;
    for(uint8_t i = 1; i < 255; i++)
    {
        if(checkHDR(i, timer.current.Exp, timer.current.Bracket))
        {
            max = i - 1;
            if(max < 3) max = 3;
            break;
        }
    }
    if(hdr_exps < max) hdr_exps++; else hdr_exps = max;
    return hdr_exps;
}

uint8_t hdrExpsDown(uint8_t hdr_exps)
{
    if(hdr_exps > 3) hdr_exps--; else hdr_exps = 3;
    return hdr_exps;
}

uint8_t hdrExpsName(char name[8], uint8_t hdr_exps)
{
    name[0] = ' ';
    name[1] = ' ';
    name[2] = ' ';
    name[3] = ' ';
    name[4] = ' ';
    name[5] = ' ';
    name[6] = ' ';
    name[7] = '\0';

    if(hdr_exps > 9)
    {
        name[5] = '0' + (hdr_exps / 10);
        hdr_exps %= 10;
    }
    name[6] = '0' + hdr_exps;

    return 1;
}

uint8_t rampTvUp(uint8_t ev)
{
    uint8_t tmp = PTP::bulbUp(ev);
    if(tmp > camera.bulbMin()) tmp = camera.bulbMin();
    return tmp;
}

uint8_t rampTvUpExtended(uint8_t ev)
{
    uint8_t tmp = PTP::shutterUp(ev);
    if(tmp == 254) tmp = ev;
    if(tmp > MAX_EXTENDED_RAMP_SHUTTER) tmp = MAX_EXTENDED_RAMP_SHUTTER;
    return tmp;
}

uint8_t rampTvUpStatic(uint8_t ev)
{
    uint8_t tmp = PTP::bulbUp(ev);
    //if(tmp > camera.bulbMin()) tmp = camera.bulbMin();
    return tmp;
}

uint8_t rampTvDown(uint8_t ev)
{
    calcBulbMax();
    uint8_t tmp = PTP::bulbDown(ev);
    if(tmp < BulbMaxEv) tmp = BulbMaxEv;
    return tmp;
}

uint8_t rampTvDownExtended(uint8_t ev)
{
    calcBulbMax();
    uint8_t tmp = PTP::shutterDown(ev);
    if(tmp < BulbMaxEv) tmp = BulbMaxEv;
    return tmp;
}


