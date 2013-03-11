#define I2C_ADDR_READ   0b10001001
#define I2C_ADDR_WRITE  0b10001000

#define LIGHT_INTEGRATION_COUNT 64


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
	int8_t readEv();
	float readIntegratedEv();
	float readIntegratedSlope();
	void integrationStart(uint8_t integration_minutes, int8_t darkOffset);
	float readLux();

	uint8_t method;

private:
    int8_t iev[LIGHT_INTEGRATION_COUNT];
    int8_t pos;
    uint8_t integration;
    uint16_t lastSeconds;
};