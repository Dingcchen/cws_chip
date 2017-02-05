#if 1
/*
 * greennet.c
 *
 * Created: 2016-08-13 오전 4:56:20
 *  Author: extre
 */ 

/************************************************************************/
/* external dependency                                                  */
/************************************************************************/

/* system */
#include <stdio.h>
#include <string.h>
#include <time.h>

/* modules */
#include "device.h"
//#include "ax_http.h"
#include "ax_task.h"
//#include "ax_socket.h"

/* app */
#include "main.h"
#include "gnet.h"
#include "gnet_hvac.h"
#include "cws_wifi_scheduler.h"

/* choose verbose option */
#define AX_PRINTF	printf
//#define AX_PRINTF	(void)

#define DEBUG_SIMULATED_RESPONSE	0   /* 0 for real resp, 1: original, 2 or above to test variants below */
#define DEBUG_SIMULATED_PARAMETER	0	/* 0 for real resp, 1 for test sceanario */

/************************************************************************/
/* declaration                                                          */
/************************************************************************/

static int _parse_reporting_url(struct dev_hvac *p, const char *url);
static int periodic_read(struct timer_tcb *p);

static int periodic_write(struct timer_tcb *p);

/************************************************************************/
/* implementation                                                       */
/************************************************************************/

static char s_response_buf[MAX_RESP_BUF_SIZE];

#if DEBUG_SIMULATED_RESPONSE > 0
/* test set for dry run or override */
static const char *__debug_resp[] = { "0",
	/* 1: original */
	"\r\n" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"\r\n" "<HVAC_CHIP>"
	"\r\n" "<comm_freq>96</comm_freq>"
	"\r\n" "<write_freq>30</write_freq>"
	"\r\n" "<write_length_time>48</write_length_time>"
	"\r\n" "<demand_resp_code>1</demand_resp_code>"
	"\r\n" "<demand_resp_time>12:00:00</demand_resp_time>"
	"\r\n" "<demand_resp_date>2017-01-04</demand_resp_date>"
	"\r\n" "<current_time>11:59:30</current_time>"
	"\r\n" "<current_date>01/04/2017</current_date>"
	"\r\n" "<delay>500</delay>"
	"\r\n" "<reporting_url>https://www.greennet.com/gmeter/aa/wifihvac.taf?_function=read</reporting_url>"
	"\r\n" "</HVAC_CHIP>"
	,
	/* 1: custom test */
	"\r\n" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"\r\n" "<HVAC_CHIP>"
	"\r\n" "<comm_freq>720</comm_freq>"								// rd every 2-min (720 = 24 x 60 / (2)) - out of spec
	"\r\n" "<write_freq>30</write_freq>"							// wr every 30-sec
	"\r\n" "<write_length_time>1</write_length_time>"				// stop in 1-hour
	"\r\n" "<demand_resp_code>1</demand_resp_code>"					// resp 2
	"\r\n" "<demand_resp_time>12:00:00.0000000</demand_resp_time>"
	"\r\n" "<demand_resp_date>2016-09-01</demand_resp_date>"
		"\r\n" "<current_time>12:00:00</current_time>"
		"\r\n" "<current_date>2016-09-01</current_date>"
	"\r\n" "<delay>5000</delay>"									// noticeable 52-sec
	"\r\n" "<reporting_url>https://www.greennet.com/gmeter/aa/wifihvac.taf?_function=read</reporting_url>"
	"\r\n" "</HVAC_CHIP>"
};
#endif
 
/************************************************************************/
/* implementation                                                       */
/************************************************************************/

int gnet_hvac_init(struct device *pd)
{
	/* attach response buffer should be large enough for take up response  */	 
 	pd->http_response = s_response_buf;

	/* test parameter for greennet access */
	strcpy(pd->mac, "080028000001");
	strcpy(pd->serial, "gHWA-8000001");
	
	/* initial url setup */
	_parse_reporting_url(&pd->hvac, "https://www.greennet.com/gmeter/aa/wifihvac.taf?_function=read");

	/* setup periodic read - write will be setup after reading response */
	task_add("read", periodic_read, pd, -1, 10 * 1000, 0);	// poll every 10-sec

	return 0;
}


static int parse_write_response(int func_mode, char *resp)
{
	char *body;
		
	body = strstr(resp, "\r\n\r\n");
	if (NULL == body)
		return -1;
		
	body += 4;	// skip marker
		
	AX_PRINTF("content: %s\r\n", body);
	
	/* nothing special, but check it is accepted */	
	if (!strstr(resp, "data_accepted"))
		return -1;

	return 0;
}

