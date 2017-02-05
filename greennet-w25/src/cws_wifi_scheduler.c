#include "asf.h"
#include <stdio.h>
#include "main.h"
#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"

#include "device.h"
#include "cws_wifi_scheduler.h"
#include "ctl_algorithm.h"
#include "gnet.h"
#include "gnet_hvac.h"
#include "ax_task.h"
#include "ap_provision.h"
#include "adc_cws_temp.h"


#define WIFI_PRINTF printf	/* verbose */
//#define WIFI_PRINTF	(void)	/* silent */

/* should be optimized */
#define DEMAND_TIME_FOR_AP_CONNECT				5000 /* msec */
#define DEMAND_TIME_FOR_CLIENT_IP				3000 /* msec */
#define DEMAND_TIME_FOR_SOCKET_CONNECT			20000 /* msec */
#define DEMAND_TIME_FOR_PACKET_SHAKE			10000 /* msec */




/************************************************************************/
/*                                                                      */
/************************************************************************/
#define IPV4_BYTE(val, index) ((val >> (index * 8)) & 0xFF)

struct cws_ap {
	char	ssid[M2M_MAX_SSID_LEN-1]; /* max 32byte */
	char	password[M2M_MAX_PSK_LEN];
	uint8	authtype;		/*!< Authentication type */
};

static struct cws_ap valid_ap_list[] = {
		{"Media Zone","^^Mountain411desktop^^", M2M_WIFI_SEC_WPA_PSK}, //John
		{"Central","mvndimag", M2M_WIFI_SEC_WPA_PSK},  //Eric Home
		{"enalasysNet","apple1212", M2M_WIFI_SEC_WPA_PSK},  //Office(calexico)
		{"netgear52","silentonion708", M2M_WIFI_SEC_WPA_PSK},   //Dave Clarks
		//{"CWS","ky34A3Ukhq", M2M_WIFI_SEC_WPA_PSK}
};

#define VAILD_AP_LIST_CNT (sizeof(valid_ap_list)/sizeof(struct cws_ap))

/************************************************************************/
/*                                                                      */
/************************************************************************/


static int check_wifi_link_status(struct device *dev);

static int hvac_wifi_init(struct device *dev);
static int hvac_cloud_link_handler(struct device *dev);


/************************************************************************/
/*                                                                      */
/************************************************************************/
extern unsigned long gAcc_MS;
unsigned long get_time_ms(void)
{
	return gAcc_MS;
}


/**
 * \brief Callback to get the Wi-Fi status update.
 *
 * \param[in] u8MsgType type of Wi-Fi notification. Possible types are:
 *  - [M2M_WIFI_RESP_CURRENT_RSSI](@ref M2M_WIFI_RESP_CURRENT_RSSI)
 *  - [M2M_WIFI_RESP_CON_STATE_CHANGED](@ref M2M_WIFI_RESP_CON_STATE_CHANGED)
 *  - [M2M_WIFI_RESP_CONNTION_STATE](@ref M2M_WIFI_RESP_CONNTION_STATE)
 *  - [M2M_WIFI_RESP_SCAN_DONE](@ref M2M_WIFI_RESP_SCAN_DONE)
 *  - [M2M_WIFI_RESP_SCAN_RESULT](@ref M2M_WIFI_RESP_SCAN_RESULT)
 *  - [M2M_WIFI_REQ_WPS](@ref M2M_WIFI_REQ_WPS)
 *  - [M2M_WIFI_RESP_IP_CONFIGURED](@ref M2M_WIFI_RESP_IP_CONFIGURED)
 *  - [M2M_WIFI_RESP_IP_CONFLICT](@ref M2M_WIFI_RESP_IP_CONFLICT)
 *  - [M2M_WIFI_RESP_P2P](@ref M2M_WIFI_RESP_P2P)
 *  - [M2M_WIFI_RESP_AP](@ref M2M_WIFI_RESP_AP)
 *  - [M2M_WIFI_RESP_CLIENT_INFO](@ref M2M_WIFI_RESP_CLIENT_INFO)
 * \param[in] pvMsg A pointer to a buffer containing the notification parameters
 * (if any). It should be casted to the correct data type corresponding to the
 * notification type. Existing types are:
 *  - tstrM2mWifiStateChanged
 *  - tstrM2MWPSInfo
 *  - tstrM2MP2pResp
 *  - tstrM2MAPResp
 *  - tstrM2mScanDone
 *  - tstrM2mWifiscanResult
 */
