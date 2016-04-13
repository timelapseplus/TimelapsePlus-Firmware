#define I2C_ADDR_READ   0b10001001
#define I2C_ADDR_WRITE  0b10001000

#define LIGHT_INTEGRATION_COUNT 32

//nightEv() has same array size as iev(LIGHT_INTEGRATION_COUNT) 
#define NIGHT_INTEGRATION_COUNT 32								//J.R. 9-30-15
//338 is the number of seconds between updates to nightEv()
//so the 32 readings in nightEv(NIGHT_INTEGRATION_COUNT) will span the previous 3 hours
#define NIGHT_COUNT_DELAY 338									//J.R. 9-30-15

#define FILTER_LENGTH 3
//This definition isn't needed because it is a settable parameter
//define NIGHT_THRESHOLD 20										//J.R. 10-11-15
#define NIGHT_THRESHOLD_HYSTERESIS 5
#define OFFSET_UNSET 65535

class Light
{
public:
    Light();
    void task();
	uint16_t readRaw();
	//void startIR();
	void start();
	void stop();
	void setRange(uint8_t range);
	void setRangeAuto();
	float readEv();
	float readIntegratedEv();
	//This is the function that determines the best EV value for middle-of-the-night darkness.
	float readNightEv();													//J.R. 9-30-15
	float readIntegratedSlope();
	float readIntegratedSlopeMedian();
	void integrationStart(uint8_t integration_minutes);
	float readLux();

    float lockedSlope, slope, integrated, median;
	uint8_t method, paused, skipTask, scale;
	bool underThreshold;

private:
    float iev[LIGHT_INTEGRATION_COUNT];
    //This is an array that contains 32 light readings spanned over the last 3 hours
	float nightEv[NIGHT_INTEGRATION_COUNT];									//J.R. 9-30-15
    float filter[FILTER_LENGTH];
    int8_t filterIndex;
    int8_t wasPaused;
    uint16_t integration;
    uint32_t lastSeconds;
    //This is the seconds timer value that decides when to put a new value in nightEv().
	//This variable is similar to "lastSeconds" for updating iev().									
	uint32_t nightSeconds;														//J.R. 9-30-15
    uint8_t initialized;
    uint16_t offset;
    bool integrationActive;

};
