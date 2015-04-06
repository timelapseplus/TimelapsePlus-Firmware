/*
 *  bluetooth.h
 *  Timelapse+
 *
 *  Created by Elijah Parker
 *  Copyright 2012 Timelapse+
 *  Licensed under GPLv3
 *
 */

#define BT_NAME_LEN 13
#define BT_BUF_SIZE 128
#define BT_MAX_SCAN 5
#define BT_ADDR_LEN 13
#define BT_NAME_LEN 13

#define BT_INIT_IO setIn(BT_RTS_PIN); setHigh(BT_RTS_PIN); setHigh(BT_CTS_PIN); setOut(BT_CTS_PIN)
#define BT_SET_CTS setLow(BT_CTS_PIN); _delay_us(10)
#define BT_CLR_CTS setHigh(BT_CTS_PIN)
#define BT_RTS !(getPin(BT_RTS_PIN))

#define BT_ST_SLEEP 0
#define BT_ST_IDLE 1
#define BT_ST_SCAN 2
#define BT_ST_CONNECTED 3
#define BT_ST_CONNECTED_NMX 4

#define BT_MODE_CMD 0
#define BT_MODE_DATA 1

#define BT_EVENT_NULL 0
#define BT_EVENT_DISCOVERY 1
#define BT_EVENT_CONNECT 2
#define BT_EVENT_DISCONNECT 3
#define BT_EVENT_DATA 4
#define BT_EVENT_SCAN_COMPLETE 5


struct discovery
{
    char name[BT_NAME_LEN + 1];
    char addr[BT_ADDR_LEN + 1];
};

class BT
{
public:
    BT(void);
    uint8_t init(void);
    uint8_t sleep(void);
    uint8_t read(void);
    uint8_t send(char *str);
    uint8_t sendP(const char *str);
    uint8_t sendByte(char byte);
    uint8_t sendCMD(char *str);
    uint8_t sendCMD(const char *str);
    uint8_t sendDATA(uint8_t id, uint8_t type, void* buffer, uint16_t bytes);
    uint8_t waitRTS(void);
    uint8_t power(uint8_t level);
    uint8_t power(void);
    uint8_t temperature(void);
    uint8_t version(void);
    uint8_t updateVersion(void);
    uint8_t cancel(void);
    uint8_t cancelScan(void);
    uint8_t scan(void);
    uint8_t advertise(void);
    uint8_t connect(char *address);
    uint8_t disconnect(void);
    uint8_t task(void);

    uint8_t checkOK(void);
    uint8_t waitEvent(char *str, char **retbuf);

    uint8_t present;
    uint8_t state;
    uint8_t mode;
    uint8_t event;

    uint8_t btVersion;

    char address[14];
    char * data;
    uint8_t dataId;
    uint8_t dataType;
    uint16_t dataSize;

    discovery device[BT_MAX_SCAN];
    uint8_t devices;


private:
    char buf[BT_BUF_SIZE];
    uint8_t wake(void);
    uint8_t btPower;
    uint8_t dataMode(void);
    uint8_t cmdMode(void);
    uint8_t newDevices;
    uint8_t waitEventStatus;
    char *waitEventString;
};
