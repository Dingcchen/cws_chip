/*
 * device.c
 *
 * Created: 2016-08-21 오후 6:35:00
 *  Author: austin
 */ 


#include "device.h"
#include "string.h"

struct device gDevice;

struct device * device_init(void)
{
	memset(&gDevice, 0, sizeof(struct device));
	gDevice.wifi.initialized = 1;
	// gDevice.wifi.wifi_link_state = SYS_AP_CONNECT_BEGIN;
	return &gDevice;
}

struct device * get_device_ctx()
{
	/* I believe this will be optimized by modern compiler.
	 * don't spend too much time on dummy job -- austin.
	 */
	return &gDevice;
}