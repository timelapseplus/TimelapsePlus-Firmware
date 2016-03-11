#define NMX_COMMAND_SPACING_MS 100

class NMX
{
public:
    NMX(uint8_t node, uint8_t motor);

    uint8_t enable();
    uint8_t disable();

    uint8_t running();

    uint8_t gotoPercent(float pos);

    uint8_t moveSteps(uint8_t dir, uint32_t steps);
    uint8_t moveToPosition(int32_t pos);

    uint8_t checkConnected();

    uint8_t setSpeed(float rate);
    uint8_t setAccel(float rate);
    uint8_t move(uint8_t dir, uint32_t steps, uint8_t update);
    uint8_t moveConstant(uint8_t dir);
    uint8_t stop();

    int32_t currentPos;
    uint8_t connected;
    bool enabled;

private:
    static uint8_t sendCommandGeneral(uint8_t node, uint8_t motor, uint8_t command, uint8_t dataLength, uint8_t *data);
    uint8_t sendCommand(uint8_t command, uint8_t dataLength, uint8_t *data);
    static uint32_t sendQueryGeneral(uint8_t node, uint8_t motor, uint8_t command, uint8_t delay);
    uint32_t sendQuery(uint8_t command);
    uint8_t nodeAddress;
    uint8_t motorAddress;

    static uint8_t genieMoveConstant(int8_t speed);
};
