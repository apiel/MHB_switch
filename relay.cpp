#include <espressif/esp_common.h>
#include <esp8266.h>

#include "config.h"
#include "relay.h"

Relay::Relay(int pin, const char * id)
{
    _id = id;
    _pin = pin;
    off();
}

const char * Relay::getId()
{
    return _id;
}

bool Relay::_can_update()
{
    if (sdk_system_get_time() < 1000000 // let's allow to switch the relay at init of mcu
     || sdk_system_get_time() - _lastUpdate > 1000000) { // 1 sec
        _lastUpdate = sdk_system_get_time();
        return true;
    }
    return false;
}

void Relay::on()
{
    if (_status != RELAY_ON && _can_update()) {
        printf("relay on\n");
        gpio_enable(_pin, GPIO_OUTPUT);
        gpio_write(_pin, RELAY_ON);
        _status = RELAY_ON; // here we should use a setState
    }
}

void Relay::off()
{
    if (_status != RELAY_OFF && _can_update()) {
        printf("relay off\n");
        gpio_enable(_pin, GPIO_OUTPUT);
        gpio_write(_pin, RELAY_OFF);
        _status = RELAY_OFF; // here we should use a setState
    }
}

void Relay::toggle()
{
    if (_status != RELAY_ON) {
        on();
    } else {
        off();
    }
}

int Relay::status()
{
    printf("-> relay status %d\n", _status);
    return _status;
}

void Relay::setStatus(int key)
{
    if (key == RELAY_ON) {
        on();
    } else if (key == RELAY_OFF) {
        off();
    } else {
        toggle();
    }
}

#ifdef PIN_RELAY_1
Relay Relay1 = Relay(PIN_RELAY_1, "relay/1");
#endif

#ifdef PIN_RELAY_2
Relay Relay2 = Relay(PIN_RELAY_2, "relay/2");
#endif

#ifdef PIN_RELAY_3
Relay Relay3 = Relay(PIN_RELAY_3, "relay/3");
#endif

#ifdef PIN_RELAY_4
Relay Relay4 = Relay(PIN_RELAY_4, "relay/4");
#endif
