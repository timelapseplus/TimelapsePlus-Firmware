#define MAX_ITEMS_WATCHED 15

struct watched_item_struct
{
    void *item;
    uint8_t chksum;
    uint8_t chkxsum;
    void *handler;
    uint8_t size;
    uint8_t active;
    uint8_t id;
};



class Notify
{
public:
    Notify(void);
    void task(void);

    void watch(uint8_t id, void * item, uint8_t size, void (handler)(uint8_t));
    void unWatch(uint8_t id); // Unwatch all with specified id
    void unWatch(uint8_t id, void (handler)(uint8_t)); // Unwatch item with specified id & handler
    void unWatch(void (handler)(uint8_t)); // Unwatch all with specified handler


private:
	watched_item_struct watchedItems[MAX_ITEMS_WATCHED];
};

