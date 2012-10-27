#define REMOTE_STATUS 1
#define REMOTE_PROGRAM 2
#define REMOTE_START 3
#define REMOTE_STOP 4
#define REMOTE_BATTERY 5
#define REMOTE_BULB_START 6
#define REMOTE_BULB_END 7
#define REMOTE_CAPTURE 8

#define REMOTE_TYPE_SEND 0
#define REMOTE_TYPE_REQUEST 1
#define REMOTE_TYPE_SET 2
#define REMOTE_TYPE_NOTIFY_SET 3
#define REMOTE_TYPE_NOTIFY_UNSET 4

class Remote
{
public:
    Remote(void);
    uint8_t request(uint8_t id);
    uint8_t set(uint8_t id);
    uint8_t send(uint8_t id, uint8_t type);
    void event(void);

    uint8_t connected;
    uint8_t running;
    uint8_t battery;

    uint8_t notifyBattery;

    timer_status status;
    program current;

private:
	volatile uint8_t requestActive;
};
