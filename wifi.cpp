#include <string.h>

#include <esplibs/libnet80211.h> // wifi off
#include <espressif/esp_common.h>

#include <esp8266.h>

#include "config.h"
#include "wifi.h"
#include "wifiSTA.h"
#include "led.h"

void wifi_wait_connection(void)
{
    for(int retry = 20; retry > 0; retry--) {
        if (sdk_wifi_station_get_connect_status() == STATION_GOT_IP) {
            break;
        }
        task_led_blink(2, 5);
        printf(".\n"); // we could show the status
        taskYIELD();
        vTaskDelay(100);
    }
    if (sdk_wifi_station_get_connect_status() != STATION_GOT_IP) {
        sdk_system_restart();
    }
    // printf("\nConnected to wifi\n");
}

void wifi_init(void)
{
    wifi_sta_connect();
}

void wifi_new_connection(char * ssid, char * password)
{
    wifi_sta_new_connection(ssid, password);
}