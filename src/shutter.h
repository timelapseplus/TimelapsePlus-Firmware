/*
 *  shutter.h
 *  AVR_Shutter
 *
 *  Created by Elijah Parker on 10/25/10.
 *  Copyright 2010 __MyCompanyName__. All rights reserved.
 *
 */
 
#ifndef shutter_h
#define shutter_h

#define MAX_STORED 25

#define SHUTTER_PRESS_TIME (conf.camera.cameraFPS * 4)

#define DONE 0
#define CONTINUE 1

#define TIMELAPSE 0x0001
#define HDR 0x0002
#define RAMP 0x0004

#define MODE_TIMELAPSE (TIMELAPSE)
#define MODE_HDR_TIMELAPSE (HDR | TIMELAPSE)
#define MODE_BULB_PHOTO 0
#define MODE_HDR_PHOTO (HDR)
#define MODE_BULB_RAMP (RAMP | TIMELAPSE)
#define MODE_HDR_RAMP (RAMP | HDR | TIMELAPSE)

#define BRAMP_METHOD_KEYFRAME 0
#define BRAMP_METHOD_GUIDED 1
#define BRAMP_METHOD_AUTO 2

// in 1/10 seconds
#define BRAMP_GAP_PADDING (conf.camera.brampGap * 10)
#define BRAMP_INTERVAL_MIN (BRAMP_GAP_PADDING + 20)
#define BRAMP_INTERVAL_VAR_MIN 20

#define P_FACTOR (((float)conf.pFactor)/10.0)
#define I_FACTOR (((float)conf.iFactor)/10.0)
#define D_FACTOR (((float)conf.dFactor)/10.0)
#define PAST_ERROR_COUNT 10

#define BRAMP_TARGET_CUSTOM 255
#define BRAMP_TARGET_AUTO 254

#define BRAMP_TARGET_OFFSET 100
 

                    //1/25 2.8 100 = 30/3
                    // 30s 2.8 1600 12*3=36
                    // 30s 2.8 160 25

#define MAX_EXTENDED_RAMP_SHUTTER 69

#define INTERVAL_MODE_FIXED 0
#define INTERVAL_MODE_AUTO 1
#define INTERVAL_MODE_KEYFRAME 2

#define MAX_KEYFRAMES 10

#define DISABLE_SHUTTER setIn(SHUTTER_FULL_PIN);  _delay_ms(50); setLow(SHUTTER_FULL_PIN)
#define DISABLE_MIRROR setIn(SHUTTER_HALF_PIN); _delay_ms(50); setLow(SHUTTER_HALF_PIN)
#define ENABLE_SHUTTER setHigh(SHUTTER_FULL_PIN); setOut(SHUTTER_FULL_PIN); setIn(CHECK_CABLE_PIN); setHigh(CHECK_CABLE_PIN); setIn(SHUTTER_SENSE_PIN); setHigh(SHUTTER_SENSE_PIN);
#define ENABLE_MIRROR setHigh(SHUTTER_HALF_PIN); setOut(SHUTTER_HALF_PIN)

#define ENABLE_AUX_PORT1 setOut(AUX_OUT1_PIN); setHigh(AUX_OUT1_PIN); setIn(AUX_INPUT1_PIN); setHigh(AUX_INPUT1_PIN)
#define ENABLE_AUX_PORT2 setOut(AUX_OUT2_PIN); setHigh(AUX_OUT2_PIN); setIn(AUX_INPUT2_PIN); setHigh(AUX_INPUT2_PIN)
#define AUX_INPUT1 (getPin(AUX_INPUT1_PIN) == 0)
#define AUX_INPUT2 (getPin(AUX_INPUT2_PIN) == 0)
#define AUX_OUT1_ON (setLow(AUX_OUT1_PIN))
#define AUX_OUT2_ON (setLow(AUX_OUT2_PIN))
#define AUX_OUT1_OFF (setHigh(AUX_OUT1_PIN))
#define AUX_OUT2_OFF (setHigh(AUX_OUT2_PIN))


#define MIRROR_UP setLow(SHUTTER_HALF_PIN)
#define MIRROR_DOWN setHigh(SHUTTER_HALF_PIN)
#define SHUTTER_OPEN setLow(SHUTTER_FULL_PIN)
#define SHUTTER_CLOSE setHigh(SHUTTER_FULL_PIN); MIRROR_DOWN

#define MIRROR_IS_DOWN (isHigh(SHUTTER_HALF_PIN))

