#define BT_NAME 
#define BT_DATA_SIZE 64

#define BT_INIT_IO setIn(BT_RTS_PIN); setHigh(BT_RTS_PIN); setHigh(BT_CTS_PIN); setOut(BT_CTS_PIN)
#define BT_SET_CTS setLow(BT_CTS_PIN); _delay_us(10)
#define BT_CLR_CTS setHigh(BT_CTS_PIN)
#define BT_RTS !(getPin(BT_RTS_PIN))


class BT
{
  public:
    BT(void);
    uint8_t init(void);
    uint8_t sleep(void);
    uint8_t read(void);
    uint8_t send(char *data);
    uint8_t power(uint8_t level);
    uint8_t power(void);
    uint8_t temperature(void);
    uint8_t version(void);
    uint8_t cancel(void);

    uint8_t present;

    char data[BT_DATA_SIZE];

  private:
    uint8_t checkOK(void);
    uint8_t btPower;
};