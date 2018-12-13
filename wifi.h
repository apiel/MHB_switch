
#ifndef __WIFI_H__
#define __WIFI_H__

void wifi_init(void);
void wifi_new_connection(char * ssid, char * password);
void wifi_wait_connection(void);

#endif
