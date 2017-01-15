============
RELEASE NOTE
============

v1.3	2016-09-06

	- BUG: timer wrap-around side effect after 24.7-days


v1.3	2016-09-04

	* changes:

	- REQ: support 'reporting_url' by checking it url is renewed or not.
	- REQ: enable write_immediate commands and response parser.
	- CHG: enable wifi scheduler to renew the hostname when sever is changed
	- CHG: remove unused wr sock and buffer, since rd's are shared for all http operation
	- CHG: increase reception buffer to have xml response correctly.
	- BUG: fix wrong expiration condition in task module. 
	- BUG: fix wrong memory copy on longer response. happen in xml response.
	- BUG: add missing return code check
	- DBG: add performance measure for task module
	- DBG: add simulated response of xml
	- DBG: add simluated paramters during verification

	* known issues:

		- value of temperature is a dummy for extension.
		- value of heater is not used for reporting -- i believe it is a design spec.
		

CONCEPT
--------

	1. Running up with HVAC control as primary,
		- and wifi connection run in *back-ground* - because HVAC gpio should work regardless of wifi connection.
	2. after wifi connected,
		- send read command to get greennet's operation parameter, and refelect it.
		- write regular reports to the sever
		- write special reports to the sever when requested (event).
		- can report to different url when requested
	3. special gpio operation
		- run special gpio logic as requested (event)

FEATURES
--------

	- [o] hw setup - gpio config
	- [o] gpio control
	- [o] wifi connect
	- [o] tcp socket connect over ssl.
	- [o] http send/recv
	- [o] read command (parameter extraction)
	- [o] write command (gpio report)
	- [o] write immediate (gpio report)
	- [o] parameter handling
	- [o] event scheduling
	- [o] url redirection (rare case i think)

	hence, app covers all the requirements of gnet now.


SPECIAL NOTES
-------------

limitation

	* greennet has fixed response
		- verfieid with the live connection and live data
		- verified using simulated response for short term testing

	* partially tested
		- some scenario requires dynamic change in gnet's response, but they are not.
		- url redirection is one of the case, and i tested by hinting them as if it was different url. (then, it will resolve dns again)

	* unexpected scenario
		- possible. please simulate those case, and add enhancements.



