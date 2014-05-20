#define I2C_ADDR_READ   0b10001001
#define I2C_ADDR_WRITE  0b10001000

#define LIGHT_INTEGRATION_COUNT 32
#define FILTER_LENGTH 3

#define NIGHT_THRESHOLD 15
#define NIGHT_THRESHOLD_HYSTERESIS 5
#define OFFSET_UNSET 65535

class Light
{
public:
    Light();
    void task();
	uint16_t readRaw();
	void startIR();
	void start();
	void stop();
	void setRange(uint8_t range);
	void setRangeAuto();
	float readEv();
	float readIntegratedEv();
	float readIntegratedSlope();
	float readIntegratedSlopeMedian();
	void integrationStart(uint8_t integration_minutes);
	float readLux();

    float lockedSlope, slope, integrated, median;
	uint8_t method, paused, skipTask;
	bool underThreshold;

private:
    float iev[LIGHT_INTEGRATION_COUNT];
    float filter[FILTER_LENGTH];
    int8_t filterIndex;
    int8_t wasPaused;
    uint16_t integration;
    uint32_t lastSeconds;
    uint8_t initialized;
    uint16_t offset;
    bool integrationActive;

};