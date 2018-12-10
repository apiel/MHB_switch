#include <lwip/api.h>
#include <string.h>
#include <espressif/esp_common.h>

// #include "action.h"
// #include "utils.h"
#include "config.h"
#include "relay.h"

char * upnp_setup_response()
{
    return (char *)
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
        "<UDN>uuid:Socket-1_0-38323636-4558-4dda-9188-cda0e6cc3dc0</UDN>"
        "<serialNumber>221517K0101769</serialNumber>"
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
        "</root>"
        ;
}

char basicEventResponse[512];
char * basicevent(char * data)
{
    if (strstr((char *)data, (char *)"SetBinaryState")) {
        bool state = strstr((char *)data, (char *)"<BinaryState>1</BinaryState>") ? RELAY_ON : RELAY_OFF;
        printf("SetBinaryState, need to change relay status (%d)\n", state);
        Relay1.setStatus(state);
    }

    sprintf(basicEventResponse, "HTTP/1.1 200 OK\r\n"
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

char * parse_request(void *data)
{
    char * response = NULL;
    // printf("data: %s\n", (char *)data);

    if (strstr((char *)data, (char *)"/wemo/setup.xml")) {
        printf("is setup xml: /wemo/setup.xml\n");
        response = upnp_setup_response();
    } else if (strstr((char *)data, (char *)"/upnp/control/basicevent1")) {
        printf("is basicevent1: /upnp/control/basicevent1\n");
        response = basicevent((char *)data);
    } else {
        printf("unknown route\n");
    }

    return response;
}

#define HTTPD_PORT 80

void httpd_task(void *pvParameters)
{
    struct netconn *client = NULL;
    struct netconn *nc = netconn_new(NETCONN_TCP);
    if (nc == NULL) {
        printf("Failed to allocate socket\n");
        vTaskDelete(NULL);
    }
    netconn_bind(nc, IP_ADDR_ANY, HTTPD_PORT);
    netconn_listen(nc);
    char * response = NULL;
    while (1) {
        err_t err = netconn_accept(nc, &client);
        if (err == ERR_OK) {
            struct netbuf *nb;
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
        }
        netconn_close(client);
        netconn_delete(client);
    }
}