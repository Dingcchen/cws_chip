/*
 * greennet.h
 *
 * Created: 2016-08-31 오전 2:43:32
 *  Author: austin
 */ 


#ifndef GREENNET_H_
#define GREENNET_H_


#define GREENNET_URL			"www.greennet.com"
/** Using broadcast address for simplicity. */
#define GREENNET_SERVER_PORT                    (443)
#define GREENNET_FLAGS_SSL			SOCKET_FLAGS_SSL

/* Request Header Format */
#define GREENNET_HTTP_REQUEST		"GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: HVAC-WINC1500\r\nConnection: Keep-Alive\r\n\r\n"
#define GREENNET_HTTP_REQ_MAXLEN	(256)
#define GREENNET_HTTP_RESP_MAXLEN	(512)



#endif /* GREENNET_H_ */