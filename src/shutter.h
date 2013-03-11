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

#define SHUTTER_PRESS_TIME (conf.cameraFPS * 4)

#define DONE 0
#define CONTINUE 1

#define TIMELAPSE 0x0001
#define HDR 0x0002
#define RAMP 0x0004

#define AUX_MODE_DISABLED 0
#define AUX_MODE_DOLLY 1

#define MODE_TIMELAPSE (TIMELAPSE)
#define MODE_HDR_TIMELAPSE (HDR | TIMELAPSE)
#define MODE_BULB_PHOTO 0
#define MODE_HDR_PHOTO (HDR)
#define MODE_BULB_RAMP (RAMP | TIMELAPSE)
#define MODE_HDR_RAMP (RAMP | HDR | TIMELAPSE)

#define BRAMP_METHOD_KEYFRAME 0
#define BRAMP_METHOD_GUIDED 1
#define BRAMP_METHOD_AUTO 2

#define BRAMP_INTERVAL_MIN 70

#define BRAMP_TARGET_AUTO 0
#define BRAMP_TARGET_STARS 9
#define BRAMP_TARGET_HALFMOON 3
#define BRAMP_TARGET_FULLMOON 1

#define MAX_KEYFRAMES 5

#define DISABLE_SHUTTER setIn(SHUTTER_FULL_PIN);  _delay_ms(50); setLow(SHUTTER_FULL_PIN)
#define DISABLE_MIRROR setIn(SHUTTER_HALF_PIN); _delay_ms(50); setLow(SHUTTER_HALF_PIN)
#define ENABLE_SHUTTER setHigh(SHUTTER_FULL_PIN); setOut(SHUTTER_FULL_PIN); setIn(CHECK_CABLE_PIN); setHigh(CHECK_CABLE_PIN); setIn(SHUTTER_SENSE_PIN); setHigh(SHUTTER_SENSE_PIN);
#define ENABLE_MIRROR setHigh(SHUTTER_HALF_PIN); setOut(SHUTTER_HALF_PIN)

#define ENABLE_AUX_PORT setOut(AUX_OUT1_PIN); setHigh(AUX_OUT1_PIN); setOut(AUX_OUT2_PIN); setHigh(AUX_OUT2_PIN); setIn(AUX_INPUT1_PIN); setHigh(AUX_INPUT1_PIN); setIn(AUX_INPUT2_PIN); setHigh(AUX_INPUT2_PIN)
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

#define SHUTTER_VERSION 20130302

struct program
{
    char Name[12];            // 12 bytes
    unsigned int Mode;        // 2 bytes
    unsigned int Delay;       // 2 bytes
    unsigned int Duration;    // 2 bytes
    unsigned int Photos;      // 2 bytes
    unsigned int Gap;         // 2 bytes
    unsigned int Exps;        // 2 bytes
    unsigned int Exp;         // 2 bytes
    unsigned int Bracket;     // 2 bytes
    unsigned int BulbStart;   // 2 bytes
    unsigned int Bulb[10];    // 20 bytes
    unsigned int Key[10];     // 20 bytes
    unsigned int Keyframes;   // 2 bytes
    unsigned int brampMethod; // 2 bytes
    unsigned int Integration; // 2 bytes
    unsigned int NightSky; // 2 bytes
    uint8_t infinitePhotos;   // 1 byte  
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
};

extern program stored[MAX_STORED]EEMEM;

class shutter
{
public:
    shutter();
    void begin(void);
    char task(void);
    void save(char id);
    void load(char id);
    void setDefault(void);
    int8_t nextId(void);

    void off(void);
    void half(void);
    void full(void);

    void bulbEnd(void);
    void bulbStart(void);
    void capture(void);

    char cableIsConnected(void);

    program current;
    timer_status status; 
    volatile char running;  // 0 = not running, 1 = running (shutter closed), 2 = running (shutter open)
    volatile int8_t currentId;
    volatile uint16_t length; // in Seconds
    volatile int8_t rampRate;
    volatile uint32_t last_photo_ms;
    volatile float lightReading;
    volatile float lightStart;
    volatile float internalRampStops;

private:
    double test;
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
void aux_on(void);
void aux_off(void);
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
int8_t calcRampMin();
uint8_t rampTvUp(uint8_t ev);
uint8_t rampTvDown(uint8_t ev);
#endif

