/*
 *  shutter.h
 *  AVR_Shutter
 *
 *  Created by Elijah Parker on 10/25/10.
 *
 */
#ifndef shutter_h
#define shutter_h

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <avr/version.h>
#include <avr/eeprom.h>
#include <string.h>


#define MAX_STORED 4

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
#define INTERVAL_MODE_EXTERNAL 3

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

#define CHECK_EXTERNAL (getPin(CHECK_CABLE_PIN) || !getPin(SHUTTER_SENSE_PIN))
#define CHECK_CABLE if(getPin(CHECK_CABLE_PIN) && (conf.cameraPort == AUX_CAM_DEFAULT || conf.auxPort != AUX_MODE_SYNC)) cable_connected = 1; else cable_connected = 0

#define SHUTTER_VERSION 20141122

#define CHECK_ALERT(string, test) if(test) { if(status.preChecked_u8 == 0) menu.alert(string); } else { if(status.preChecked_u8 == 1) menu.clearAlert(string); }

struct keyframe_t {
    int32_t value;
    uint32_t ms;
};

struct keyframeGroup_t {
    keyframe_t keyframes[MAX_KEYFRAMES];
    uint8_t count;
    uint8_t selected;
    uint8_t hasEndKeyframe;
    uint8_t type;
    int32_t max;
    int32_t min;
    int32_t steps;
    uint8_t rangeExtendable;    
    void (*move)(uint32_t ms, int32_t pos, uint8_t type, uint8_t force);
};

#define KFT_EXPOSURE 0
#define KFT_MOTOR1 1
#define KFT_MOTOR2 2
#define KFT_MOTOR3 3
#define KFT_FOCUS 4
#define KFT_INTERVAL 5

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
    keyframeGroup_t kfMotor1;   
    keyframeGroup_t kfMotor2;   
    keyframeGroup_t kfMotor3;   
    keyframeGroup_t kfExposure;   
    keyframeGroup_t kfFocus;   
    keyframeGroup_t kfInterval;   
    uint16_t brampMethod; 
    uint8_t  nightMode;
    uint8_t  nightISO;
    uint8_t  nightAperture;
    uint8_t  nightShutter;
    uint16_t nightND;
    uint8_t  infinitePhotos;
};

struct timer_status
{
    char textStatus[12];
    uint8_t mode_u8;
    uint16_t photosTaken_u16;
    uint16_t photosRemaining_u16;
    uint16_t nextPhoto_u16;
    uint8_t infinitePhotos_u8;
    float rampStops_f;
    uint16_t bulbLength_u16;
    int8_t rampMax_i8;
    int8_t rampMin_i8;
    uint16_t interval_u16;
    float rampTarget_f;
    int8_t nightTarget_i8;
    uint8_t preChecked_u8;
    float lightStart_f;
    uint8_t showND_u8;
    uint8_t hasND_u8;
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

    void switchToGuided(void);
    void switchToAuto(void);

    void resetKeyframes(void);

    program current;
    timer_status status; 
    volatile char running;  // 0 = not running, 1 = running (shutter closed), 2 = running (shutter open)
    int8_t currentId;
    uint16_t length; // in Seconds
    int8_t rampRate, apertureEvShift;
    uint32_t last_photo_ms;
    float lightReading;
	//This variable is the precise median/average EV value for the darkest part of the night 
	float nightLight;																		//J.R. 11-30-15
	//This flag maintains night2day bulb ramping once the light reaches a certain point		
	bool Night2Day;																			//J.R. 10-11-15
    float pastErrors[PAST_ERROR_COUNT];
    volatile uint8_t paused, pausing, apertureReady;
    int8_t evShift;
    float ndShift;

private:
    double test;
    uint8_t iso;
    uint8_t aperture;
    uint8_t movedFocus;
    uint8_t switchBack;
};

void check_cable();
void shutter_off(void);
void shutter_off_quick(void);
void shutter_half(void);
void shutter_half_delayed(void);
void shutter_full(void);
void shutter_bulbEnd(void);
void shutter_bulbEndQuick(void);
void shutter_bulbEndFinish(void);
void shutter_bulbStart(void);
void shutter_bulbStartQuick(void);
void shutter_bulbStartPrepare(void);
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
float interpolateKeyframe(keyframeGroup_t *kf, uint32_t ms);
void updateKeyframeGroup(keyframeGroup_t *kf);
void moveFocus(int32_t pos);
void moveMotor1(int32_t pos);
void moveMotor2(int32_t pos);
void moveMotor3(int32_t pos);
void moveAxes(uint32_t seconds, int32_t pos, uint8_t type, uint8_t force);
void moveToStart(void);

extern uint8_t lastShutterError;
#endif

