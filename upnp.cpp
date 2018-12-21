#include "config.h"

#include <unistd.h>
#include <string.h>
#include <lwip/udp.h>
#include <lwip/igmp.h>
#include <lwip/api.h>
#include <espressif/esp_common.h>

#include "upnp.h"
// #include "wifi.h"

#define UPNP_MCAST_GRP  ("239.255.255.250")
#define UPNP_MCAST_PORT (1900)

static const char* get_my_ip(void)
{
    static char ip[16] = "0.0.0.0";
    ip[0] = 0;
    struct ip_info ipinfo;
    (void) sdk_wifi_get_ip_info(STATION_IF, &ipinfo);
    snprintf(ip, sizeof(ip), IPSTR, IP2STR(&ipinfo.ip));
    return (char*) ip;
}

ip_addr_t ipgroup;
struct netif* netif;

static struct udp_pcb* mcast_join_group(char *group_ip, uint16_t group_port, void (* recv)(void * arg, struct udp_pcb * upcb, struct pbuf * p, const ip4_addr * addr, short unsigned int port))
{
    bool status = false;
    struct udp_pcb *upcb;

    printf("Joining mcast group %s:%d\n", group_ip, group_port);
    do {
        upcb = udp_new();
        if (!upcb) {
            printf("Error, udp_new failed");
            break;
        }
        udp_bind(upcb, IP_ADDR_ANY, group_port);
        netif = sdk_system_get_netif(STATION_IF);
        if (!netif) {
            printf("Error, netif is null");
            break;
        }
        if (!(netif->flags & NETIF_FLAG_IGMP)) {
            netif->flags |= NETIF_FLAG_IGMP;
            igmp_start(netif);
        }
        ipaddr_aton(group_ip, &ipgroup);
        err_t err = igmp_joingroup(&netif->ip_addr, &ipgroup);
        if(ERR_OK != err) {
            printf("Failed to join multicast group: %d", err);
            break;
        }
        status = true;
    } while(0);

    if (status) {
        printf("Join successs\n");
        udp_recv(upcb, recv, upcb);
    } else {
        if (upcb) {
            udp_remove(upcb);
        }
        upcb = NULL;
    }
    return upcb;
}

static void send(struct udp_pcb *upcb, const ip_addr_t * addr, u16_t port)
{
    struct pbuf *p;
    char msg[500];

    snprintf(msg, sizeof(msg),
        "HTTP/1.1 200 OK\r\n"
        "CACHE-CONTROL: max-age=86400\r\n"
        "DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n"
        "EXT:\r\n"
        "LOCATION: http://%s:80/wemo/setup.xml\r\n"
        "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
        "01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
        "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
        "ST: urn:Belkin:device:**\r\n"
        "USN: uuid:Socket-1_0-38323636-4558-4dda-9188-cda0e6cc3dc0::urn:Belkin:device:**\r\n"
        "X-User-Agent: redsonic\r\n\r\n", get_my_ip());

    p = pbuf_alloc(PBUF_TRANSPORT, strlen(msg)+1, PBUF_RAM);

    if (!p) {
        printf("Failed to allocate transport buffer\n");
    } else {
        memcpy(p->payload, msg, strlen(msg)+1);
        err_t err = udp_sendto(upcb, p, addr, port);
        if (err < 0) {
            printf("Error sending message: %s (%d)\n", lwip_strerr(err), err);
        }
        // else { // to comment
        //     printf("Sent message '%s'\n", msg);
        // }
        pbuf_free(p);
    }
}

/**
  * @brief This function is called when an UDP datagrm has been received on the port UDP_PORT.
  * @param arg user supplied argument (udp_pcb.recv_arg)
  * @param pcb the udp_pcb which received data
  * @param p the packet buffer that was received
  * @param addr the remote IP address from which the packet was received
  * @param port the remote port from which the packet was received
  * @retval None
  */
static void receive_callback(void *arg, struct udp_pcb *upcb, struct pbuf *p, const ip_addr_t * addr, short unsigned int port)
{
    if (p) {
        send(upcb, addr, port);

        pbuf_free(p);
    }
}

/**
  * @brief Initialize the upnp server
  * @retval true if init was succcessful
  */
bool upnp_server_init(void)
{
    printf("Upnp server init\n\r");
    struct udp_pcb *upcb = mcast_join_group((char*)UPNP_MCAST_GRP, UPNP_MCAST_PORT, receive_callback);
    return (upcb != NULL);
}

void upnp_task(void *pvParameters)
{
    bool ok = false;
    printf("Upnp task\n\r");

    #ifdef UPNP_TIMEOUT
    bool end = false;
    #endif

    while (1) {
        #ifdef UPNP_TIMEOUT
        int uptime = xTaskGetTickCount() * portTICK_PERIOD_MS / 1000;
        #endif

        if (!ok) ok = upnp_server_init();
        #ifdef UPNP_TIMEOUT
        else if (!end && uptime > UPNP_TIMEOUT) {
            err_t err = igmp_leavegroup(&netif->ip_addr, &ipgroup);
            printf("Leave upnp (err should be 0: %d)\n", err);
            end = true;
        }
        #endif
        vTaskDelay(1000);
    }
}
