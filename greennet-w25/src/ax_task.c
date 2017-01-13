/*
  Simple Task Manager using tick timer comparison

	feature:
		- using static array
		- oneshot or periodic

	how to use:
		- just port milisecond tick and call task_process() in your main loop.
		- add task, for example, task_add("poll", poll_task, null, 100, 5000, 60000); <-- call poll_task after 100-msec and repeat every 5-sec, until it expired after 1-min.
 *
 * Created: 2016-08-06 오전 4:09:06
 *  Author: austin
 */ 

#ifdef _WIN32
#include <windows.h>
#endif
#include <stdio.h>
#include <string.h>

#include "ax_task.h"

/* choose verbose option */
#define AX_PRINTF	printf
//#define AX_PRINTF	(void)

#define DEBUG_SIMULATED_TASK_TICK	0	/* use simulated tick that making start from 0, and supporting different time scale for fast forward effects */

/************************************************************************/
/* declaration                                                          */
/************************************************************************/
#define MAX_TIMER_TASK	10

static struct timer_tcb g_tasks[MAX_TIMER_TASK];

/************************************************************************/
/* porting                                                              */
/************************************************************************/

extern unsigned long get_time_ms(void);

/*port this for your platform */
static unsigned long __get_task_tick_msec(void)
{
	unsigned long tick;

#ifdef _WIN32
	tick = GetTickCount();
#else
	tick = get_time_ms();
#endif

#if DEBUG_SIMULATED_TASK_TICK
	/* simulate tick using 0-based tick compensation, scale time as fast forwarding
		theoretically, 32-bit counter overflows in 49.7-days (~ 2^32 / 86400000)
		i prefer signed integer for 24.8-days, shorter but we can distinguish a wrap-around happened or not.
	*/
	static int tick0 = 0;
	const int offset = (35788 * 60 * 1000); //MAXINT32 - 1234000 //0 /* msec */
	const int scale = 10;//1	// /* should be smaler than max resolution. e.g., 1000 for app that has 1-sec decision, some task will be skpped. recomends 950 having 50-msec marginal  /*
	
	if (tick0 == 0) tick0 = -tick;
	tick = (tick0 + tick) * scale + offset;
#endif

	return tick;
}

/************************************************************************/
/* internal                                                             */
/************************************************************************/

/* get zero-based index, without error handling
 */
static struct timer_tcb * __task_get(int i)
{
	return &g_tasks[i];	
}

static int __task_find_index(struct timer_tcb *ref)
{
	struct timer_tcb *p;
		
	/* find tcb who has same callproc */
	for (int i = 0; i < MAX_TIMER_TASK; i++)
	{
		p = __task_get(i);
			
		if (p == ref)
			return i;
	}

	return -1;
}

/* find tcb who has given hander call
 */
static struct timer_tcb * __task_find_by_exec(void *exec)
{
	struct timer_tcb *p;
	
	/* find tcb who has same callproc */
	for (int i = 0; i < MAX_TIMER_TASK; i++)
	{
		p = __task_get(i);
		
		if (p->exec == exec)
			return p;
	}

	/* not found */
	return NULL;		
}

static struct timer_tcb * __task_find_by_name(const char *name)
{
	struct timer_tcb *p;
	
	/* find tcb who has same callproc */
	for (int i = 0; i < MAX_TIMER_TASK; i++)
	{
		p = __task_get(i);
		
		if (p->exec && (strcmp(p->name, name) == 0))
			return p;
	}

	/* not found */
	return NULL;
}

/************************************************************************/
/* user api                                                             */
/************************************************************************/

/* clear task struct
 */
int task_init()
{
	struct timer_tcb *p;

	for (int i = 0; i < MAX_TIMER_TASK; i++)
	{
		p = __task_get(i);

		memset(p, 0, sizeof(struct timer_tcb));;
	}

	return 0;
}

/* add task to the task queue.
   this means a given func will be called after msec triggered.
   note:
   - name must reside in code or global space if you plan any comparison later. The shorter, the efficient.
   - zero delay means call-it-now (no-delay), and negative delay means same delay of repeat interval (start after repeat interval)
   - zero expire means no-expire by repeating unlimitedly (range 2^31 covers 24.7-days max)
 */
