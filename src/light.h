#define I2C_ADDR_READ   0b10001001
#define I2C_ADDR_WRITE  0b10001000

#define LIGHT_INTEGRATION_COUNT 32
#define FILTER_LENGTH 3

#define ANALOG_THRESHOLD 18
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
	void integrationStart(uint8_t integration_minutes, int8_t darkOffset);
	float readLux();

	uint8_t method;

private:
    float iev[LIGHT_INTEGRATION_COUNT];
    float filter[FILTER_LENGTH];
    int8_t filterIndex;
    int8_t pos;
    uint8_t integration;
    uint16_t lastSeconds;
    uint8_t initialized;
    uint16_t offset;
};