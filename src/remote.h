#define REMOTE_VERSION 20130509

#define REMOTE_STATUS 1
#define REMOTE_PROGRAM 2
#define REMOTE_START 3
#define REMOTE_STOP 4
#define REMOTE_BATTERY 5
#define REMOTE_BULB_START 6
#define REMOTE_BULB_END 7
#define REMOTE_CAPTURE 8

#define REMOTE_MODEL 9
#define REMOTE_EDITION 10 // Reserved for future use
#define REMOTE_FIRMWARE 11
#define REMOTE_BT_FW_VERSION 12
#define REMOTE_PROTOCOL_VERSION 13
#define REMOTE_CAMERA_FPS 14
#define REMOTE_CAMERA_MAKE 15

#define REMOTE_DEBUG 16

#define REMOTE_ISO 17
#define REMOTE_APERTURE 18
#define REMOTE_SHUTTER 19

#define REMOTE_THUMBNAIL 20
#define REMOTE_THUMBNAIL_SIZE 21
// Note: REMOTE_THUMBNAIL_SIZE cannot be requested -- it only get sent before sending REMOTE_THUMBNAIL

#define REMOTE_TYPE_SEND 0
#define REMOTE_TYPE_REQUEST 1
#define REMOTE_TYPE_SET 2
#define REMOTE_TYPE_NOTIFY_WATCH 3
#define REMOTE_TYPE_NOTIFY_UNWATCH 4

#define REMOTE_MODEL_TLP 1
#define REMOTE_MODEL_IPHONE 2

class Remote
{
public:
    Remote(void);
    uint8_t request(uint8_t id);
    uint8_t set(uint8_t id);
    uint8_t debug(char *str);
    uint8_t watch(uint8_t id);
    uint8_t unWatch(uint8_t id);
    static uint8_t send(uint8_t id, uint8_t type);
    void event(void);

    uint8_t connected;
    uint8_t running;
    uint8_t battery;

    timer_status status;
    program current;

    uint8_t model;

private:
	volatile uint8_t requestActive;
};

void remote_notify(uint8_t id);