int parse_write_immediate_response(int func_mode, char *resp)
{

	return 0;
}


/* split url into components */
static int _parse_reporting_url(struct dev_hvac *p, const char *url)
{
	int changed;
	int n, len;
	
	/* if url is same, changed is null, use previously copied url. */
	if (strcmp(url, p->reporting_url) == 0) 
		return 0;

	if (strlen(url) >= sizeof(p->reporting_url))
		return -1;	
	
	/* sscanf has more power than what you might think. google it.
	   some tips here:
			"%[^/]" --> match any except '/'
			"%[^?]" --> match any except '?'
			"%*[0-9]" > match numbers but do not use as arg.
	 */
	n = sscanf(url, "https://%[^/]%[^?]?%[^=]=name", p->url_host, p->url_base, p->url_func);
	if (n < 3)
		return -2;

	
	AX_PRINTF("URL old: %s\r\n", p->reporting_url);
	AX_PRINTF("URL new: %s\r\n", url);
	AX_PRINTF("> host: %s\r\n", p->url_host);
	AX_PRINTF("> base: %s\r\n", p->url_base);
	AX_PRINTF("> func: %s\r\n", p->url_func);

	/* compare host */
	len = strlen(p->url_host);
	changed = strncmp(p->reporting_url + 8, p->url_host, len);
	
	/* backup for later comparison */
	strncpy(p->reporting_url, url, sizeof(p->reporting_url));

	/* base changed */
	if (changed)
		return 1;

	return 0;
}

/*Demand response time_t*/
static long _parse_time(const char *p)
{
		long h, m, s;
		int n = sscanf(p, "%ld:%ld:%ld", &h, &m, &s);
		if (n < 3) return 0;
		return h * 3600000 + m * 60000 + s * 1000;
}


/*Demand response date */
static long _parse_date(const char *p)
{
	
 long y, m, d;
 int n = sscanf(p, "%ld-%ld-%ld", &y, &m, &d);
 if (n < 3) return 0;
 return m * 1 + d * 1 + y * 1;
}

/*Current time */
static long _parse_current_time(const char *p)
{
	long h, m, s;
	int n = sscanf(p, "%ld:%ld:%ld", &h, &m, &s);
	if (n < 3) return 0;
	return h * 3600000 + m * 60000 + s * 1000;
}


/*Current Date */
static long _parse_current_date(const char *p)
{
	long y, m, d;
	int n = sscanf(p, "%ld/%ld/%ld", &m, &d, &y);
	if (n < 3) return 0;
	return m * 1 + d * 1 + y * 1;
}


