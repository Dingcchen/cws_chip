/*
 * device.c
 *
 * Created: 2016-08-21 오후 6:35:00
 *  Author: austin
 */ 


#include "device.h"

struct device gDevice;

struct device * get_device_ctx()
{
	/* I believe this will be optimized by modern compiler.
	 * don't spend too much time on dummy job -- austin.
	 */
	return &gDevice;
}