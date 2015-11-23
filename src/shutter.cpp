/*
 *  shutter.cpp
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *	Licensed under GPLv3
 *
 */

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
#include "Menu.h"
#include "remote.h"
#include "nmx.h"

#define RUN_DELAY 0
#define RUN_BULB 1
#define RUN_PHOTO 2
#define RUN_GAP 3
#define RUN_NEXT 4
#define RUN_END 5
#define RUN_ERROR 6
#define RUN_EXTERNAL_TRIGGER 7
#define RUN_EXTERNAL_TRIGGER_RELEASE 8

extern IR ir;
extern PTP camera;
extern settings_t conf;
extern shutter timer;
extern BT bt;
extern Clock clock;
extern Light light;
extern MENU menu;
extern LCD lcd;
extern Button button;
extern Remote remote;

extern NMX motor1;
extern NMX motor2;
extern NMX motor3;


volatile unsigned char state;
const uint16_t settings_warn_time = 0;
const uint16_t settings_mirror_up_time = 3;
volatile char cable_connected; // 1 = cable connected, 0 = disconnected

char shutter_state, ir_shutter_state; // used only for momentary toggle mode //
uint32_t BulbMax; // calculated during bulb ramp mode
program stored[MAX_STORED+1]EEMEM;
uint8_t BulbMaxEv;

uint8_t lastShutterError = 0;
uint8_t usbPrimary = 0;
int32_t focusPos = 0;
uint8_t turnOffLV = 0;

uint32_t motionMS = 0, motionCorrection = 0;

const char STR_BULBMODE_ALERT[]PROGMEM = "Please make sure the camera is in bulb mode before continuing";
const char STR_BULBSUPPORT_ALERT[]PROGMEM = "Bulb mode not supported via USB. Use an adaptor cable with the USB";
const char STR_MEMORYSPACE_ALERT[]PROGMEM = "Please confirm there is enough space on the memory card in the camera";
const char STR_APERTURE_ALERT[]PROGMEM = "Note that using an aperture other than the maximum without lens-twist can cause flicker";
const char STR_DEVMODE_ALERT[]PROGMEM = "DEV Mode can interfere with the light sensor. Please disable before continuing";
const char STR_ZEROLENGTH_ALERT[]PROGMEM = "Length greater than zero required in bulb ramp mode. Set the length before running";
const char STR_NOCAMERA_ALERT[]PROGMEM = "Camera not connected. Connect a camera or press ok to use the IR interface";
const char STR_LOWLIGHT_ALERT[]PROGMEM = "Low light for auto bulb ramping. Will not track night to day exposure optimally";