int task_add(const char* name, void *call, void *data, int delay_msec, int repeat_msec, int expire_msec)
{
	struct timer_tcb *p;
	unsigned long tick = __get_task_tick_msec();
	
	p = __task_find_by_exec(NULL);
	if (!p)
		return -1;	// full
	
	/* for lazy guy, just use -1 for initial delay to be same to repeat_msec, common scenario for many timers. */
	if (delay_msec < 0)
		delay_msec = repeat_msec;

	/* only start contains absolute tick value. all should use relative comparisons to avoid wrap-around side effects */
	p->start0 = p->start = tick;
	p->trigger = delay_msec;
	p->repeat = repeat_msec;
	p->expire = expire_msec;
	p->exec = call;
	p->data = data;
	p->name = name;
	
	return 0;
}

/* remove task from a task queue
 */
int task_remove_by_exec(void* call)
{
	struct timer_tcb *p;
	
	p = __task_find_by_exec(call);
	if (!p)
		return -1;
	
	p->exec = NULL;
	
	return 0;
}

int task_remove_by_name(const char *name)
{
	struct timer_tcb *p;
	
	p = __task_find_by_name(name);
	if (!p)
		return -1;
	
	p->exec = NULL;
	
	return 0;
}


/************************************************************************/
/* debug block - ignore this since it is not yet. you may customize.    */
/************************************************************************/

/* debug support for performance or stats.
 */
struct perf_profile {
	int t0;
	int start;
	int end;
	int lag;
	int jitter;
	int sum;
	int count;
};

enum profile_stage {
	PP_START,
	PP_END,
	PP_SUMMARY
};

static struct perf_profile s_tdb[MAX_TIMER_TASK];

static void __debug_profile(struct timer_tcb *p, enum profile_stage stage)
{
	if (!p)
		return;

	if (!p->debug)
		p->debug = &s_tdb[__task_find_index(p)];	// attach one if empty.

	struct perf_profile *d = p->debug;
	
	int tick = __get_task_tick_msec();

	switch (stage)
	{
		case PP_START:	/* start */
		{
			d->start = tick;
			d->lag = tick - p->trigger;
			d->jitter += d->lag;
		}
		break;

		case PP_END:	/* end */
		{
			d->end = tick;
			d->sum += (d->end - d->start);
			d->count++;
		
		}
		break;
		
		case PP_SUMMARY:	/* some of post calculus time consuming, and prints */
		{
			int cnt = d->count ? d->count : 1;
			AX_PRINTF("> call lagged  = %d-msec\r\n" , d->lag);
			AX_PRINTF("> call elapsed = %d-msec\r\n" , (d->end - d->start));
			AX_PRINTF("> call jitter  = %d-msec\r\n" , d->jitter / cnt);
			AX_PRINTF("> call e avg   = %d-msec\r\n" , d->sum / cnt);
		}
		break;
	}
}

/************************************************************************/
/* main process - must be call within main loop                         */
/************************************************************************/

/* process task handler by checking timestamp
   automatically remove from a task queue or repeat periodically
 */
int task_process()
{
	struct timer_tcb *p;
	int tick;
	int ret;
	
	/* get current time tick */
	tick = __get_task_tick_msec();

	for (int i = 0; i < MAX_TIMER_TASK; i++)
	{
		p = __task_get(i);
		if (!p)
			continue;
		
		/* skip no valid handler */
		if (!p->exec)
			continue;

		/* check event to be triggered */
		if (tick - p->start > p->trigger)
		{
			
			AX_PRINTF("[%d:%02d.%03d] task '%s' triggered ========{{ (#%d, delay:%d, jitter:%d)\r\n", tick / 60000, (tick / 1000) % 60, tick % 1000, p->name ? p->name : "", i, p->trigger, tick - p->start - p->trigger);
			__debug_profile(p, PP_START);
			
			/* execute handler */
			ret = (*(p->exec))(p);
			
			__debug_profile(p, PP_END);
			AX_PRINTF("[%d:%02d.%03d] ========}} task '%s' done (ret:%d)\r\n", tick / 60000, (tick / 1000) % 60, tick % 1000, p->name ? p->name : "", ret);
			
			//__debug_profile(p, PP_SUMMARY);	/* verbose */

			/* update previous (not using tick to keep jitter minimal) */
			p->start += p->trigger;
			
			/* extend for periodic */
			if (p->repeat)
				p->trigger = p->repeat;
				
			/* oneshot */
			if (!p->repeat)
				p->exec = NULL;
			
			/* expire */
			if (p->expire > 0 && (tick - p->start0 > p->expire))
				p->exec = NULL;
				
			if (p->exec == NULL)
				AX_PRINTF("[%d:%02d.%03d] task '%s' expired = = = = = = = = (delay:%d)\r\n", tick / 60000, (tick / 1000) % 60, tick % 1000, p->name ? p->name : "", p->start - p->start0);
		}
	}

	return 0;
}