static void wifi_callback(uint8_t u8MsgType, void *pvMsg)
{
	switch (u8MsgType)
	{
		case M2M_WIFI_RESP_CON_STATE_CHANGED:
		{
			tstrM2mWifiStateChanged *pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
			if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
				printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED\r\n");
				
			} else {
				printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED\r\n");
				printf("> check the WiFi signal or check the correct AP name\r\n");
			}
		}
		break;
		
		case M2M_WIFI_RESP_DEFAULT_CONNECT:
		{
			tstrM2MDefaultConnResp *pResp = (tstrM2MDefaultConnResp *)pvMsg;
			if(pResp->s8ErrorCode == M2M_DEFAULT_CONN_EMPTY_LIST) {
				printf("wifi_cb: M2M_WIFI_RESP_DEFAULT_CONNECT: M2M_DEFAULT_CONN_EMPTY_LIST\r\n");
			} else if(pResp->s8ErrorCode == M2M_DEFAULT_CONN_SCAN_MISMATCH) {
				printf("wifi_cb: M2M_WIFI_RESP_DEFAULT_CONNECT: M2M_DEFAULT_CONN_SCAN_MISMATCH\r\n");
			}
			break;
		}
		case M2M_WIFI_REQ_DHCP_CONF:
		{
			uint8_t *pu8IPAddress = (uint8_t *)pvMsg;
			printf("wifi_cb: M2M_WIFI_REQ_DHCP_CONF: IP is %u.%u.%u.%u\r\n",
				pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);

			struct device *pd = get_device_ctx();
			switch(pd->wifi.wifi_link_state)
			{
				case SYS_AP_CONNECT_DEFAULT_WAIT:
				case  SYS_AP_CONNECT_DOING:
					// Acquired IP address from AP.
					// In AP mode, this event give IP address to client.
					pd->wifi.connected = true;
					break;
				case SYS_AP_PROVISION:
					pd->wifi.connected = (pd->wifi.ap_provision_state == AP_PROVISION_STATE_DONE)?1:0;
					break;
			}
		}
		break;
		
		case M2M_WIFI_RESP_GET_SYS_TIME:
		{
			tstrSystemTime *pstrSystemTime = (tstrSystemTime *)pvMsg;
			
			printf("wifi_cb: M2M_WIFI_RESP_GET_SYS_TIME: time = %d-%d-%d %02d:%02d:%02d\r\n",
				pstrSystemTime->u16Year,
				pstrSystemTime->u8Month,
				pstrSystemTime->u8Day,
				pstrSystemTime->u8Hour,
				pstrSystemTime->u8Minute,
				pstrSystemTime->u8Second);

			struct device *dev = get_device_ctx();
			dev->wifi.extra_link_status = true;

		}
		break;
		
		default:
		{
			printf("wifi_cb: Unhandled event = %d\r\n", u8MsgType);
		}
		break;
	}
}

/**
 * \brief Callback function of IP address.
 *
 * \param[in] hostName Domain name.
 * \param[in] hostIp Server IP.
 *
 * \return None.
 */
static void resolve_callback(uint8_t *hostName, uint32_t hostIp)
{
	printf("resolve_callback: %s resolved with IP %d.%d.%d.%d\r\n",
		hostName,
		(int)IPV4_BYTE(hostIp, 0), (int)IPV4_BYTE(hostIp, 1),
		(int)IPV4_BYTE(hostIp, 2), (int)IPV4_BYTE(hostIp, 3));
	
//	handle_dns_found((char *)hostName, hostIp);
	struct device *dev = get_device_ctx();

	dev->server_ipv4 = hostIp;
	dev->wifi.valid_domain_status = 1;
}

