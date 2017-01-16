/*
 * device.h
 *
 * Created: 2016-08-21 오후 6:29:42
 *  Author: austin
 */ 


#ifndef DEVICE_H_
#define DEVICE_H_

#define MAX_RESP_BUF_SIZE		1024
#define MAX_RECV_BUF_SIZE		256

struct device * get_device_ctx(void);
struct device * device_init(void);

struct network_status {
	char initialized;
	char connected;
	char extra_link_status;
	char valid_domain_status;
	char read_sock_connected;
	char write_sock_connected;
	char packet_tx_status;
	char packet_rx_done;
	int  wifi_link_state;
	int  ap_provision_state;
	//unsigned long server_ipv4;
};


struct dev_hvac {
	char serial[15];
	char mac[15];

	/* gpio status */
	int fan;
	int heater;
	int compressor;
	int temperature;
	
	/* event report */
	int status_at_event_comp;
	int status_at_event_fan;
	int status_after_event_comp;
	int status_after_event_fan;
	int restart_chk_comp;
	int restart_chk_fan;

	/* read fields */
	int comm_freq;
	int write_freq;
	int write_length_time;	// life time of write report
	int demand_resp_code;
	int demand_resp_time;
	int demand_resp_date;
	int current_time;
	int current_date;
	int delay;
	char reporting_url[256];
	
	/* fraction of url components */
	char url_host[32];
	char url_base[32];
	char url_func[32];
	int url_chaged;
	
	/*restart device after any event*/
	int restart_dev;
	
	/*When the demand date and time occurs */
	int demand_event_stage_read;
	int demand_control_stage;
	int demand_date_event;
	int demand_time_event;
	int demand_resp_code_dup;
	int demand_event_stage_time;
};

struct device {
	/* device id */
	char mac[13];			/* mac address 12-byte + '\0' */
	char serial[12];		/* serial address 11-byte + '\0' */
	
	/* wifi router candidates */
	int aplist_index;
	// struct cws_ap *cws_ap_list;
	
	/* wifi module */
	struct network_status wifi;
	unsigned long local_ipv4;
	unsigned long server_ipv4;
	
	/* sock buffer */
	char rcv_buffer[MAX_RECV_BUF_SIZE];
	int rcv_ofs;

	/* http module */
	int http_sock;
	char *http_request;
	char *http_response;
	int (*response_cb)(int func_mode, char *resp_str);

	/* hvac module */
	struct dev_hvac hvac;
};

#endif /* DEVICE_H_ */