static int parse_read_response(int func_mode, char *resp)
{
	struct device *pd = get_device_ctx();
	struct dev_hvac *p = &pd->hvac;
	char *body;
	
	body = strstr(resp, "\r\n\r\n");
	if (NULL == body)
		return -1;
	
	body += 4;	// skip marker

	AX_PRINTF("content: %s\r\n", body);

#if DEBUG_SIMULATED_RESPONSE > 0
	/* for test: override response here */
	strcpy(body, __debug_resp[DEBUG_SIMULATED_RESPONSE]);
	AX_PRINTF("content overrided: %s\r\n", body);
#endif	

	/* parse XML format parameters */
	const char span[] = "<>\r\n";		// we expect tokens of "comm_freq" + "24" + "/comm_freq" + "write_freq" + "15" + ...
	char *v;
	
	char *token = strtok(body, span);	// xml header is ok to drop.
	while (token)
	{
		token = strtok(NULL, span);		// skip to next
		if (!token) break;				// end of parse
		if (token[0] == '/') continue;	// skip trailing key.
		
		if (strcmp(token, "comm_freq") == 0) {
			v = strtok(NULL, span);
			p->comm_freq = atoi(v);
			continue;
		}
		if (strcmp(token, "write_freq") == 0) {
			v = strtok(NULL, span);
			p->write_freq = atoi(v);
			continue;
		}
		if (strcmp(token, "write_length_time") == 0) {
			v = strtok(NULL, span);
			p->write_length_time = atoi(v);
			continue;
		}
		if (strcmp(token, "demand_resp_code") == 0) {
			v = strtok(NULL, span);
			p->demand_resp_code = atoi(v);
			continue;
		}
		if (strcmp(token, "demand_resp_time") == 0) {
			v = strtok(NULL, span);
			p->demand_resp_time = _parse_time(v);
			continue;
		}
		if (strcmp(token, "demand_resp_date") == 0) {
			v = strtok(NULL, span);
			p->demand_resp_date = _parse_date(v);
			continue;
		}
		if (strcmp(token, "current_date") == 0) {
				v = strtok(NULL, span);
				p->current_date = _parse_current_date(v);
				continue;
	    }
			if (strcmp(token, "current_time") == 0) {
				v = strtok(NULL, span);
				p->current_time = _parse_current_time(v);
				continue;
			}
		if (strcmp(token, "delay") == 0) {
			v = strtok(NULL, span);
			p->delay = atoi(v);
			continue;
		}
		if (strcmp(token, "reporting_url") == 0) {
			v = strtok(NULL, span);
			p->url_chaged = _parse_reporting_url(p, v) > 0 ? true : false;
			continue;
		}
	}
	
	/* summarize HVAC operation parameter */
	AX_PRINTF("XML parameters: \r\n"
			"\t comm_freq = %d \r\n"
			"\t write_freq = %d \r\n"
			"\t write_length_time = %d \r\n"
			"\t demand_resp_code = %d \r\n" 
			"\t demand_resp_time = %d \r\n"
			"\t demand_resp_date = %d \r\n"
			"\t current_time = %d \r\n"
			"\t current_date = %d \r\n"
			"\t delay = %d \r\n"
			"\t reporting_url = '%s' \r\n"
			,
			p->comm_freq,
			p->write_freq,
			p->write_length_time,
			p->demand_resp_code,
			p->demand_resp_time,
			p->demand_resp_date,
			p->current_time,
			p->current_date,
			p->delay,
			p->reporting_url
		);
		
		
		pd->hvac.demand_resp_code_dup = pd->hvac.demand_resp_code; // storing the demand respond code .
		
		pd->hvac.demand_resp_code = 0;  //Resetting the demand code for normal conditioner
	    
		pd->hvac.demand_event_stage_read = 1;  //Set the event stage after read to check the date 
		
		if (pd->hvac.demand_control_stage == 0)
		{
		      p->demand_event_stage_time = 0;       //Event_stage set to 0 for loading the current_time after every read.
		}
	/* validate HVAC operation schedule here */
	
	/* determine HVAC operation schedule */
	if (DEBUG_SIMULATED_PARAMETER == 0)
	{
		/* see readme.txt for operation diagram 
		*/  
		unsigned long read_msec = 86400000 / p->comm_freq; //For random reads in a day
		unsigned long writ_msec = p->write_freq * 1000;				// seconds
		unsigned long expire_ms = p->write_length_time * 3600000;	// hours
		unsigned long read_msec_t = 60 * 1000;
		

		/* adjust periodic r/w */
		task_remove_by_name("read");
		task_remove_by_name("write");
		task_add("read", periodic_read, pd, read_msec, read_msec, 0);
		task_add("write", periodic_write, pd, p->delay, writ_msec, expire_ms);
	
		
		if (p->url_chaged)
			pd->wifi.valid_domain_status = 0;
	}
	/* override HVAC operation schedule */
	if (DEBUG_SIMULATED_PARAMETER)
	{
		/* see readme.txt for operation diagram 
		*/
		unsigned long read_msec =  120 * 1000;
		unsigned long delay_ms  =   2 * 1000;
		unsigned long writ_msec =  180 * 1000;
		unsigned long expire_ms =  1200 * 1000;
		unsigned long read_msec_t = 60 * 1000;
		
		int w0_ms =   0 * 1000;
		int w1_ms =  40 * 1000;
		int w2_ms =  90 * 1000;
		

		/* adjust periodic r/w */
		task_remove_by_name("read");
		task_remove_by_name("write");
		task_add("read", periodic_read, pd, -1, read_msec, 0);
		task_add("write", periodic_write, pd, delay_ms, writ_msec, expire_ms);
		
// 		if (p->url_chaged)
// 			pd->wifi.valid_domain_status = 0;
	}

	return 0;
}


static int gnet_cmd_read(struct device *pd)
{
	char req[GREENNET_HTTP_REQ_MAXLEN];

	/* cmd format:
		AAF:	/gmeter/aa/automaticairfilter.taf?_function=read&serial=AAG10022001&mac=00409D74E237&comm_freq
		HVAC:	/gmeter/aa/wifihvac.taf?_function=read&serial=gHWA-8000001&mac=080028000001
		*/
	int n = snprintf(req, sizeof(req),
		"GET %s?%s=%s" "&serial=%s" "&mac=%s"
		" HTTP/1.1\r\nHost: %s\r\nUser-Agent: HVAC-WINC1500\r\nConnection: Keep-Alive\r\n\r\n",
		pd->hvac.url_base, pd->hvac.url_func, "read",
		pd->serial, pd->mac, 
		pd->hvac.url_host);
		
			
	//ax_http_send_command(pd->http);
	int err = http_command_request(pd, 0, req, parse_read_response);
	if (err < 0)
		return err;

	return 0;
}



