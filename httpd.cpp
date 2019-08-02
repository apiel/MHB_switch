#include <lwip/api.h>
#include <string.h>
#include <espressif/esp_common.h>

#include "config.h"
#include "relay.h"
#include "wifi.h"

char setupResponse[1024];
void upnp_set_setup_response()
{
    char uid[50];
    uint8_t hwaddr[6];
    if (sdk_wifi_get_macaddr(STATION_IF, (uint8_t*)hwaddr)) {
        snprintf(uid, sizeof(uid), DEVICE_ID MACSTR, MAC2STR(hwaddr));
    } else {
        strcpy(uid, DEVICE_ID);
    }
    printf("-> Device unique id for upnp: %s\n", uid);

    snprintf(setupResponse, sizeof(setupResponse),
        "HTTP/1.1 200 OK\r\n"
        "Content-type: application/xml\r\n\r\n"
        "<?xml version=\"1.0\"?>"
        "<root>"
        "<device>"
        "<deviceType>urn:Belkin:device:controllee:1</deviceType>"
        "<friendlyName>"
        DEVICE_NAME
        "</friendlyName>"
        "<manufacturer>Belkin International Inc.</manufacturer>"
        "<modelName>Emulated Socket</modelName>"
        "<modelNumber>3.1415</modelNumber>"
        "<UDN>uuid:Socket-1_0-%s</UDN>"
        "<serialNumber>%s</serialNumber>"
        "<binaryState>0</binaryState>"
        "<serviceList>"
        "<service>"
        "<serviceType>urn:Belkin:service:basicevent:1</serviceType>"
        "<serviceId>urn:Belkin:serviceId:basicevent1</serviceId>"
        "<controlURL>/upnp/control/basicevent1</controlURL>"
        "<eventSubURL>/upnp/event/basicevent1</eventSubURL>"
        "<SCPDURL>/eventservice.xml</SCPDURL>"
        "</service>"
        "</serviceList>"
        "</device>"
        "</root>", uid, uid);
}
char * upnp_setup_response()
{
    return setupResponse;
}

char basicEventResponse[512];
char * basicevent(char * data)
{
    if (strstr((char *)data, (char *)"SetBinaryState")) {
        bool state = strstr((char *)data, (char *)"<BinaryState>1</BinaryState>") ? RELAY_ON : RELAY_OFF;
        printf("SetBinaryState, need to change relay status (%d)\n", state);
        Relay1.setStatus(state);
    }

    snprintf(basicEventResponse, sizeof(basicEventResponse),
        "HTTP/1.1 200 OK\r\n"
        "Content-type: application/xml\r\n\r\n"
        "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
        "<s:Body>"
        "<u:GetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">"
        "<BinaryState>%d</BinaryState>"
        "</u:GetBinaryStateResponse>"
        "</s:Body>"
        "</s:Envelope>",
        Relay1.status());

    return basicEventResponse;
}

char statusResponse[512];
char * status()
{
    snprintf(statusResponse, sizeof(statusResponse),
        "HTTP/1.1 200 OK\r\n"
        "Content-type: application/json\r\n\r\n"
        "%s",
        Relay1.status() ? "on": "off");

    return statusResponse;
}

char uptimeResponse[128];
char * getUptime()
{
    int sec = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
    int min = sec * 0.01666667;
    sec -= min * 60;
    int h = min * 0.01666667;
    min -= h*60;
    int day = h * 0.041666667;
    h -= day * 24;

    snprintf(uptimeResponse, sizeof(uptimeResponse),
        "HTTP/1.1 200 OK\r\n"
        "Content-type: application/json\r\n\r\n"
        "%d day %d hrs %d min %d sec",
        day, h, min, sec);

    return uptimeResponse;
}

char * parse_request(void *data)
{
    char * response = NULL;
    // printf("data: %s\n", (char *)data);
    if (strstr((char *)data, (char *)"/on")) {
        Relay1.on();
        response = status();
    } else if (strstr((char *)data, (char *)"/off")) {
        Relay1.off();
        response = status();
    } else if (strstr((char *)data, (char *)"/toggle")) {
        Relay1.toggle();
        response = status();
    } else if (strstr((char *)data, (char *)"/status")) {
        printf("Get status\n");
        response = status();
    } else if (strstr((char *)data, (char *)"/wemo/setup.xml")) {
        printf("is setup xml: /wemo/setup.xml\n");
        response = upnp_setup_response();
    } else if (strstr((char *)data, (char *)"/upnp/control/basicevent1")) {
        printf("is basicevent1: /upnp/control/basicevent1\n");
        response = basicevent((char *)data);
    } else if (strstr((char *)data, (char *)"/uptime")) {
        response = getUptime();
    } else if (strstr((char *)data, (char *)"/reboot")) {
        printf("Reboot\n");
        sdk_system_restart();
    } else {
        printf("unknown route\n");
    }

    return response;
}

#define HTTPD_PORT 80

struct netconn *client = NULL;
struct netconn *nc = NULL;

void httpd_init()
{
    nc = netconn_new(NETCONN_TCP);
    if (nc == NULL) {
        printf("Failed to allocate socket\n");
        vTaskDelete(NULL);
    }
    upnp_set_setup_response();
    netconn_bind(nc, IP_ADDR_ANY, HTTPD_PORT);
    netconn_listen(nc);
    netconn_set_recvtimeout(nc, 2000);
    netconn_set_sendtimeout(nc, 2000);
}

void httpd_loop()
{
    char * response = NULL;
    printf("> httpd\n");
    err_t err = netconn_accept(nc, &client);
    if (err == ERR_OK) {
        struct netbuf *nb;
        netconn_set_recvtimeout(client, 2000);
        if ((err = netconn_recv(client, &nb)) == ERR_OK) {
            void *data = NULL;
            u16_t len;
            if (netbuf_data(nb, &data, &len) == ERR_OK) {
                // printf("Received data:\n%.*s\n", len, (char*) data);
                response = parse_request(data);
            }
            if (!response) {
                response = (char *)"HTTP/1.1 404 OK\r\nContent-type: text/html\r\n\r\nUnknown route\r\n";
            }
            netconn_write(client, response, strlen(response), NETCONN_COPY);
        }
        netbuf_delete(nb);
        netconn_close(client);
    } else {
        netconn_delete(client);
    }
}
