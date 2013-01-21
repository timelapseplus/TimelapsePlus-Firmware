
#define SHUTTER_MODE_BULB 1
#define SHUTTER_MODE_PTP 2
#define SHUTTER_MODE_EITHER SHUTTER_MODE_PTP | SHUTTER_MODE_BULB

#define CUBE_ROOT_OF_2 1.25992104989
#define THIRTIETH_ROOT_OF_2 1.023373892
#define THREE_HUNDREDTH_ROOT_OF_2 1.00231316184


struct propertyDescription_t
{
    char name[8];
    uint8_t eos;
    uint8_t ev; // 1/3 stop increments
};

struct bulbSettings_t
{
    char name[8];
    uint32_t ms;
    uint8_t ev;
};


struct CameraSupports_t
{
    bool capture;
    bool bulb;
    bool iso;
    bool shutter;
    bool aperture;
};

class PTP
{
public:
    PTP(void);

    uint8_t init(void);
    uint8_t checkEvent(void);
    uint8_t close(void);
    uint8_t capture(void);
    uint8_t checkCaptureDone(void);
    uint8_t bulbStart(void);
    uint8_t bulbEnd(void);
    uint8_t setParameter(uint16_t param, uint8_t value);
    
    uint8_t iso(void);
    uint8_t shutter(void);
    uint8_t aperture(void);

    uint8_t setISO(uint8_t value);
    uint8_t setShutter(uint8_t value);
    uint8_t setAperture(uint8_t value);

    uint32_t bulbTime(int8_t ev);
    uint32_t bulbTime(float ev);
    uint8_t bulbMode(void);
    uint32_t shiftBulb(uint32_t ms, int8_t ev);

    static uint8_t isoName(char name[8], uint8_t ev);
    static uint8_t apertureName(char name[8], uint8_t ev);
    static uint8_t shutterName(char name[8], uint8_t ev);
    static uint8_t shutterType(uint8_t ev);

    static uint8_t bulbMax(void);
    static uint8_t bulbMin(void);
    static uint8_t isoMax(void);
    static uint8_t isoMin(void);
    static uint8_t shutterMax(void);
    static uint8_t shutterMin(void);
    static uint8_t isoUp(uint8_t ev);
    static uint8_t isoDown(uint8_t ev);
    static uint8_t shutterUp(uint8_t ev);
    static uint8_t shutterDown(uint8_t ev);
    static uint8_t apertureUp(uint8_t ev);
    static uint8_t apertureDown(uint8_t ev);
    static uint8_t bulbUp(uint8_t ev);
    static uint8_t bulbDown(uint8_t ev);

    uint8_t ready, busy, bulb;

    CameraSupports_t supports;

private:
    uint32_t data[3];

    static uint8_t isoEv(uint8_t id);
    static uint8_t shutterEv(uint8_t id);
    static uint8_t apertureEv(uint8_t id);
    static uint8_t isoEvPTP(uint8_t ev);
    static uint8_t shutterEvPTP(uint8_t ev);
    static uint8_t apertureEvPTP(uint8_t ev);

};

void sendHex(char *hex);
void sendByte(char byte);