/**
 * \brief Callback to get the Data from socket.
 *
 * \param[in] sock socket handler.
 * \param[in] u8Msg socket event type. Possible values are:
 *  - SOCKET_MSG_BIND
 *  - SOCKET_MSG_LISTEN
 *  - SOCKET_MSG_ACCEPT
 *  - SOCKET_MSG_CONNECT
 *  - SOCKET_MSG_RECV
 *  - SOCKET_MSG_SEND
 *  - SOCKET_MSG_SENDTO
 *  - SOCKET_MSG_RECVFROM
 * \param[in] pvMsg is a pointer to message structure. Existing types are:
 *  - tstrSocketBindMsg
 *  - tstrSocketListenMsg
 *  - tstrSocketAcceptMsg
 *  - tstrSocketConnectMsg
 *  - tstrSocketRecvMsg
 */
static void socket_callback(SOCKET sock, uint8_t u8MsgType, void *pvMsg)
{
	struct device *dev = get_device_ctx();

	switch (u8MsgType)
	{
		case SOCKET_MSG_CONNECT:
		{
			tstrSocketConnectMsg *conmsg = (tstrSocketConnectMsg *)pvMsg;

			/* check the sock is ours */
			if (sock == dev->http_sock)
			{
				if (conmsg->s8Error == SOCK_ERR_NO_ERROR) {
					printf("socket_callback: info: socket %d was connected to the server %s\r\n", sock, dev->hvac.url_host);
					dev->wifi.read_sock_connected = true;
				} else {
					printf("socket_callback: error: failed to connect (err = %d)\r\n", conmsg->s8Error);
					dev->http_sock = -1;
				}
			}
		}
		break;
		
		case SOCKET_MSG_SEND:
		{
			Assert(pvMsg);
			sint16 nSent = *(sint16 *)pvMsg;
			
			/* check the sock is ours */
			if (sock == dev->http_sock)
			{
				/* something wrong happened */
				if (nSent <= 0) {
					printf("socket_callback: error: socket %d sent %d bytes)\r\n", sock, nSent);
					break;
				}
				
				/* read response consecutively */
				if (dev->http_response) {
					recv(dev->http_sock, dev->rcv_buffer, MAX_RECV_BUF_SIZE, 0); /* to do err */
				}
			}
		}
		break;
		
		case SOCKET_MSG_RECV:
		{
			tstrSocketRecvMsg *pstrRecv = (tstrSocketRecvMsg *)pvMsg;

			/* check the sock is ours */
			if (sock == dev->http_sock)
			{
				/* check error */
				if (pstrRecv->s16BufferSize <= 0) {
					printf("socket_callback: error or reset by peer (%d)\r\n", pstrRecv->s16BufferSize);
					dev->wifi.packet_rx_done = 1;
					goto check_done;
				}

				/* protect */
				if (dev->rcv_ofs + pstrRecv->s16BufferSize > MAX_RESP_BUF_SIZE - 1 /* for print safe */) {
					printf("socket_callback: error: reception buffer overflow\r\n");
					goto check_done;
				}
			
				/* append */
				if (pstrRecv->s16BufferSize > 0) {
					memcpy(dev->http_response + dev->rcv_ofs, pstrRecv->pu8Buffer, pstrRecv->s16BufferSize);
					dev->rcv_ofs += pstrRecv->s16BufferSize;
				}
			
			check_done:				
				/* check done. if more to come, cb will be called again automatically. you don't need to call recv again */
				if (pstrRecv->u16RemainingSize == 0)	{
					dev->wifi.packet_rx_done = 1;
					dev->http_response[dev->rcv_ofs] = '\0';	/* make print safe */
					dev->rcv_ofs = 0;
				}
			}
		}
		break;
		
		default:
		{
			printf("socket_callback: Unhandled socket message: msg = %d\r\n" ,u8MsgType);	
		}
		
	} // switch
	
}



