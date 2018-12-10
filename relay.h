
#ifndef __RELAY_H__
#define __RELAY_H__

#include "config.h"

class Relay {
    public:
        Relay(int pin, const char * id);
        void on();
        void off();
        void toggle();
        int status();
        const char * getId();
        void setStatus(int key);

    protected:
        const char * _id;
        int _pin;
        int _status = -1;
        int _lastUpdate = 0;
        bool _can_update();
};

#ifdef PIN_RELAY_1
extern Relay Relay1;
#endif

#ifdef PIN_RELAY_2
extern Relay Relay2;
#endif

#ifdef PIN_RELAY_3
extern Relay Relay3;
#endif

#ifdef PIN_RELAY_4
extern Relay Relay4;
#endif

#endif