TIMING TEST
-----------
2016-09-04


	* greenet timing
		
		                                       <--- wrpt ---> <--- wrpt --->
		[RD ] ---> (0) read #1 --+------------------------------------------------------//------------------------//---------------------//---- (rrpt) read #2 --+--->
		[WR ]                    +--- delay ---> write #1 ---> write #2 ---> write #3 --//--> write last (wexp)                                                  +--->
		[WRI]                    +--> (w0) at ------------------------------------------//--> (w1) after ---------//----> (w2) check                             +---> (if demand > 0)


	* realworld xml parameter
	
		read interval (rrpt) = 24-hours(!)
		write delay, interval, expire (delay, wrpt, wexp) = 200-msec, 15-sec, 48-hours(!)
		report at, after, check (w0, w1, w2) = 0-sec, 6-min, 20-min

		- 24-48-hrs repeating is not easy for testing, so we need to do scale down.


	* simulation parameter (in msec units)

		unsigned long read_msec = 120 * 1000;
		unsigned long delay_ms  =   2 * 1000;
		unsigned long writ_msec =  15 * 1000;
		unsigned long expire_ms =  60 * 1000;
		
		int w0_ms =   0 * 1000;
		int w1_ms =  40 * 1000;
		int w2_ms =  90 * 1000;

		
	* simulation timing
		                                        <------ 15 ------><------ 15 ------>
		RD -- read #1 (here) --+-------------------------------------------------------------------//---------------//-----------------------//-----------------//---> (2:00) read #2 --+--->
		WR                     +----- 2 ------> (2) write #1 --> (17) write #2 --> (32) write #3 --//- - - - - - - -//---> (1:00) write #n (the last)                                   +--->
		WRI                    +--> (0) w0 --------------------------------------------------------//-- (0:40) w1 --//-----------------------//--(1:30) w2 chk                          +---> (if requested)


	* test points:

		1. read command happens periodically, since it was setup in hvac init
			1. check gpio is initialized
			1. check gpio input and output is working as before.
		2. check system is getting online
			1. wifi associated (with correct ssid and password of your ap)
			1. got ip
			2. get time of today (then it means internet is working)
			3. check the read command is successfully sent after online (read #1)
			4. read #1 successfully read xml response, and check it parse parameter correctly overwritten
		3. read
			1. check any read command is not happening (other writes happens) until next read command (read #2) time in (2-min) --> (GN option is 24-hrs)
			2. check read command repeats every in read interval (2-min), and will repeats 4-min, 6-min.
			3. check read repeats forever
		4. write immediate,
			1. check first (w0) event reports right after read finish, check req string has valid values of gpio status
			2. check sencod (w1) event reports corectly after 40-sec later., check req string has valid values of gpio status.  --> (GN option is 6-min or 12-min)
			3. check third (w2) event reports correctly after 1:30 later. check req string has valid values of gpio status.     --> (GN option is 20-min)
		5. write
			1. check first regular report (write #1) starts after the initial delay (2-sec)										--> (GN option is 200-msec)
			2. check repeated regular reports (write #1 .. write last) happen in every repeat interval (15-sec)					--> (GN option is similarly, 10-60 sec)
			3. check no more regular reports (write #n) happen after expiration time (1:00)                                     --> (GN option is 48-hrs)
		5. other opetion
			1. override demanded_resp_code = 0, and see write_immediate (w0, w1, w2) is not happening
		6. url
			1. check intial url is setup via gnet_hvac_init() and copyed into the context. (it prints in first read)
			3. override p->reporting_url as if was different from what now have (e.g. strcpy(p->reporting_url, "") before xml parsing),
			2. verify p->url_changed is set as 1, and see the DNS operation happen (Got IP),
		7. time jitter
			1. run w25 and check the timestamp of commands running. record the timestamp and a real watch together.
			2. let w25 to freely run for 1 or 2-hours, and later check both timestamp and a real watch again. verify time difference is neglectable.
			3. try more longer aging if needed. integral of time difference should be marginal.

				
	* Tweak items at your side

		1. adjust gpio polarity inversion, i used a value read as is. -- it is trivial.
		2. adjust gpio control logic per different event stage. -- proprietary i don't understand.
		3. adjust other tweaks per your needs.


COMMAND FORMAT
--------------

refer to wifihvac for detail

	- gmeter.API.Wifi.HVAC.Chip.pdf
	- gMeter.API.AA.Draft.pdf

example

	* read
		AAF:	/gmeter/aa/automaticairfilter.taf?_function=read&serial=AAG10022001&mac=00409D74E237&comm_freq
		HVAC:	/gmeter/aa/wifihvac.taf?_function=read&serial=gHWA-8000001&mac=080028000001

	* write
		AAF:	/gmeter/aa/automaticairfilter.taf?_function=write&serial=AAG10022001&mac=00409D74E237&roll_pos=%d&status=2&warning=2
		HVAC:	/gmeter/aa/wifihvac.taf?_function=write&serial=gHWA-8000001&mac=080028000001&comp_status=0&fan_status=0&temp_alert=0&temp=78.0

	* write_immediate
		HVAC:	/gmeter/aa/wifihvac.taf?_function=write_immediate&serial=gHWA-8000001&mac=080028000001&status_at_event_comp=0&status_at_event_fan=1


HISTORY
-------

	v1.3-0904	- write_im, supports url redirection, task bugfix
	v1.2-0901	- xml parsing, scheduled task, bugfix
	v1.1-0831	- wifihvac api (spec defined latified 0824, later live on 0829)
	v1.0-0812	- d21 + w25 (for aaf)
	v1.0-0810	- refactoring
	v1.0-0806	- gpio ports, access gnet with aaf spec.
	v0.x-0728	- interim, gpio ports and logic


CABLES
------

2016-09-01

Hi Kumar and all,

For your good, I depict what I¡¯ve tested:

1.	Board starts with controlling GPIO and trying WiFi connection (as done before)
2.	It also has initial setup of READ in 10-sec periodic, and WRITE in 60-sec. (adjustable)
3.	GPIO control is working regardless of Wi-Fi connection.
4.	10-sec READ was successful with HTTP 200 OK, and it emits XML (it will retry in every 10-sec if fails)
5.	The XML declares, GN want 1-hour periodic READ, and 15-sec periodic WRITE, so we do that.
	Hence, in short term, no READ is seen, but WRITE are observed in every 15-sec.
	Each WRITE was successful as GN responded ¡°data accepted¡±, and its report values are from real GPIO read. (except a temperature)
6.	XML also designated that WRITE pre-delay of 200 msec (hard to distinguish in a test), and it expires after 48-hours
7.	XML also said GN want a demanded event 1 (6-min), so we¡¯ve setup three scheduled reports. The first event ¡°w0¡± is observed right after READ.

Above are from real world, and all you may aware of that:
-	GN gave no interfaces for different configuration ? hence, better to override GN¡¯s request for a test.
-	GN spec itself has lesser room for short term operations, because some base units are in hours or days ? better to override them in sec/min for a test.

Best regards,
Austin.

--

Hi Kumar,

Please find v1.2 attached, this version is more realistic for Greennet, and several things you can test with.

Some code explanation:

1.	New Timer Task module: ax_task.c is module manages GN read/write periodic operations, or for multiple one-shot operations
	a.	The gnet_hvac.c has task setups using task_add() and task_remove() variants.
	b.	The task_process() is the master caller for its added tasks, so the main loop calls it in hvac_scheduler() loop
		Since those tasks are called from the main loop ? not from a interrupt context ? the system dependency will get simpler, just by referencing 1-milisec tick counter.
	c.	Various timing can be setup by defining the initial-delay (0 for right after), repeat-interval (0 for oneshot), and expiration-time (0 for no expiration).
2.	Periodic action
	a.	Periodic read/write is being setup during initialization, as seen in gnet_hvac_init(), 
	b.	And then later, being adjusted its intervals from Greennet designated values.
	c.	Those XML values are extracted in parse_read_response(), then are used for a new setup, and stored in struct dev_hvac for later use.
3.	Demanded response
	a.	The stored demand_resp_code is used for Hvac_scheduler() by providing 4 placeholders for different GPIO control scheme.
		I think this will be compatible to your flow chart.
	b.	For GN requested events handling, periodic_write_demand() will be called 3-times (w0, w1, w2) consecutively at 0-min, 6/12-min, 20-min, as scheduled in parse_read_response() followed action.
4.	Property for dev_hvac
	a.	Struct dev_hvac keeps logical states of GPIO, that are updated by hvac_get_control_status(), and later used for GN R/W and events,
	b.	It also keeps values read from GN as parsed in XML responses,
	c.	It is a member of the device object, so that is accessible via get_device_ctx() globally.
5.	Wifi Scheduler
	a.	Removed redundant socket framework for write action. Actually they works as the same way of the read, so now we can shared the one.
	b.	Removed message queue framework. Actually they are intended for events gathering into single outlet, it was useful, but exposed to a bugs.
6.	GPIOs
	a.	I checked GPIOs using W25, because W25 module is deemed for a real product. I can see D21 exposed a slightly different, but any guys can adapt the pin map. 
		You also can split board dependent codes by referring #if BOARD == SAMW25_XPLAINED_PRO expression.
	b.	Your new Control_conditioner1(), 2(), or 3() are not in my codes, so you can use yours if any updated.
	c.	I have several opinions for enhancements, but the most important is working correctly for the HW system, and being manageable at your side.

Let me know any feedback.