static int check_wifi_link_status(struct device *dev_handler)
{
	if ((dev_handler->wifi.valid_domain_status) && (dev_handler->wifi.extra_link_status))
		return 0;
	else
		return -1;
}

int http_command_request(struct device *pd, int func_mode, char *packet_str, int (*respcb)(int func_mode, char *resp_str))
{
	char *pallocstr;

	if (packet_str == NULL)
		return -1;

	if (pd->http_request)
		return -2;
	
	pallocstr = malloc(strlen(packet_str) + 1);

	if (pallocstr == NULL)
		return -3;

	memcpy(pallocstr, packet_str, strlen(packet_str) + 1);

	pd->http_request = pallocstr;
	pd->response_cb = respcb;

	return 0;
}


static int hvac_wifi_init(struct device *dev_handler)
{
	tstrWifiInitParam param;
	sint8 ret;

	/* Initialize the BSP. */
	nm_bsp_init();
	
	/* Initialize Wi-Fi parameters structure. */
	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));
	
	/* Initialize Wi-Fi driver with data and status callbacks. */
	param.pfAppWifiCb = wifi_callback;
	ret = m2m_wifi_init(&param);
	
	if (ret != M2M_SUCCESS)
		return -1;

	device_init();
	
	return 0;
}

static int havc_wifi_link_handler(struct device *dev)
{
	int current_state = dev->wifi.wifi_link_state;
	int next_state = SYS_AP_CONNECT_BEGIN;
	static unsigned long wait_ms = 0, previous_ms = 0;
	sint8 ret;

	switch (current_state) {
		case SYS_AP_CONNECT_BEGIN:
			if (dev->wifi.initialized != 1 && hvac_wifi_init(dev) != M2M_SUCCESS) {
				return -1;
			} else {
				next_state = SYS_AP_CONNECT_DEFAULT;
			}
		case SYS_AP_CONNECT_DEFAULT:
			if(m2m_wifi_default_connect() == M2M_SUCCESS) {
				previous_ms = get_time_ms();
				wait_ms = DEMAND_TIME_FOR_AP_CONNECT;
				next_state = SYS_AP_CONNECT_DEFAULT_WAIT;
			} else {
				next_state = SYS_AP_CONNECT_LIST;
			}
			break;
			
		case SYS_AP_CONNECT_DEFAULT_WAIT:
			if ((get_time_ms() - previous_ms) < wait_ms) {
				if (dev->wifi.connected == 1) {
					next_state = SYS_AP_CONNECT_DONE;
					printf("AP default connection success\r\n");
				} else {
					return 0;
				}
			} else {
				next_state = SYS_AP_CONNECT_LIST;
			}
			break;
			
		case SYS_AP_CONNECT_LIST:
		{
			// struct cws_ap *ap_list = dev->cws_ap_list;
			int index = dev->aplist_index;

			ret = m2m_wifi_connect((char *)valid_ap_list[index].ssid,
				sizeof(valid_ap_list[index].ssid),
				valid_ap_list[index].authtype,
				(char *)valid_ap_list[index].password,
				M2M_WIFI_CH_ALL);

			if (ret != M2M_SUCCESS)
			{
				// next_state starts from begin.
				printf("m2m_wifi_connect() fail = %d\r\n", ret);
			} else {
				previous_ms = get_time_ms();
				wait_ms = DEMAND_TIME_FOR_AP_CONNECT;
				next_state = SYS_AP_CONNECT_DOING;
			}
			break;
		}
		case SYS_AP_CONNECT_DOING:
			if ((get_time_ms() - previous_ms) < wait_ms) {
				if (dev->wifi.connected == 1) {
					next_state = SYS_AP_CONNECT_DONE;
					printf("AP connection success\r\n");
				} else
					next_state = SYS_AP_CONNECT_DOING;
			} else {
				/* retry to another AP as timeout */
				dev->aplist_index++;
				if(dev->aplist_index == VAILD_AP_LIST_CNT) {
					dev->aplist_index = 0;
					// None of listed AP is available. try AP Provision.
					ret = ap_provision_init();
					if(ret >=0) {
						next_state = SYS_AP_PROVISION;
					} else {
						printf("ap_provision_init() fail = %d\r\n", ret);
					}
				} else {
					printf("AP connection retry = %d\r\n", dev->aplist_index);
					next_state = SYS_AP_CONNECT_LIST;
				}
			}
			break;

		case SYS_AP_PROVISION:
			ret = ap_provision();
			dev->wifi.ap_provision_state = ret;
			if (ret >= 0) {
				if(dev->wifi.connected == 1) {
					next_state = SYS_AP_CONNECT_DONE;
					printf("AP Provision connection success\r\n");
				} else {
					// keep waiting
					next_state = SYS_AP_PROVISION;
				}
			} else {
				// Provisioning fails, next state starts from beginning.
			}
			break;
		
		case SYS_AP_CONNECT_DONE:
			printf("debug SYS_AP_CONNECT_DONE \r\n");
			if (dev->wifi.connected == 1) {
				/* Initialize socket module */
				socketInit();
				registerSocketCallback(socket_callback, resolve_callback);
				next_state = SYS_SERVER_GET_IP_BEGIN;
			}
			break;

		case SYS_SERVER_GET_IP_BEGIN:
			/*
			To do : if already know server IP
			*/
			dev->server_ipv4 = 0;
			gethostbyname((uint8_t *)dev->hvac.url_host);
			
			previous_ms = get_time_ms();
			wait_ms = DEMAND_TIME_FOR_CLIENT_IP * 4;

			next_state = SYS_SERVER_GET_IP_DOING;
			break;

		case SYS_SERVER_GET_IP_DOING:
			if ((get_time_ms() - previous_ms) < wait_ms) {
				if (dev->wifi.valid_domain_status == 1) {
					next_state = SYS_SERVER_GET_IP_DONE;
					printf("Debug : Got IP\r\n");
				} else {
					next_state = SYS_SERVER_GET_IP_DOING;
				}
			} else { /* timeout */
				/* retry to get IP from another domain as timeout */
				printf("Invalid Domain\r\n");
				next_state = SYS_SERVER_GET_IP_BEGIN;
			}
			break;

		case SYS_SERVER_GET_IP_DONE:
			printf("debug SYS_SERVER_GET_IP_DONE \r\n");
			next_state = SYS_AP_CONNECT_IDLE;
			break;

		case SYS_AP_CONNECT_IDLE:

			/* check if AP connection is maintained */
			
			/* redirect url */
			if (!dev->wifi.valid_domain_status) {
				next_state = SYS_SERVER_GET_IP_BEGIN;
				break;
			}
			/* To Do */
			next_state = SYS_AP_CONNECT_IDLE;
			break;

		default:
			printf("%s: Error : Undefined state\r\n", __FUNCTION__);
			break;
	}
	
	if(next_state != dev->wifi.wifi_link_state ) {
		printf("Next_state: %d\r\n", next_state);
		dev->wifi.wifi_link_state = next_state;
	}
	
	return 0;
}

