
#ifndef __CONFIG_H__
#define __CONFIG_H__

#define DEVICE_ID "MHB_SWITCH_"
#define DEVICE_NAME "wemo light"

#define WEMOS
#ifdef WEMOS
    #define PIN_BUTTON 0 // D3
    #define PIN_LED 2
    #define PIN_RELAY_1 5 // D1
#else
    #define PIN_BUTTON 0 // D3
    #define PIN_LED 13
    #define PIN_RELAY_1 12
#endif

// https://community.blynk.cc/uploads/default/original/2X/4/4f9e2245bf4f6698e10530b9060595c893bf49a2.png
// D0 > GPIO 16
// D1 > GPIO 5
// D2 > GPIO 4 // WEMOS MINI LED
// D3 > GPIO 0 // when emitter connected bug on boot
// D4 > GPIO 2 // when emitter connected bug on boot
// D5 > GPIO 14
// D6 > GPIO 12
// D7 > GPIO 13
// D8 > GPIO 15

#define RELAY_ON 1
#define RELAY_OFF 0
#define RELAY_TOGGLE 2

#endif