/******************************************************************
 *
 *   shutter::shutter
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
    current.Name[0] = 'S';
    current.Name[1] = 'U';
    current.Name[2] = 'N';
    current.Name[3] = 'S';
    current.Name[4] = 'E';
    current.Name[5] = 'T';
    current.Name[6] = '\0';
    current.Name[7] = '\0';
    current.Delay = 5;
    current.Photos = 500;
    current.Gap = 360;
    current.GapMin = BRAMP_INTERVAL_MIN;
    current.IntervalMode = INTERVAL_MODE_AUTO;
    current.Exps = 3;
    current.Mode = MODE_BULB_RAMP;
    current.Exp = 46;
    current.ArbExp = 100;
    current.Bracket = 6;
    current.Duration = 240;
    current.BulbStart = 56;
    current.brampMethod = BRAMP_METHOD_AUTO;
    current.nightMode = BRAMP_TARGET_AUTO;
    current.nightISO = 31;
    current.nightShutter = 31;
    current.nightAperture = 9;
    current.nightND = 0;

    resetKeyframes();

    wdt_reset();
    save(1);

    current.Name[0] = 'N';
    current.Name[1] = 'I';
    current.Name[2] = 'G';
    current.Name[3] = 'H';
    current.Name[4] = 'T';
    current.Name[5] = '\0';
    current.Name[6] = '\0';
    current.Name[7] = '\0';
    current.Delay = 5;
    current.Photos = 400;
    current.Gap = 300;
    current.Mode = MODE_TIMELAPSE;
    current.Exp = 254;
    current.ArbExp = 20000;

    resetKeyframes();

    wdt_reset();
    save(2);

    current.Name[0] = 'D';
    current.Name[1] = 'A';
    current.Name[2] = 'Y';
    current.Name[3] = 'T';
    current.Name[4] = 'I';
    current.Name[5] = 'M';
    current.Name[6] = 'E';
    current.Name[7] = '\0';
    current.Delay = 5;
    current.Photos = 500;
    current.Gap = 30;
    current.Mode = MODE_TIMELAPSE;
    current.Exp = 254;
    current.ArbExp = 1000;

    resetKeyframes();

    wdt_reset();
    save(0);

    uint8_t i;
    for(i = 3; i < MAX_STORED; i++)
    {
        wdt_reset();
        eeprom_write_byte((uint8_t*)&stored[i].Name[0],  255);
    }

    wdt_reset();
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
    
    if((conf.cameraPort == AUX_CAM_DOLLY && conf.auxPort == AUX_MODE_SYNC)) return; // disable if we're using the camera port for MotionSync

    SHUTTER_CLOSE;
    MIRROR_DOWN; 
    clock.in(20, &check_cable);
    ir_shutter_state = 0;
    shutter_state = 0;
}
void shutter_off_quick(void)
{
    if((conf.cameraPort == AUX_CAM_DOLLY && conf.auxPort == AUX_MODE_SYNC)) return; // disable if we're using the camera port for MotionSync

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
    if((conf.cameraPort == AUX_CAM_DOLLY && conf.auxPort == AUX_MODE_SYNC)) return; // disable if we're using the camera port for MotionSync

    shutter_off(); // first we completely release the shutter button since some cameras need this to release the bulb

    if(conf.camera.halfPress == HALF_PRESS_ENABLED) clock.in(30, &shutter_half_delayed);
}
void shutter_half_delayed(void)
{
    if(conf.devMode) 
        hardware_flashlight(0);
    
    if((conf.cameraPort == AUX_CAM_DOLLY && conf.auxPort == AUX_MODE_SYNC)) return; // disable if we're using the camera port for MotionSync

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

    if((conf.cameraPort == AUX_CAM_DOLLY && conf.auxPort == AUX_MODE_SYNC)) return; // disable if we're using the camera port for MotionSync

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
    shutter_bulbStartPrepare();
    shutter_bulbStartQuick();
}
void shutter_bulbStartQuick(void)
{
    lastShutterError = 0;
    if((cable_connected == 0 || usbPrimary || !(conf.camera.interface & INTERFACE_CABLE)) && ir_shutter_state != 1)
    {
        if(camera.supports.bulb && (conf.camera.interface & INTERFACE_USB))
        {
            lastShutterError = camera.bulbMode();
            if(lastShutterError) return;
            lastShutterError = camera.bulbStart();
        }
        else if(conf.camera.interface & INTERFACE_IR && !usbPrimary)
        {
            ir_shutter_state = 1;
            ir.bulbStart();
        }
    }

    if(conf.camera.interface & INTERFACE_CABLE && !usbPrimary)
    {
        if(conf.camera.bulbMode == 0)
        {
            shutter_full();
        } 
        else if(conf.camera.bulbMode == 1 && shutter_state != 1)
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
void shutter_bulbStartPrepare(void)
{
    if(camera.ready && conf.camera.cameraMake == SONY) // hack to allow Sony to save to SD card
    {
        shutter_off_quick();
        camera.disable();
        _delay_ms(500);
        shutter_half();
        _delay_ms(1000);
    }
}

//Error: 1001:2001
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
    shutter_bulbEndQuick();
    shutter_bulbEndFinish();
}
void shutter_bulbEndQuick(void)
{
    if(camera.bulb_open || ir_shutter_state == 1)
    {
        if(camera.bulb_open && (conf.camera.interface & INTERFACE_USB))
        {
            // USB
            camera.bulbEnd();
        }
        else if(conf.camera.interface & INTERFACE_IR && !usbPrimary)
        {
            // IR
            ir.bulbEnd();
        }
        ir_shutter_state = 0;
    } 
    if(conf.camera.interface & INTERFACE_CABLE && !usbPrimary)
    {
        if(conf.camera.bulbMode == 0)
        {
            // CABLE
            shutter_off();
        }
        else if(conf.camera.bulbMode == 1 && shutter_state == 1)
        {
            shutter_full();
            shutter_state = 0;
            clock.in(SHUTTER_PRESS_TIME, &shutter_off);
        }
    }
}
void shutter_bulbEndFinish(void)
{
    if(camera.disabled && conf.camera.cameraMake == SONY) {
        camera.enable();
    }
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
    if(camera.ready && conf.camera.cameraMake == SONY) // hack to allow Sony to save to SD card
    {
        shutter_off_quick();
        camera.disable();
        wdt_reset();
        _delay_ms(500);
        shutter_half();
        _delay_ms(1500);
        wdt_reset();
    }
    lastShutterError = 0;
    if(conf.camera.interface & (INTERFACE_CABLE | INTERFACE_USB))
    {
        shutter_full();
        if(conf.camera.cameraMake == SONY)
        {
            clock.in(SHUTTER_PRESS_TIME, &shutter_off);
        }
        else
        {
            clock.in(200, &shutter_off);
        }
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
                if(conf.camera.interface & INTERFACE_IR) ir.shutterNow();
            }
        }
        else
        {
            if(camera.supports.capture) camera.busy = true;
        }
    }
    else if(conf.camera.interface == INTERFACE_IR)
    {
        ir.shutterNow();
    }
    shutter_bulbEndFinish();
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
    //focusPos = 0;
    if(camera.modeLiveView)
    {
        camera.liveView(false);
        _delay_ms(1000);
        camera.checkEvent();
    }
    paused = 0;
    pausing = 0;
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
    static uint8_t enter, exps, run_state = RUN_DELAY, old_state = 255;
    static uint16_t photos;
    static uint32_t last_photo_end_ms;
    static uint8_t usingUSB;
    static int8_t aperturePausedStart;

    if(MIRROR_IS_DOWN)
    {
        CHECK_CABLE;
    }
    
    if(!running)
    {
        if(turnOffLV && menu.state != ST_KEYFRAME && camera.supports.focus && camera.modeLiveView)
        {
            camera.liveView(0);
        }

        if(enter) 
            cancel = 1;
        else 
            return 0;
    }

    if((enter == 0 || status.preChecked_u8 > 0) && cancel == 0) // Initialize variables and setup I/O pins
    {
        enter = 1;
        paused = 0;
        pausing = 0;
        if(current.Mode & RAMP)
        {
            uint32_t tmp = (uint32_t)current.Duration * 10 * 60;  //J.R.
            tmp /= (uint32_t) current.Gap;
            current.Photos = (uint16_t) tmp;
        }
        status.interval_u16 = current.Gap;

        usingUSB = camera.ready;

        if(conf.camera.interface == INTERFACE_AUTO && usingUSB && camera.supports.bulb) usbPrimary = 1; else usbPrimary = 0;

        ////////////////////////////// pre check ////////////////////////////////////
        CHECK_ALERT(STR_NOCAMERA_ALERT, conf.camera.interface != INTERFACE_IR && !camera.supports.capture && !cableIsConnected());
        CHECK_ALERT(STR_APERTURE_ALERT, camera.ready && camera.supports.aperture && camera.aperture() != camera.apertureWideOpen() && (current.Mode & TIMELAPSE) && !((current.Mode & RAMP) && (conf.brampMode & BRAMP_MODE_APERTURE)));
        CHECK_ALERT(STR_BULBSUPPORT_ALERT, (current.Mode & RAMP) && camera.ready && !camera.supports.bulb && !cable_connected);
        CHECK_ALERT(STR_BULBMODE_ALERT, (current.Mode & RAMP) && !(camera.isInBulbMode() || conf.extendedRamp));
        CHECK_ALERT(STR_MEMORYSPACE_ALERT, (current.Photos > 0) && camera.ready && camera.photosRemaining && camera.photosRemaining < current.Photos && (current.Mode & TIMELAPSE));
        CHECK_ALERT(STR_DEVMODE_ALERT, (current.Mode & RAMP) && (current.brampMethod == BRAMP_METHOD_AUTO) && conf.devMode);
        CHECK_ALERT(STR_ZEROLENGTH_ALERT, (current.Mode & RAMP) && (current.Duration == 0));
        CHECK_ALERT(STR_LOWLIGHT_ALERT, (current.Mode & RAMP) && (current.brampMethod == BRAMP_METHOD_AUTO) && (light.underThreshold));

        if(!status.preChecked_u8 && menu.waitingAlert())
        {
            menu.blink();
        }

        if(status.preChecked_u8 == 0) status.preChecked_u8 = 1;
        if(menu.waitingAlert()) return CONTINUE; //////////////////////////////////////////////

        if(status.preChecked_u8 == 1)
        {
            switchBack = current.brampMethod;

            menu.message(TEXT("Loading"));
            menu.task();

            moveToStart();
            if(camera.modeLiveView)
            {
                camera.liveView(false);
                _delay_ms(300);
            }
            while(remote.nmx && (motor1.running() || motor2.running() || motor3.running())) // wait for move to start
            {
              wdt_reset();
            }

            if(current.Mode & RAMP)
            {
                memset(&pastErrors, 0, sizeof(pastErrors));
                calcBulbMax();
                status.rampMax_i8 = calcRampMax();
                status.rampMin_i8 = calcRampMin();
                rampRate = 0;
                status.rampStops_f = 0.0;
                light.integrationStart(conf.lightIntegrationMinutes);
                lightReading = status.lightStart_f = light.readIntegratedEv();

                if(current.nightMode == BRAMP_TARGET_AUTO)
                {
                    status.nightTarget_i8 = 0;
                    status.rampTarget_f = (float)status.nightTarget_i8;
                }
                else if(current.nightMode == BRAMP_TARGET_CUSTOM)
                {
                    status.nightTarget_i8 = calcRampTarget(current.nightShutter, current.nightISO, current.nightAperture);
                    status.rampTarget_f = (float)status.nightTarget_i8 + ((float)current.nightND * 3) / 10.0;
                    if(current.nightND > 0)
                    {
                        status.hasND_u8 = 1;
                    }
                    else
                    {
                        status.hasND_u8 = 0;
                    }
                    status.showND_u8 = 0;
                    ndShift = 0.0;
                }
                else
                {
                    status.nightTarget_i8 = ((int8_t)current.nightMode) - BRAMP_TARGET_OFFSET;
                    status.rampTarget_f = status.lightStart_f - (float)status.nightTarget_i8;
                }
                
                status.preChecked_u8 = 2;
            }
            
            if(!(current.Mode & RAMP) || current.nightMode == BRAMP_TARGET_AUTO || current.brampMethod != BRAMP_METHOD_AUTO)
            {
                status.preChecked_u8 = 3;
            }


            if(camera.ready)
            {
              _delay_ms(500);
              camera.checkEvent();
              if(camera.supports.aperture) aperture = camera.aperture();
              if(camera.supports.iso) iso = camera.iso();
              camera.setFocus(false);
            }
        }

        if(status.preChecked_u8 < 3) return CONTINUE;

        status.preChecked_u8 = 0;

        clock.tare();
        clock.reset();

        run_state = RUN_DELAY;
        photos = 0;
        exps = 0;

        current.infinitePhotos = (current.Mode & RAMP) ? (current.IntervalMode == INTERVAL_MODE_EXTERNAL) : (current.Photos == 0 ? 1 : 0);
        status.infinitePhotos_u8 = current.infinitePhotos;
        status.photosRemaining_u16 = current.Photos;
        status.photosTaken_u16 = 0;
        status.mode_u8 = (uint8_t) current.Mode;
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
        if(pausing > 8)
        {
            if(status.showND_u8)
            {
                ndShift = ((float)current.nightND * 3) / 10.0;
                status.hasND_u8 = 0;
                status.showND_u8 = 0;
            }
            else
            {
                paused = 0;
                pausing = 0;
                evShift += apertureEvShift;
                status.rampMax_i8 -= apertureEvShift;
                status.rampMin_i8 -= apertureEvShift;
                apertureEvShift = 0;
                aperturePausedStart = 0;
                apertureReady = 0;
            }
        }
        else
        {
            apertureReady = 0;
            pausing++;
        }
    }

    if(paused)
    {
        //if(status.hasND_u8)
        //{
        //    status.showND_u8 = 1;
        //}

        if(aperturePausedStart == -1 && camera.supports.aperture && camera.aperture() > 0 && camera.aperture() != 255)
        {
            aperturePausedStart = camera.aperture();
        }
        else if(camera.supports.aperture && aperturePausedStart > 0)
        {
            int8_t rampCurrent = (int8_t) status.rampStops_f;
            if(status.rampStops_f > (float) rampCurrent) rampCurrent++;
            else if(status.rampStops_f < (float) rampCurrent) rampCurrent--;

            int8_t avMax = aperturePausedStart + (status.rampMax_i8 - rampCurrent); // max stops darker
            int8_t avMin = aperturePausedStart + (status.rampMin_i8 - rampCurrent); // max stops brighter

            if(avMax > conf.apertureMax) avMax = conf.apertureMax;
            if(avMin < conf.apertureMin) avMin = conf.apertureMin;

            if(avMax > camera.apertureMax()) avMax = camera.apertureMax();
            if(avMin < camera.apertureMin()) avMin = camera.apertureMin();

            if(camera.aperture() != avMin) camera.setAperture(avMin);

            apertureEvShift = camera.aperture() - aperturePausedStart;
            apertureReady = 1;
        }
        else if(!status.showND_u8)
        {
            apertureReady = 0;

            // limit any manual changes to the limits
            int8_t rampCurrent = (int8_t) status.rampStops_f;
            if(status.rampStops_f > (float) rampCurrent) rampCurrent++;
            else if(status.rampStops_f < (float) rampCurrent) rampCurrent--;
            if(apertureEvShift > status.rampMax_i8 - rampCurrent) apertureEvShift = status.rampMax_i8 - rampCurrent; // max stops darker
            if(apertureEvShift < status.rampMin_i8 - rampCurrent) apertureEvShift = status.rampMin_i8 - rampCurrent; // max stops brighter

        }
        return CONTINUE;
    }
    else
    {
        if(status.hasND_u8)
        {
            //int8_t rampCurrent = (int8_t) status.rampStops_f;
            //if(status.rampStops_f > (float) rampCurrent) rampCurrent++;
            //else if(status.rampStops_f < (float) rampCurrent) rampCurrent--;

            //if((float)(status.rampMax_i8 - rampCurrent) > ((float)(current.nightND * 3) / 10.0))
            //{
            //    status.showND_u8 = 1;
            //}

            if(status.rampStops_f + 1.0 > ((float)(current.nightND * 3) / 10.0))
            {
                status.showND_u8 = 1;
            }
        }
    }

    if(pausing && run_state != RUN_BULB)
    {
        apertureReady = 0;
        pausing = 0;
        paused = 1;
        apertureEvShift = 0;
        if(!camera.supports.aperture && (current.Mode & RAMP)) aperturePausedStart = -1; else aperturePausedStart = 0;
    }

    /////////////////////////////////////////

    if(cancel)
    {
        run_state = RUN_END;
    }

    /////// RUNNING PROCEDURE ///////
    if(run_state == RUN_DELAY)
    {
        if(old_state != run_state)
        {
            strcpy((char *) status.textStatus, TEXT("Delay"));
            old_state = run_state;
        }


        if(((unsigned long)clock.event_ms / 1000) > current.Delay)
        {
            clock.tare();
            clock.reset();
            motionMS = 0;
            motionCorrection = 0;
            last_photo_ms = 0;
            run_state = RUN_PHOTO;
        } 
        else
        {
            status.nextPhoto_u16 = (unsigned int) (current.Delay - (unsigned long)clock.event_ms / 1000);
            if(((unsigned long)clock.event_ms / 1000) + settings_mirror_up_time >= current.Delay)
            {
                // Mirror Up //
                shutter_half(); // This is to wake up the camera (even if USB is connected)
                _delay_ms(100);
                if(usbPrimary) shutter_off();
            }

            if((settings_warn_time > 0) && (((unsigned long)clock.event_ms / 1000) + settings_warn_time >= current.Delay))
            {
                // Flash Light //
                _delay_ms(50);
            }
            if(current.IntervalMode == INTERVAL_MODE_EXTERNAL) run_state = RUN_EXTERNAL_TRIGGER;
        }
    }

    if(run_state == RUN_PHOTO && (exps + photos == 0 || (uint8_t)((clock.Ms() - last_photo_end_ms) / 10) >= conf.camera.cameraFPS))
    {
        movedFocus = 0;
        last_photo_end_ms = 0;
        if(old_state != run_state)
        {
            strcpy((char *) status.textStatus, TEXT("Photo"));
            old_state = run_state;

            if(camera.ready && timer.current.Mode & RAMP && conf.extendedRamp && !camera.isInBulbMode() && conf.camera.modeSwitch == USB_CHANGE_MODE_ENABLED && timer.status.bulbLength_u16 > camera.bulbTime((int8_t)camera.bulbMin()))
            {
                camera.bulbMode();
            }
            else if(camera.ready && timer.current.Mode & RAMP && conf.extendedRamp && camera.isInBulbMode() && conf.camera.modeSwitch == USB_CHANGE_MODE_ENABLED && timer.status.bulbLength_u16 < camera.bulbTime((int8_t)camera.bulbMin()))
            {
                camera.manualMode();
            }
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
            
            if(status.interval_u16 <= settings_mirror_up_time && !camera.ready) 
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

            strcpy((char *) status.textStatus, TEXT("Bulb"));

            m = camera.shutterType(current.Exp);
            if(m & SHUTTER_MODE_BULB || conf.arbitraryBulb)
            {
                if(conf.arbitraryBulb)
                {
                    exp = current.ArbExp;
                    exp *= 100;
                    m = SHUTTER_MODE_BULB;
                }
                else
                {
                    exp = camera.bulbTime((int8_t)current.Exp);
                }
            }

            if(current.Mode & RAMP)
            {
                //char found = 0;
                m = SHUTTER_MODE_BULB;
                shutter_off();


                if(current.brampMethod == BRAMP_METHOD_KEYFRAME) //////////////////////////////// KEYFRAME RAMP /////////////////////////////////////
                {
                    // Keyframe Bulb ramp algorithm goes here
                    status.rampStops_f = interpolateKeyframe(&current.kfExposure, clock.Ms());
                    exp = camera.bulbTime((float)current.BulbStart - status.rampStops_f - (float)evShift + ndShift);
                }

                else if(current.brampMethod == BRAMP_METHOD_GUIDED || current.brampMethod == BRAMP_METHOD_AUTO) //////////////////////////////// GUIDED / AUTO RAMP /////////////////////////////////////
                {

                    //#################### AUTO BRAMP ####################
                    if(current.brampMethod == BRAMP_METHOD_AUTO)
                    {
                        if(light.underThreshold && current.nightMode != BRAMP_TARGET_AUTO) //  holding night exposure
                        {
                            if(current.nightMode == BRAMP_TARGET_CUSTOM)
                            {
                                status.rampTarget_f = (float)status.nightTarget_i8 + ((float)current.nightND * 3) / 10.0;
                            }
                            else
                            {
                                status.rampTarget_f = status.lightStart_f - (float)status.nightTarget_i8; // hold at night exposure
                            }
                        }
                        else
                        {
                            // using light sensor target
                            status.rampTarget_f = (status.lightStart_f - lightReading);
                        }

                        if(status.rampTarget_f > status.rampMax_i8) status.rampTarget_f = status.rampMax_i8;
                        if(status.rampTarget_f < status.rampMin_i8) status.rampTarget_f = status.rampMin_i8;
                        float delta = status.rampTarget_f - status.rampStops_f;

                        float pastError = 0.0;
                        for(uint8_t i = 0; i < PAST_ERROR_COUNT; i++)
                        {
                            pastError += pastErrors[i];
                            if(i < PAST_ERROR_COUNT - 1) pastErrors[i] = pastErrors[i + 1];
                        }
                        pastError /= PAST_ERROR_COUNT;
                        pastErrors[PAST_ERROR_COUNT - 1] = delta;

                        if(delta != 0)
                        {
                            delta *= P_FACTOR;
                            if(!light.underThreshold) delta += pastError * I_FACTOR;

                            if(light.lockedSlope > 0.0 && current.nightMode != BRAMP_TARGET_AUTO)
                            {
                                if(light.lockedSlope < delta)
                                {
                                  delta = light.lockedSlope; // hold the last valid slope reading from the light sensor
                                  float minRate = (status.rampTarget_f - status.rampStops_f) / 4.5; // make sure to reach target within 1.5 hours
                                  if(delta < minRate) delta = minRate;
                                }
                            }
                            else
                            {
                                delta += light.slope * D_FACTOR;
                            }

                        }

                        rampRate = (int8_t) delta;

                        if(rampRate == 0 && light.underThreshold && current.nightMode != BRAMP_TARGET_AUTO && status.rampStops_f == status.rampTarget_f)
                        {
                          // if we've met the night target, switch to guided mode to hold exposure
                          switchToGuided();
                        }
                    }
                    //####################################################

                    status.rampStops_f += ((float)rampRate / (3600.0 / 3)) * ((float)status.interval_u16 / 10.0);

                    if(status.rampStops_f >= status.rampMax_i8)
                    {
                        rampRate = 0;
                        status.rampStops_f = status.rampMax_i8;
                    }
                    else if(status.rampStops_f <= status.rampMin_i8)
                    {
                        rampRate = 0;
                        status.rampStops_f = status.rampMin_i8;
                    }
                    exp = camera.bulbTime(current.BulbStart - status.rampStops_f - (float)evShift + ndShift);
                }

                bulb_length = exp;

                if(current.IntervalMode == INTERVAL_MODE_AUTO) // Auto Interval
                {
                    float intPercent = (status.rampStops_f + ((float)camera.bulbMin() - (float)current.BulbStart)) / (float) ((int8_t)camera.bulbMin() - (int8_t)BulbMaxEv);

                    if(intPercent > 1.0) intPercent = 1.0;
                    if(intPercent < 0.0) intPercent = 0.0;

                    status.interval_u16 = current.GapMin + (uint16_t)(intPercent * (float)(current.Gap - current.GapMin));
                }
                else if(current.IntervalMode == INTERVAL_MODE_KEYFRAME) // Keyframe Interval
                {
                    status.interval_u16 = interpolateKeyframe(&current.kfInterval, clock.Ms());
                }
                else if(current.IntervalMode == INTERVAL_MODE_FIXED)// Fixed Interval
                {
                    status.interval_u16 = current.Gap;
                }

                calculateExposure(&bulb_length, &aperture, &iso, &evShift);

                shutter_off_quick(); // Can't change parameters when half-pressed
                if((conf.brampMode & BRAMP_MODE_APERTURE) && camera.supports.aperture)
                {
                    // Change the Aperture //
                    if(camera.aperture() != aperture)
                    {
                        if(camera.setAperture(aperture) == PTP_RETURN_ERROR)
                        {
                            run_state = RUN_ERROR;
                            return CONTINUE;
                        }
                    }
                }
                if((conf.brampMode & BRAMP_MODE_ISO) && camera.supports.iso)
                {
                    // Change the ISO //
                    if(camera.iso() != iso)
                    {
                        if(camera.setISO(iso) == PTP_RETURN_ERROR)
                        {
                            run_state = RUN_ERROR;
                            return CONTINUE;
                        }
                    }
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
                        //DEBUG(PSTR("Shutter Mode PTP\r\n"));
                        camera.setShutter(current.Exp - tv_offset);
                        bulb_length = camera.bulbTime((int8_t)(current.Exp - tv_offset));
                    }
                    else
                    {
                        camera.bulbMode();
                        //DEBUG(PSTR("Shutter Mode BULB\r\n"));
                        m = SHUTTER_MODE_BULB;
                        bulb_length = camera.bulbTime((int8_t)(current.Exp - tv_offset));
                    }
                }

            }
            
            if((current.Mode & (HDR | RAMP)) == 0) //basic time-lapse, set shutter speed
            {
                bulb_length = exp;
                if(m & SHUTTER_MODE_PTP)
                {
                    shutter_off_quick(); // Can't change parameters when half-pressed
                    camera.manualMode();
                    camera.setShutter(current.Exp);
                }
            }

            status.bulbLength_u16 = bulb_length;

            if(current.Mode & RAMP && (!camera.isInBulbMode() && camera.ready && camera.supports.shutter)) // bulb ramp, manual mode shutter
            {
                camera.setShutter(camera.bulbToShutterEv(bulb_length));
                shutter_capture();
                _delay_ms(10);
                if(lastShutterError)
                {
                    clock.cancelBulb();
                    DEBUG(PSTR("USB Error - trying again\r\n"));
                    old_state = 0;
                    if(PTP_Error) run_state = RUN_ERROR;
                    return CONTINUE; // try it again
                }
            }
            else if(m & SHUTTER_MODE_BULB) // bulb mode
            {                
                camera.bulb_open = true;
                shutter_bulbStartPrepare();           		
                clock.bulb(bulb_length);
                _delay_ms(10);
				
				
                if(lastShutterError)
                {
                    clock.cancelBulb();
                    DEBUG(PSTR("USB Error - trying again\r\n"));
                    old_state = 0;
                    if(PTP_Error) run_state = RUN_ERROR;
                    return CONTINUE; // try it again
                }
            }
            else // basic capture
            {
                shutter_capture();
            }
        }
        else if(!clock.bulbRunning)
        {
            exps++;
            lightReading = light.readIntegratedEv();

            shutter_bulbEndFinish();

            _delay_ms(50);

            if(status.interval_u16 <= settings_mirror_up_time && !camera.ready) 
                shutter_half(); // Mirror Up //

            run_state = RUN_NEXT;
            return CONTINUE; // run through loop once first before continuing
        }
    }
    
    if(run_state == RUN_NEXT)
    {
        if(old_state != run_state)
        {
            old_state = run_state;
            last_photo_end_ms = clock.Ms();
        }

        if(usingUSB && camera.disabled) return CONTINUE;

        if((PTP_Connected && PTP_Error) || (usingUSB && !camera.ready))
        {
            run_state = RUN_ERROR;
            return CONTINUE;
        }

        if((exps >= current.Exps && (current.Mode & HDR)) || (current.Mode & HDR) == 0)
        {
            exps = 0;
            photos++;
            clock.tare();
            run_state = RUN_GAP;
        }
        else if(!camera.busy) // begin the next exposure in the HDR set
        {
            clock.tare();
            run_state = RUN_PHOTO;
        }
    
        if(!current.infinitePhotos)
        {
            if(current.Mode & RAMP)
            {
                if(clock.Seconds() >= ((uint32_t)current.Duration * 60))  //J.R.
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
        else if(current.Mode & RAMP)
        {
          current.Duration = (uint16_t)(clock.Seconds() / 60) + 20;
        }

        status.photosRemaining_u16 = current.Photos - photos;
        status.photosTaken_u16 = photos;
    }

    if(run_state == RUN_GAP)
    {
        if(old_state != run_state)
        {
            movedFocus = 0;
            if(conf.auxPort == AUX_MODE_DOLLY || conf.cameraPort == AUX_CAM_DOLLY) aux_pulse();

            motionMS += ((uint32_t)status.interval_u16) * 100UL;

            uint32_t motionError = clock.Ms();
            if(motionError > motionMS)
            {
              motionError -= motionMS;
              motionError /= 64; // catchup on error over next 64 frames
              if(motionError > motionCorrection) motionCorrection = motionError;
            }
            else
            {
              motionCorrection = 0;
            }
            motionMS += motionCorrection;

            strcpy((char *) status.textStatus, TEXT("Waiting"));
            old_state = run_state;
            if(motor1.connected) moveMotor1((int32_t)interpolateKeyframe(&current.kfMotor1, motionMS));
            if(motor2.connected) moveMotor2((int32_t)interpolateKeyframe(&current.kfMotor2, motionMS));
            if(motor3.connected) moveMotor3((int32_t)interpolateKeyframe(&current.kfMotor3, motionMS));
        }

        uint32_t cms = clock.Ms();

        if(camera.supports.focus && movedFocus < 2 && conf.focusEnabled)
        {
            int32_t pos = (int32_t)interpolateKeyframe(&current.kfFocus, motionMS);
            if(pos != focusPos)
            {
                if(camera.busy)
                {
                  strcpy((char *) status.textStatus, TEXT("Busy"));
                  return CONTINUE;
                }
                if(movedFocus == 0)
                {
                    if((cms - last_photo_ms) / 100 >= BRAMP_GAP_PADDING)
                    {
                        movedFocus = 1;
                    }
                    return CONTINUE; // do event loop once first
                }
                movedFocus = 2;
                camera.setFocus(true);
                _delay_ms(100);
                wdt_reset();
                moveFocus(pos);
                _delay_ms(100);
                camera.setFocus(false);
                wdt_reset();
                if(camera.modeLiveView)
                {
                    camera.liveView(false);
                    _delay_ms(1000);
                }
                strcpy((char *) status.textStatus, TEXT("Waiting"));
            }
            else
            {
              movedFocus = 2;
            }
        }

        if(!motor1.running() && !motor2.running() && !motor3.running())
        {
            cms = clock.Ms();

            if((cms - last_photo_ms) / 100 >= status.interval_u16)
            {
                last_photo_ms = cms;
                clock.tare();
                run_state = RUN_PHOTO;
            } 
            else
            {
                status.nextPhoto_u16 = (unsigned int) ((status.interval_u16 - (cms - last_photo_ms) / 100) / 10);
                if((cms - last_photo_ms) / 100 + (uint32_t)settings_mirror_up_time * 10 >= status.interval_u16)
                {
                    // Mirror Up //
                    if(!camera.ready) shutter_half(); // Don't do half-press if the camera is connected by USB
                }
            }
            if(current.IntervalMode == INTERVAL_MODE_EXTERNAL)
            {
                run_state = RUN_EXTERNAL_TRIGGER;
            }
        }
    }
    
    if(run_state == RUN_EXTERNAL_TRIGGER)
    {
      // wait for external intervalometer trigger
      if(CHECK_EXTERNAL)
      {
        run_state = RUN_EXTERNAL_TRIGGER_RELEASE;
      }
      else if(photos > 2 && (clock.Ms() - last_photo_ms) / 1000 >= (uint32_t)status.interval_u16) // end if it's been 10 times the normal interval length
      {
        run_state = RUN_END;
      }
    }

    if(run_state == RUN_EXTERNAL_TRIGGER_RELEASE)
    {
      // wait for external intervalometer trigger to be released
      if(!CHECK_EXTERNAL)
      {
        if(photos > 1) status.interval_u16 = (uint16_t)((clock.Ms() - last_photo_ms) / 100);
        last_photo_ms = clock.Ms();
        clock.tare();
        run_state = RUN_PHOTO;
      }
      else if(photos > 2 && (clock.Ms() - last_photo_ms) / 1000 >= (uint32_t)status.interval_u16) // end if it's been 10 times the normal interval length
      {
        run_state = RUN_END;
      }
    }

    if(run_state == RUN_ERROR)
    {
        static uint8_t errorRetryCount = 0;
        if(old_state != run_state || errorRetryCount > 40)
        {
            errorRetryCount = 0;
            shutter_off();
            camera.bulbEnd();

            menu.blink();
            strcpy((char *) status.textStatus, TEXT("Error"));
            old_state = run_state;

            shutter_half();
            _delay_ms(100);
            shutter_off();
            camera.resetConnection();
        }

        errorRetryCount++;

        if(camera.ready)
        {
            errorRetryCount = 0;
            run_state = RUN_NEXT;
        }
        return CONTINUE;
    }

    if(run_state == RUN_END)
    {
        if(old_state != run_state)
        {
            strcpy((char *) status.textStatus, TEXT("Done"));
            old_state = run_state;
        }

        current.brampMethod = switchBack;
        pausing = 0;
        paused = 0;
        enter = 0;
        running = 0;
        shutter_off();
        camera.bulbEnd();
        hardware_flashlight(0);
        light.stop();
        aux1_off();
        aux2_off();
        clock.awake();
        usbPrimary = 0;
        status.preChecked_u8 = 0;
        //if(remote.nmx)
        //{
        //  motor1.disable();
        //  motor2.disable();
        //  motor3.disable();
        //}

        return DONE;
    }

    return CONTINUE;
}

void shutter::calculateExposure(uint32_t *nextBulbLength, uint8_t *nextAperture, uint8_t *nextISO, int8_t *bulbChangeEv)
{
    if(camera.supports.iso || camera.supports.aperture || camera.supports.shutter)
    {
        uint8_t aperture = *nextAperture, iso = *nextISO;

        uint32_t exp = *nextBulbLength;

        int8_t tmpShift = 0;

        if((conf.brampMode & BRAMP_MODE_APERTURE) && camera.supports.aperture)
        {
            // Check for too long bulb time and adjust Aperture //
            while(*nextBulbLength > BulbMax)
            {
                *nextAperture = camera.apertureDown(aperture);
                if(*nextAperture != aperture)
                {
                    *bulbChangeEv += *nextAperture - aperture;
                    tmpShift += *nextAperture - aperture;
                    aperture = *nextAperture;
                }
                else
                {
                    break;
                }
                *nextBulbLength = camera.shiftBulb(exp, tmpShift);
            }
        }

        if((conf.brampMode & BRAMP_MODE_ISO) && camera.supports.iso)
        {
            // Check for too long bulb time and adjust ISO //
            while(*nextBulbLength > BulbMax)
            {
                *nextISO = camera.isoUp(iso);
                if(*nextISO != iso)
                {
                    *bulbChangeEv += *nextISO - iso;
                    tmpShift += *nextISO - iso;
                    iso = *nextISO;
                }
                else
                {
                    break;
                }
                *nextBulbLength = camera.shiftBulb(exp, tmpShift);
            }
        }


        if((conf.brampMode & BRAMP_MODE_ISO) && camera.supports.iso)
        {
            // Check for too short bulb time and adjust ISO //
            for(;;)
            {
                *nextISO = camera.isoDown(iso);
                if(*nextISO != iso && *nextISO < 127)
                {
                    uint32_t bulb_length_test = camera.shiftBulb(exp, tmpShift + (*nextISO - iso));
                    if(bulb_length_test < BulbMax)
                    {
                        *bulbChangeEv += *nextISO - iso;
                        tmpShift += *nextISO - iso;
                        iso = *nextISO;
                        *nextBulbLength = bulb_length_test;
                        continue;
                    }
                }
                *nextISO = iso;
                break;
            }
        }


        if((conf.brampMode & BRAMP_MODE_APERTURE) && camera.supports.aperture)
        {
            // Check for too short bulb time and adjust Aperture //
            for(;;)
            {
                *nextAperture = camera.apertureUp(aperture);
                if(*nextAperture != aperture)
                {
                    if(*nextAperture >= 127) break;
                    uint32_t bulb_length_test = camera.shiftBulb(exp, tmpShift + (*nextAperture - aperture));
                    if(bulb_length_test < BulbMax)
                    {
                        *bulbChangeEv  += *nextAperture - aperture;
                        tmpShift += *nextAperture - aperture;
                        aperture = *nextAperture;
                        *nextBulbLength = camera.shiftBulb(exp, tmpShift);
                        continue;
                    }
                }
                *nextAperture = aperture;
                break;
            }
        }
        
        if(conf.extendedRamp)
        {
            if(*nextBulbLength < camera.bulbTime((int8_t)MAX_EXTENDED_RAMP_SHUTTER))
            {
                *nextBulbLength = camera.bulbTime((int8_t)MAX_EXTENDED_RAMP_SHUTTER);
            }
        }
        else
        {
            if(*nextBulbLength < camera.bulbTime((int8_t)camera.bulbMin()))
            {
                *nextBulbLength = camera.bulbTime((int8_t)camera.bulbMin());
            }
        }
    }
}

void shutter::switchToGuided()
{
    rampRate = 0;
    current.brampMethod = BRAMP_METHOD_GUIDED;
}

void shutter::switchToAuto()
{
    light.integrationStart(conf.lightIntegrationMinutes);
    lightReading = status.lightStart_f = light.readIntegratedEv();
    current.brampMethod = BRAMP_METHOD_AUTO;
    current.nightMode = BRAMP_TARGET_AUTO;
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
  if(conf.cameraPort == AUX_CAM_DOLLY && conf.auxPort == AUX_MODE_SYNC)
  {
    ENABLE_SHUTTER;
    SHUTTER_OPEN;
  }
  else
  {
    ENABLE_AUX_PORT1;
    AUX_OUT1_ON;
  }
}

void aux1_off()
{
  if(conf.cameraPort == AUX_CAM_DOLLY && conf.auxPort == AUX_MODE_SYNC)
  {
    ENABLE_SHUTTER;
    SHUTTER_CLOSE;
  }
  else
  {
    ENABLE_AUX_PORT1;
  }
}

void aux2_on()
{
  if(conf.cameraPort == AUX_CAM_DOLLY && conf.auxPort == AUX_MODE_SYNC)
  {
    ENABLE_MIRROR;
    MIRROR_UP;
  }
  else
  {
    ENABLE_AUX_PORT2;
    AUX_OUT2_ON;
  }
}

void aux2_off()
{
  if(conf.cameraPort == AUX_CAM_DOLLY && conf.auxPort == AUX_MODE_SYNC)
  {
    ENABLE_MIRROR;
    MIRROR_DOWN;
  }
  else
  {
    ENABLE_AUX_PORT2;
  }
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
    if(timer.running && timer.current.IntervalMode == INTERVAL_MODE_KEYFRAME)
    {
      BulbMax = (timer.status.interval_u16 - BRAMP_GAP_PADDING) * 100;
    }
    else
    {
      BulbMax = (timer.current.Gap - BRAMP_GAP_PADDING) * 100;
    }
    BulbMaxEv = 1;
    for(int8_t i = camera.bulbMax(); i < camera.bulbMin(); i++)
    {
        if(BulbMax >= camera.bulbTime(i))
        {
            if(i < 1) i = 1;
            BulbMaxEv = i;
            BulbMax = camera.bulbTime((int8_t)BulbMaxEv);
            break;
        }
    }
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
    int8_t ndRange = (int8_t)(floor((float)timer.current.nightND * 3) / 10.0);

    if(conf.brampMode & BRAMP_MODE_BULB) bulbRange = (int8_t)timer.current.BulbStart - (int8_t)BulbMaxEv;
    if(conf.brampMode & BRAMP_MODE_ISO) isoRange = (int8_t)camera.iso() - (int8_t)camera.isoMax();
    if(conf.brampMode & BRAMP_MODE_APERTURE) apertureRange = (int8_t)camera.aperture() - (int8_t)camera.apertureMin();

    return isoRange + apertureRange + bulbRange + ndRange;
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
        if(conf.extendedRamp && camera.ready)
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

    //DEBUG(PSTR("up: "));
    //DEBUG(up);
    //DEBUG_NL();
    //DEBUG(PSTR("down: "));
    //DEBUG(down);
    //DEBUG_NL();
    //DEBUG(PSTR("max: "));
    //DEBUG(camera.shutterMax());
    //DEBUG_NL();
    //DEBUG(PSTR("min: "));
    //DEBUG(camera.shutterMin());
    //DEBUG_NL();

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

uint8_t tvUp(uint8_t ev)
{
    if(ev == 254) return ev;
    uint8_t tmp = PTP::bulbUp(ev);
    if(tmp > 33) tmp = 254;
    return tmp;
}

uint8_t tvDown(uint8_t ev)
{
    if(ev == 254) return 33;
    return PTP::bulbDown(ev);
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

float interpolateKeyframe(keyframeGroup_t *kf, uint32_t ms)
{
    if(ms >= kf->keyframes[kf->count - 1].ms)
    {
        return kf->keyframes[kf->count - 1].value;
    }

    float key1 = 0, key2 = 0, key3 = 0, key4 = 0;
    uint8_t ki = 0;
    for(uint8_t k = 0; k < kf->count; k++)
    {
        if(kf->keyframes[k].ms >= ms && k > 0)
        {
            ki = k;
            key1 = key2 = key3 = key4 = (float)kf->keyframes[ki].value;
            if(ki > 0)
            {
                ki--;
                key1 = key2 = (float)kf->keyframes[ki].value;
            }
            if(ki > 0)
            {
                ki--;
                key1 = (float)kf->keyframes[ki].value;
            }
            ki = k;
            if(ki < kf->count - 1)
            {
                ki++;
                key4 = (float)kf->keyframes[ki].value;
            }
            ki = k;
            break;
        }
    }
    uint32_t lastKFms = kf->keyframes[ki > 0 ? ki - 1 : 0].ms;
    float t = (float)(ms - lastKFms) / (float)(kf->keyframes[ki].ms - lastKFms);
    return curve(key1, key2, key3, key4, t);  
}

void moveMotor1(int32_t pos)
{
  motor1.enable();
  motor1.moveToPosition((int32_t) pos);
}

void moveMotor2(int32_t pos)
{
  motor2.enable();
  motor2.moveToPosition((int32_t) pos);
}

void moveMotor3(int32_t pos)
{
  motor3.enable();
  motor3.moveToPosition((int32_t) pos);
}

void moveFocus(int32_t pos)
{
    if(focusPos != pos && conf.focusEnabled)
    {
        if(menu.state == ST_KEYFRAME) turnOffLV = 1;
        if(!camera.modeLiveView && conf.camera.cameraMake == CANON)
        {
            camera.liveView(true);
            _delay_ms(500);
        }
        uint8_t move;
        int32_t steps = pos - focusPos;
        if(steps < 0)
        {
            steps = 0 - steps;
//            if(conf.focusRes == 0) move = +1; else move = +2;
            if(1) move = +1; else move = +2;
        }
        else
        {
            if(1) move = -1; else move = -2;
        }
        uint8_t attempts = 0;
        while(camera.moveFocus(move, steps))
        {
            attempts++;
            if(attempts >= 10) break;
            _delay_ms(100);
        }
        if(attempts < 10) focusPos = pos;
    }
}

void moveAxes(uint32_t ms, int32_t pos, uint8_t type, uint8_t force)
{
    if(type != KFT_MOTOR1 && motor1.connected && (conf.motionSetupMove1 || force)) moveMotor1((int32_t)interpolateKeyframe(&timer.current.kfMotor1, ms));
    if(type != KFT_MOTOR2 && motor2.connected && (conf.motionSetupMove2 || force)) moveMotor2((int32_t)interpolateKeyframe(&timer.current.kfMotor2, ms));
    if(type != KFT_MOTOR3 && motor3.connected && (conf.motionSetupMove3 || force)) moveMotor3((int32_t)interpolateKeyframe(&timer.current.kfMotor3, ms));
    if(type == KFT_MOTOR1 && motor1.connected && (conf.motionSetupMove1 || force)) moveMotor1(pos);
    if(type == KFT_MOTOR2 && motor2.connected && (conf.motionSetupMove2 || force)) moveMotor2(pos);
    if(type == KFT_MOTOR3 && motor3.connected && (conf.motionSetupMove3 || force)) moveMotor3(pos);
    if(type != KFT_FOCUS && camera.supports.focus && conf.focusEnabled) moveFocus((int32_t)interpolateKeyframe(&timer.current.kfFocus, ms));
    if(type == KFT_FOCUS && camera.supports.focus && conf.focusEnabled) moveFocus(pos);
}

void updateKeyframeGroup(keyframeGroup_t *kf)
{
    if(kf->type == KFT_MOTOR1 || kf->type == KFT_MOTOR2 || kf->type == KFT_MOTOR3)
    {
        if(kf->keyframes[0].value)
        {
            for(uint8_t i = 1; i < kf->count; i++)
            {
                kf->keyframes[i].value -= kf->keyframes[0].value;
            }
            if(kf->type == KFT_MOTOR1) motor1.currentPos -= kf->keyframes[0].value;
            if(kf->type == KFT_MOTOR2) motor2.currentPos -= kf->keyframes[0].value;
            if(kf->type == KFT_MOTOR3) motor3.currentPos -= kf->keyframes[0].value;
            kf->keyframes[0].value = 0;
        }
        
        kf->max = 1000;
        kf->min = -1000;
        for(uint8_t i = 0; i < kf->count; i++)
        {
            if(kf->keyframes[i].value > kf->max) kf->max = kf->keyframes[i].value;
            if(kf->keyframes[i].value < kf->min) kf->min = kf->keyframes[i].value;
        }
        if(kf->type == KFT_MOTOR1) kf->steps = conf.motionStep1;
        if(kf->type == KFT_MOTOR2) kf->steps = conf.motionStep2;
        if(kf->type == KFT_MOTOR3) kf->steps = conf.motionStep3;
        kf->rangeExtendable = 1;
        kf->move = &moveAxes;
    }
    else if(kf->type == KFT_FOCUS)
    {
        kf->max = 100;
        kf->min = -100;
        for(uint8_t i = 0; i < kf->count; i++)
        {
            if(kf->keyframes[i].value > kf->max) kf->max = kf->keyframes[i].value;
            if(kf->keyframes[i].value < kf->min) kf->min = kf->keyframes[i].value;
        }
        kf->steps = conf.focusStep;
        kf->rangeExtendable = 1;
        kf->move = &moveAxes;
    }
    else if(kf->type == KFT_EXPOSURE)
    {
        kf->max = (int32_t) calcRampMax();
        kf->min = (int32_t) calcRampMin();
        kf->steps = 1;
        kf->rangeExtendable = 0;
        kf->move = NULL;
        kf->keyframes[0].value = 0;
    }
    else if(kf->type == KFT_INTERVAL)
    {
        kf->max = 45;
        kf->min = 2;
        for(uint8_t i = 0; i < kf->count; i++)
        {
            if(kf->keyframes[i].value > kf->max) kf->max = kf->keyframes[i].value;
            if(kf->keyframes[i].value < kf->min) kf->keyframes[i].value = kf->min;
        }
        kf->steps = 1;
        kf->rangeExtendable = 1;
        kf->move = NULL;
    }

    kf->keyframes[0].ms = 0;

    if(!kf->hasEndKeyframe && kf->count >= 2)
    {
        kf->keyframes[kf->count - 1].value = kf->keyframes[kf->count - 2].value;
    }

    if(timer.current.Mode & RAMP)
    {
        kf->keyframes[kf->count - 1].ms = (uint32_t)timer.current.Duration * 60 * 1000;
    }
    else
    {
        kf->keyframes[kf->count - 1].ms = ((uint32_t)timer.current.Photos * (uint32_t)timer.current.Gap) * 100;
    }
    if(kf->keyframes[kf->count - 1].ms < 84) kf->keyframes[kf->count - 1].ms = 84;
}

void shutter::resetKeyframes()
{
  current.kfExposure.count = 2;
  current.kfExposure.selected = 0;
  current.kfExposure.hasEndKeyframe = 0;
  current.kfExposure.type = KFT_EXPOSURE;
  current.kfExposure.max = 0;
  current.kfExposure.min = 0;
  current.kfExposure.keyframes[0].value = 0;
  current.kfExposure.keyframes[1].value = 0;

  current.kfInterval.count = 2;
  current.kfInterval.selected = 0;
  current.kfInterval.hasEndKeyframe = 0;
  current.kfInterval.type = KFT_INTERVAL;
  current.kfInterval.max = 450;
  current.kfInterval.min = 20;
  current.kfInterval.keyframes[0].value = 80;
  current.kfInterval.keyframes[1].value = 80;

  current.kfFocus.count = 2;
  current.kfFocus.selected = 0;
  current.kfFocus.hasEndKeyframe = 0;
  current.kfFocus.type = KFT_FOCUS;
  current.kfFocus.max = 0;
  current.kfFocus.min = 0;
  current.kfFocus.keyframes[0].value = 0;
  current.kfFocus.keyframes[1].value = 0;

  current.kfMotor1.count = 2;
  current.kfMotor1.selected = 0;
  current.kfMotor1.hasEndKeyframe = 0;
  current.kfMotor1.type = KFT_MOTOR1;
  current.kfMotor1.max = 0;
  current.kfMotor1.min = 0;
  current.kfMotor1.keyframes[0].value = 0;
  current.kfMotor1.keyframes[1].value = 0;

  current.kfMotor2.count = 2;
  current.kfMotor2.selected = 0;
  current.kfMotor2.hasEndKeyframe = 0;
  current.kfMotor2.type = KFT_MOTOR2;
  current.kfMotor2.max = 0;
  current.kfMotor2.min = 0;
  current.kfMotor2.keyframes[0].value = 0;
  current.kfMotor2.keyframes[1].value = 0;

  current.kfMotor3.count = 2;
  current.kfMotor3.selected = 0;
  current.kfMotor3.hasEndKeyframe = 0;
  current.kfMotor3.type = KFT_MOTOR3;
  current.kfMotor3.max = 0;
  current.kfMotor3.min = 0;
  current.kfMotor3.keyframes[0].value = 0;
  current.kfMotor3.keyframes[1].value = 0;

  focusPos = 0;
  motor1.currentPos = 0;
  motor2.currentPos = 0;
  motor3.currentPos = 0;
}

void moveToStart()
{
  moveAxes(0, 0, 0, 1);
  menu.cursor = 0;
}