static int hvac_cloud_link_handler(struct device *dev)
{
	static int current_state = SYS_SERVER_CONNECT_BEGIN;
	int next_state = SYS_SERVER_CONNECT_BEGIN;
	static unsigned long wait_ms = 0, previous_ms = 0;
	sint8 ret;

	switch (current_state) {
		case SYS_SERVER_CONNECT_BEGIN:
			/* Open TCP client socket. */
			if (dev->wifi.read_sock_connected == 1)
				next_state = SYS_SERVER_CONNECT_DONE;
			else {
				static struct sockaddr_in addr_in;				

				dev->http_sock = socket(AF_INET, SOCK_STREAM, GREENNET_FLAGS_SSL);

				if (dev->http_sock < 0) {
					printf("main: failed to create TCP client socket error!\r\n");
					return -1; //todo
				}

				/* Connect TCP client socket. */
				addr_in.sin_family = AF_INET;
				addr_in.sin_port = _htons(GREENNET_SERVER_PORT);
				addr_in.sin_addr.s_addr = dev->server_ipv4;

				ret = connect(dev->http_sock, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in));
				if (ret != SOCK_ERR_NO_ERROR) {
					printf("main: failed to connect socket error!\r\n");
					close(dev->http_sock);
					dev->http_sock = -1;
				
				} else {
					previous_ms = get_time_ms();
					wait_ms = DEMAND_TIME_FOR_SOCKET_CONNECT;
					next_state = SYS_SERVER_CONNECT_DOING;
				}
			}

			break;

		case SYS_SERVER_CONNECT_DOING:
			if ((get_time_ms() - previous_ms) < wait_ms) {
				if (dev->wifi.read_sock_connected == 1) {
					printf("Debug : Socket Connect Done # lead time = %ld\r\n", get_time_ms() - previous_ms);
					next_state = SYS_SERVER_CONNECT_DONE;
				} else
					next_state = SYS_SERVER_CONNECT_DOING;
			} else {
				/* retry to connect socket as timeout */
				printf("Debug : Socket Connect Fail # lead time = %ld\r\n", get_time_ms() - previous_ms);
				close(dev->http_sock);
				dev->http_sock = -1;
			  
			}

			break;

		case SYS_SERVER_CONNECT_DONE:
			printf("debug SYS_SERVER_CONNECT_DONE \r\n");
			next_state = SYS_SERVER_COMMAND_BEGIN;

			break;

		case SYS_SERVER_COMMAND_BEGIN:
			if (dev->http_request != NULL) {
				sint16 s16Ret, len;

				len = strlen((char *)dev->http_request);

				printf("==============< Request String >=============\r\n");
				printf("%ssize =%d\r\n",dev->http_request, len);

				dev->wifi.packet_rx_done = 0;

				s16Ret = send(dev->http_sock,
								dev->http_request,
								len,
								0);

				if (s16Ret != SOCK_ERR_NO_ERROR) {
					printf("debug : send err = %d\r\n", s16Ret);

				} else {
					previous_ms = get_time_ms();
					wait_ms = DEMAND_TIME_FOR_PACKET_SHAKE;

					next_state = SYS_SERVER_COMMAND_SENDING;
				}
			} else
				next_state = SYS_SERVER_COMMAND_BEGIN;

			break;

		case SYS_SERVER_COMMAND_SENDING:
			if ((get_time_ms() - previous_ms) < wait_ms) {
				if (dev->wifi.packet_rx_done == 1) {
					printf("Debug : Socket Communicaiton Success # lead time = %ld\r\n", get_time_ms() - previous_ms);
					next_state = SYS_SERVER_COMMAND_DONE;
				} else
					next_state = SYS_SERVER_COMMAND_SENDING;
			} else { /* timeout */
			/*
				To Do
			*/         //CWS modified 
						close(dev->http_sock);
						dev->http_sock = -1;
						dev->wifi.read_sock_connected = 0;

						if (dev->http_request != NULL) {
							free(dev->http_request);
							dev->http_request = NULL;
						}
						next_state = SYS_SERVER_CONNECT_BEGIN;
			}

			break;

		case SYS_SERVER_COMMAND_DONE:
			printf("debug SYS_SERVER_COMMAND_DONE \r\n");

			if (dev->response_cb) {
				int e = dev->response_cb(0, dev->http_response);
				if (e < 0)
				printf("response_cb return %d\r\n", e);
			}

			close(dev->http_sock);
			dev->http_sock = -1;
			dev->wifi.read_sock_connected = 0;

			if (dev->http_request != NULL) {
				free(dev->http_request);
				dev->http_request = NULL;
			}
			next_state = SYS_SERVER_COMMAND_IDLE;

			break;

		case SYS_SERVER_COMMAND_IDLE:
			/* check if AP connection is maintained */
			/* check if socket is valid */
			/* check if there is a updated news. if is, change current state */

			if ( dev->http_request != NULL) {
				next_state = SYS_SERVER_CONNECT_BEGIN;
			} else
				next_state = SYS_SERVER_CONNECT_BEGIN; /* beforehand open socket */

			break;

		default:
			printf("%s: Error : Undefined state\r\n", __FUNCTION__);
			break;
	}
	
	if(next_state != current_state ) {
		printf("Next_state: %d\r\n", next_state);
		current_state = next_state;
	}

	return 0;
}

