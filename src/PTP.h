


struct propertyDescription_t
{
    char name[7];
    uint16_t id;
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
    uint8_t bulbStart(void);
    uint8_t bulbEnd(void);
    uint8_t setParameter(uint16_t param, uint16_t value);
    uint8_t setISO(uint16_t value);
    uint8_t setShutter(uint16_t value);
    uint8_t setAperture(uint16_t value);
    
    uint8_t isoName(char name[7], uint16_t id);
    uint8_t apertureName(char name[7], uint16_t id);
    uint8_t shutterName(char name[7], uint16_t id);

    uint8_t isoUp(uint16_t id);
    uint8_t isoDown(uint16_t id);

    uint8_t ready;

    uint16_t iso;
    uint16_t shutter;
    uint16_t aperture;

    CameraSupports_t supports;

private:
    uint32_t data[3];

};