//#define CHECK_CABLE if(getPin(CHECK_CABLE_PIN) || getPin(SHUTTER_SENSE_PIN)) cable_connected = 1; else cable_connected = 0
#define CHECK_CABLE if(getPin(CHECK_CABLE_PIN)) cable_connected = 1; else cable_connected = 0

#define SHUTTER_VERSION 20131122

#define CHECK_ALERT(string, test) if(test) { if(status.preChecked == 0) menu.alert(string); } else { if(status.preChecked == 1) menu.clearAlert(string); }

struct program
{
    char Name[12];        
    uint16_t Mode;        
    uint16_t Delay;       
    uint16_t Duration;    
    uint16_t Photos;      
    uint16_t IntervalMode;
    uint16_t Gap;         
    uint16_t GapMin;      
    uint16_t Exps;        
    uint16_t Exp;         
    uint16_t ArbExp;      
    uint16_t Bracket;     
    uint16_t BulbStart;   
    uint16_t Bulb[10];    
    uint16_t Key[10];     
    uint16_t Keyframes;   
    uint16_t brampMethod; 
    uint8_t  nightMode;
    uint8_t  nightISO;
    uint8_t  nightAperture;
    uint8_t  nightShutter;
    uint8_t  infinitePhotos;
    uint8_t pad[16];
};

struct timer_status
{
    char textStatus[12];
    uint8_t mode;
    unsigned int photosTaken;
    unsigned int photosRemaining;
    unsigned int nextPhoto;
    uint8_t infinitePhotos;
    float rampStops;
    uint16_t bulbLength;
    int8_t rampMax;
    int8_t rampMin;
    unsigned int interval;
    float rampTarget;
    int8_t nightTarget;
    uint8_t preChecked;
    float lightStart;
};

extern program stored[MAX_STORED+1]EEMEM;

class shutter
{
public:
    shutter();
    void begin(void);
    void pause(uint8_t p);
    char task(void);
    void save(char id);
    void load(char id);
    void setDefault(void);
    int8_t nextId(void);
    void calculateExposure(uint32_t *nextBulbLength, uint8_t *nextAperture, uint8_t *nextISO, int8_t *bulbChangeEv);

    void saveCurrent(void);
    void restoreCurrent(void);

    void off(void);
    void half(void);
    void full(void);

    void bulbEnd(void);
    void bulbStart(void);
    void capture(void);

    char cableIsConnected(void);

    void switchToGuided();
    void switchToAuto();

    program current;
    timer_status status; 
    volatile char running;  // 0 = not running, 1 = running (shutter closed), 2 = running (shutter open)
    int8_t currentId;
    uint16_t length; // in Seconds
    int8_t rampRate, apertureEvShift;
    uint32_t last_photo_ms;
    float lightReading;
    float internalRampStops;
    float pastErrors[PAST_ERROR_COUNT];
    volatile uint8_t paused, pausing, apertureReady;
    int8_t evShift;

private:
    double test;
    uint8_t iso;
    uint8_t aperture;
};

void check_cable();
void shutter_off(void);
void shutter_off_quick(void);
void shutter_half(void);
void shutter_half_delayed(void);
void shutter_full(void);
void shutter_bulbEnd(void);
void shutter_bulbStart(void);
void shutter_capture(void);
void aux1_on(void);
void aux1_off(void);
void aux2_on(void);
void aux2_off(void);
void aux_pulse(void);
uint8_t stopName(char name[7], uint8_t stop);
uint8_t stopUp(uint8_t stop);
uint8_t stopDown(uint8_t stop);
uint8_t checkHDR(uint8_t exps, uint8_t mid, uint8_t bracket);
uint8_t hdrTvUp(uint8_t ev);
uint8_t hdrTvDown(uint8_t ev);
uint8_t bracketUp(uint8_t ev);
uint8_t bracketDown(uint8_t ev);
uint8_t hdrExpsUp(uint8_t hdr_exps);
uint8_t hdrExpsDown(uint8_t hdr_exps);
uint8_t hdrExpsName(char name[8], uint8_t hdr_exps);
void calcBulbMax(void);
int8_t calcRampMax();
int8_t calcRampTarget(int8_t targetShutter, int8_t targetISO, int8_t targetAperture);
int8_t calcRampMin();
uint8_t tvUp(uint8_t ev);
uint8_t tvDown(uint8_t ev);
uint8_t rampTvUp(uint8_t ev);
uint8_t rampTvUpStatic(uint8_t ev);
uint8_t rampTvUpExtended(uint8_t ev);
uint8_t rampTvDown(uint8_t ev);
uint8_t rampTvDownExtended(uint8_t ev);
void calcInterval();

extern uint8_t lastShutterError;
#endif