static int network_process(struct device *dev)
{	
	m2m_wifi_handle_events(NULL);
	havc_wifi_link_handler(dev);
	if (check_wifi_link_status(dev) == 0) /* is connected */
		hvac_cloud_link_handler(dev);
	
	return 0;
}





//static void hvac_get_control_status(struct dev_hvac *p)
//{
	//p->compressor	= !(port_pin_get_input_level(COMPRESSOR_INPUT_PIN));
	//p->fan			= !(port_pin_get_input_level(FAN_INPUT_PIN));
	//p->heater		= !(port_pin_get_input_level(HEATER_INPUT_PIN));
//}



int hvac_scheduler(void)
{
	struct device *pd = get_device_ctx();
	struct dev_hvac *p = &pd->hvac;
	
	/* Initialize Pin for CWS */
	ctl_port_init();
	Clean_vars();

	/* utility module */
	task_init();
	
	/* wifi and gnet module */
	hvac_wifi_init(pd);
	gnet_hvac_init(pd);		/* for HVAC */

	while(1) {

		/* must process following for wifi events
		 */
		network_process(pd);
		
		/* temperature Sensor */
		
		adc_temp_sensor();
		
		/* read gpio status */
	//	hvac_get_control_status(&pd->hvac);

		/* more flexible than switch case
		 */
		if (pd->hvac.demand_resp_code == 0 && pd->hvac.demand_control_stage == 0) // Regular conditioner (normal routine)
			control_conditioner();
		 
		//When event occurs 
		 if (pd->hvac.demand_event_stage_read)
			{
	     //Check the demand resp date 
		 if (pd->hvac.demand_resp_date == pd->hvac.current_date)
		   {
			 pd->hvac.demand_date_event = 1;
			
            check_time_for_event();
			  
			if (pd->hvac.demand_date_event == 1 && pd->hvac.demand_time_event ==1 && pd->hvac.demand_resp_code_dup == 1)
              {
			control_conditioner1();
              }
	        
			if (pd->hvac.demand_date_event == 1 && pd->hvac.demand_time_event ==1 && pd->hvac.demand_resp_code_dup == 2)
	          {
		     control_conditioner2();
	          }
		
		    if (pd->hvac.demand_date_event == 1 && pd->hvac.demand_time_event ==1 && pd->hvac.demand_resp_code_dup == 3)
		     {
			 control_conditioner3();
		     }
	
		}
}
		
			if (p->demand_control_stage > 0 && pd->hvac.demand_event_stage_write_immediate == 0)
		{
			int min = (p->demand_resp_code_dup == 1) ? 6 : 12;
		
			
			task_add("w0", periodic_write_demand, pd, 1000, 0, 0); // event right after
			task_add("w1", periodic_write_demand, pd, min * 61000, 0, 0); // 6 or 12-min later
			task_add("w2", periodic_write_demand, pd,  20 * 61000, 0, 0); // 20-min later
			pd->hvac.demand_event_stage_write_immediate = 1;
			
		}

		/* handle timer task - see usage in gnet_hvac.c
		 */
		if (check_wifi_link_status(pd) == 0) {
			// handle task only network is available
			task_process();
		}
	
	}
}

