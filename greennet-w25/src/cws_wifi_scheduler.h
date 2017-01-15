#ifndef _CWS_WIFI_SCHEDULER_H_INCLUDED
#define _CWS_WIFI_SCHEDULER_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"
/*
typedef enum {
	WIFI_STATE_DEFAULT_CONNECTION,
	WIFI_STATE_AP_CONNECTION,
	WIFI_STATE_AP_PROVISION,
	WIFI_STATE_CLOUD_LINK
} WIFI_LINK_STATE;
*/
typedef enum {

	SYS_AP_CONNECT,
		SYS_AP_CONNECT_BEGIN = SYS_AP_CONNECT,
		SYS_AP_CONNECT_DEFAULT,
		SYS_AP_CONNECT_DEFAULT_WAIT,
		SYS_AP_CONNECT_LIST,
		SYS_AP_CONNECT_DOING,
		SYS_AP_CONNECT_DONE,
		SYS_AP_CONNECT_IDLE,
		
	SYS_AP_PROVISION,
		SYS_AP_PROVISION_BEGIN = SYS_AP_PROVISION,
		SYS_AP_PROVISION_DOING,
		SYS_AP_PROVISION_DONE,
		SYS_AP_PROVISION_FAIL,

	SYS_SERVER_CONNECT,
		SYS_SERVER_GET_IP_BEGIN = SYS_SERVER_CONNECT,
		SYS_SERVER_GET_IP_DOING,
		SYS_SERVER_GET_IP_DONE,
		SYS_SERVER_CONNECT_BEGIN,
		SYS_SERVER_CONNECT_DOING,
		SYS_SERVER_CONNECT_DONE,
		SYS_SERVER_COMMAND_BEGIN,
		SYS_SERVER_COMMAND_SENDING,
		SYS_SERVER_COMMAND_DONE,
		SYS_SERVER_COMMAND_IDLE,

} SYSTEM_STATUS;


int hvac_scheduler(void);

unsigned long get_time_ms(void);
int http_command_request(struct device *pd, int func_mode, char *packet_str, int (*respcb)(int func_mode, char *resp_str));


#ifdef __cplusplus
}
#endif

#endif //_CWS_MSG_QUE_H_INCLUDED