static int gnet_cmd_write(struct device *pd)
{
	char req[GREENNET_HTTP_REQ_MAXLEN];
			
	/* cmd format:
		AAF:	/gmeter/aa/automaticairfilter.taf?_function=write&serial=AAG10022001&mac=00409D74E237&roll_pos=%d&status=2&warning=2
		HVAC:	/gmeter/aa/wifihvac.taf?_function=write&serial=gHWA-8000001&mac=080028000001&comp_status=0&fan_status=0&temp_alert=0&temp=78.0
	*/
	int n = snprintf(req, sizeof(req),
		"GET %s?%s=%s" "&serial=%s" "&mac=%s"
		"&comp_status=%d" "&fan_status=%d" "&temp_alert=%d" "&temp=%d"
		" HTTP/1.1\r\nHost: %s\r\nUser-Agent: HVAC-WINC1500\r\nConnection: Keep-Alive\r\n\r\n",
		pd->hvac.url_base, pd->hvac.url_func, "write",
		pd->serial, pd->mac, 
		pd->hvac.compressor, pd->hvac.fan, /* pd->hvac.heater */ pd->hvac.temperature > 80 ? 1: 0, pd->hvac.temperature,
		pd->hvac.url_host);
											
	int err = http_command_request(pd, 1, req, parse_write_response);
	if (err < 0)
		return err;

	return 0;
}

int gnet_cmd_write_immediate(struct device *pd)
{
	char req[GREENNET_HTTP_REQ_MAXLEN*2]; // longer req
	struct dev_hvac *p = &pd->hvac;
	
	/* cmd format:
		HVAC:	/gmeter/aa/wifihvac.taf?_function=write_immediate&serial=gHWA-8000001&mac=080028000001&status_at_event_comp=0&status_at_event_fan=1 ...
	*/
	int n = snprintf(req, sizeof(req),
		"GET %s?%s=%s" "&serial=%s" "&mac=%s"
		"&status_at_event_comp=%d" "&status_at_event_fan=%d"
		"&status_after_event_comp=%d" "&status_after_event_fan=%d"
		"&restart_chk_comp=%d" "&restart_chk_fan=%d"
		" HTTP/1.1\r\nHost: %s\r\nUser-Agent: HVAC-WINC1500\r\nConnection: Keep-Alive\r\n\r\n",
		pd->hvac.url_base, pd->hvac.url_func, "write_immediate",
		pd->serial, pd->mac, 
		p->status_at_event_comp, p->status_at_event_fan,
		p->status_after_event_comp, p->status_after_event_fan,
		p->restart_chk_comp, p->restart_chk_fan,		
		pd->hvac.url_host);
											
	int err = http_command_request(pd, 1, req, parse_write_immediate_response);
	if (err < 0)
		return err;

	return 0;
}

/************************************************************************/
/*                                                                      */
/************************************************************************/

static int periodic_read(struct timer_tcb *p)
{
	struct device *pd = p->data;
	
	AX_PRINTF("> read periodic\r\n");
	
	int err = gnet_cmd_read(pd);
	if (err < 0)
		return err;

	return 0;
}

static int periodic_write(struct timer_tcb *p)
{
	struct device *pd = p->data;
	
	AX_PRINTF("> wrtie periodic\r\n");
	
	int err = gnet_cmd_write(pd);
	if (err < 0)
		return err;

	return 0;
}

int periodic_write_demand(struct timer_tcb *p)
{
	struct device *pd = p->data;

	/* three oneshot call shares this function, so split using its name */	
	
	if (strcmp(p->name, "w0") == 0)
	{
		AX_PRINTF("> event start\r\n");

		pd->hvac.status_at_event_comp = pd->hvac.compressor;
		pd->hvac.status_at_event_fan = pd->hvac.fan;
	}
	if (strcmp(p->name, "w1") == 0)
	{
		AX_PRINTF("> event finish\r\n");

		pd->hvac.status_after_event_comp = pd->hvac.compressor;
		pd->hvac.status_after_event_fan = pd->hvac.fan;
	}
	if (strcmp(p->name, "w2") == 0)
	{
		AX_PRINTF("> event recheck\r\n");

		pd->hvac.restart_chk_comp = pd->hvac.compressor;
		pd->hvac.restart_chk_fan = pd->hvac.fan;
	}

	int err = gnet_cmd_write_immediate(p->data);
	if (err < 0)
		return err;

	return 0;
}


#endif