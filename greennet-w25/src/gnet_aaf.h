
#ifndef CWS_PROTOCOL_H_INCLUDED
#define CWS_PROTOCOL_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"



typedef enum {
	READ_FUNC		= 0,
	WRITE_FUNC,
	WRITEIM_FUNC,
	VIEW_FUNC,

	REST_FUNC_MAX,
} REST_FUNC_MOD;


typedef enum {
#define CMDS(Enum, String) Enum,
	#include "cmd_list.inc"
#undef CMDS
} CWS_CMDS;

extern const char* Greennet_Cmds[];

/*


*/
typedef enum {
  FREQ_1 = 1,  /* once/day(24hours) */
  FREQ_2 = 2,  /* twice/day(every 12hrs) */
  FREQ_4 = 4   /* 4/day(every 6hrs) */
} COMM_FREQ_VAL;

struct atmflt_r_comm_freq {
  unsigned char* cmd_str;
  COMM_FREQ_VAL cmd_val;
  void *next_cmd;
};

/*


*/
/* 4 weeks (Monthly is the default) */
typedef enum {
  SCH_1 = 1,
  SCH_2 = 2,
  SCH_4 = 4,
  SCH_8 = 8,
  SCH_12 = 12,
  SCH_16 = 16,
} SCHEDULE_VAL;

struct atmflt_r_schedule{
  unsigned char* cmd_str;
  SCHEDULE_VAL cmd_val;
  void *next_cmd;
};

/*

*/

typedef enum {
  STA_1 = 1,  /* Good(green) */
  STA_2 = 2,  /* Warning(Amber) */
  STA_3 = 3,  /* Fault(Red) */
} STATUS_VAL;


struct atmflt_w_led_status{
  unsigned char* cmd_str;
  STATUS_VAL cmd_val;
  void *next_cmd;
};

/*


*/
typedef enum {
  WARN_1 = 1,  /* No Warning */
  WARN_2 = 2,  /* Battery Power Low */
  WARN_3 = 3,  /* Filter Media Low */
} WARNING_VAL;

struct atmflt_w_warning{
  unsigned char* cmd_str;
  STATUS_VAL cmd_val;
  void *next_cmd;
};


/*


*/

typedef enum {
  FAULT_1 = 1,
  FAULT_2 = 2,
  FAULT_3 = 3,
  FAULT_4 = 4,
} FAULT_VAL;

struct atmflt_w_fault{
  unsigned char* cmd_str;
  FAULT_VAL cmd_val;
  void *next_cmd;
};

/*


*/

typedef enum {
  R_POS_0 = 0,
  R_POS_1,
  R_POS_2,
  R_POS_3,
  R_POS_4,
  R_POS_5,
  R_POS_6,
  R_POS_7,
  R_POS_8,
  R_POS_9,
  R_POS_10,
  R_POS_11,
  R_POS_12,
} ROLL_POS_VAL;

struct atmflt_w_rollpos{
  unsigned char* cmd_str;
  ROLL_POS_VAL cmd_val;
  void *next_cmd;
};


struct atmfilt_r_packet_header {
  unsigned char* url_str;
  void* next_cmd;
};

struct atmfilt_w_packet_header {
  unsigned char* url_str;
  void* next_cmd;
};


struct cws_control_info {
	char serial_val[11];
	char mac_val[7];	//mac 6byte + '\0'
	
	SCHEDULE_VAL schedule_val;
	COMM_FREQ_VAL comm_freq_val;

	STATUS_VAL status_val;
	WARNING_VAL warning_val;
	FAULT_VAL fault_val;
	ROLL_POS_VAL poll_pos_val;


};

int restpacket_create(REST_FUNC_MOD cmdtype, char *packet_buf);
int restpacket_add_item(CWS_CMDS cmd, char *val);
int restpacket_close(void);
int parse_response(char *respstr, struct cws_control_info *pstatus);

#ifdef __cplusplus
}
#endif

#endif /* CTL_ALGORITHM_H_INCLUDED */

