/*
 * ax_task.h
 *
 * Created: 2016-08-08 오전 11:34:48
 *  Author: Austin Shin <extrealm@gmail.com>
 */ 


#ifndef AX_TASK_H_
#define AX_TASK_H_

/*
	simple utility for software timer emulated task
	1. call task process during main loop,
	2. add/remove async call in other place.
 */

int task_init(void);
int task_add(const char* name, void *call, void *data, int msec_after, int repeat_msec, int expire_msec);
int task_remove_by_exec(void *func);
int task_remove_by_name(const char *name);
int task_process(void);


/* porting layer of tick function */
unsigned long get_tick_ms(void);

/**/	
struct timer_tcb {
	int start0, start;
	int trigger;
	int repeat;
	int expire;
	int(*exec)(void*);
	void *data;
	const char* name;
	void *debug;
};

#endif /* AX_TASK_H_ */