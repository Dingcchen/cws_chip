#if 0
#include "asf.h"
#include <stdio.h>
#include <string.h>
#include "gnet_aaf.h"


const char Greennet_Url[][65] = {
	READ_PREFIX_URL,
	WRITE_PREFIX_URL,
	WRITEIM_PREFIX_URL,
	VIEW_PREFIX_URL,
};


const char* Greennet_Cmds[] = {
#define CMDS(Enum, String) String, //""#String
#include "cmd_list.inc"
#undef CMDS
};

#ifdef _BOGUS_NOT_USED
#define NS_INT16SZ		2
#define NS_INADDRSZ		4
#define NS_IN6ADDRSZ	16
#define EAFNOSUPPORT	97
static int errno = 0;

static int inet_pton4(const char *src, unsigned char *dst)
{
	static const char digits[] = "0123456789";
	int saw_digit, octets, ch;
	unsigned char tmp[NS_INADDRSZ], *tp;

	saw_digit = 0;
	octets = 0;
	*(tp = tmp) = 0;
	while ((ch = *src++) != '\0') {
		const char *pch;

		if ((pch = strchr(digits, ch)) != NULL) {
			unsigned int iNew = (unsigned int)(*tp * 10 + (pch - digits));

			if (iNew > 255)
				return (0);
			*tp = iNew;
			if (!saw_digit) {
		 		if (++octets > 4 ) return (0);
				saw_digit = 1;
			}
		} else if (ch == '.' && saw_digit) {
			if (octets == 4)
		   		return (0);
			*++tp = 0;
			saw_digit = 0;
	 	} else {
			return (0);
		}
	}
 
	if (octets < 4)
		return (0);
 
   memcpy (dst, tmp, NS_INADDRSZ);

   return (1);

}

static int inet_pton6( const char *src, unsigned char * dst )
{
	static const char xdigits_l[] = "0123456789abcdef", xdigits_u[] = "0123456789ABCDEF";
	unsigned char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
	const char *xdigits, *curtok;
	int ch, saw_xdigit;
	unsigned int val;

	memset((tp = tmp), '\0', NS_IN6ADDRSZ);
	endp = tp + NS_IN6ADDRSZ;
	colonp = NULL;

 	if (*src == ':' && *++src != ':')
		return (0);

	curtok = src;
	saw_xdigit = 0;
	val = 0;
	while ((ch = *src++ ) != '\0') {
		const char *pch;

		if ((pch = strchr(( xdigits = xdigits_l ), ch )) == NULL)
			pch = strchr((xdigits = xdigits_u), ch);

	    if (pch != NULL) {
			val <<= 4;
			val |= (pch - xdigits);
			if( val > 0xffff ) return (0);
			saw_xdigit = 1;
			continue;
		}

		if (ch == ':') {
			curtok = src;
			if (!saw_xdigit) {
				if (colonp)
					return (0);
				colonp = tp;
				continue;
			}

		if (tp + NS_INT16SZ > endp)
			return (0);

		*tp++ = (unsigned char)(val >> 8) & 0xff;
		*tp++ = (unsigned char)val & 0xff;
		saw_xdigit = 0;
		val = 0;
		continue;
	}

    if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) && inet_pton4(curtok, tp) > 0) {
		tp += NS_INADDRSZ;
		saw_xdigit = 0;
		break;  /* '\0' was seen by inet_pton4(). */
    }
    return (0);
  }

	if (saw_xdigit) {
		if (tp + NS_INT16SZ > endp)
			return (0);

		*tp++ = (unsigned char)(val >> 8) & 0xff;
		*tp++ = (unsigned char)val & 0xff;
	  }

	if (colonp != NULL)
	{
		const int n = (int)(tp - colonp);
		int i;

		for (i = 1; i <= n; i++) {
			endp[- i] = colonp[n - i];
			colonp[n - i] = 0;
		}
		tp = endp;
	}

	if (tp != endp)
		return (0);
  
	memcpy(dst, tmp, NS_IN6ADDRSZ);
	return (1);
}

static int inet_pton(int af, const char *src, void *dst)
{
	switch (af) {
		case AF_INET:
			return inet_pton4(src, (unsigned char *)dst);

// 		case AF_INET6:
// 			return inet_pton6(src, (unsigned char *)dst );
		default:
			errno = EAFNOSUPPORT;
			return (-1);
	}
}
#endif // not used


static char *restpacketfilltbuf;

int restpacket_create(REST_FUNC_MOD cmdtype, char *packet_buf)
{
	if (!packet_buf || (cmdtype >= REST_FUNC_MAX))
		return -1;

	restpacketfilltbuf = packet_buf;
	sprintf(restpacketfilltbuf, "%s", Greennet_Url[cmdtype]);
	return 0;
}

int restpacket_add_item(CWS_CMDS cmd, char *val)
{
	if ((cmd == CWS_SERIAL) || (cmd == CWS_MAC)) {
		if (val) {
			strcat(restpacketfilltbuf,"&");
			strcat(restpacketfilltbuf, Greennet_Cmds[cmd]);
			strcat(restpacketfilltbuf, "=");
			strcat(restpacketfilltbuf, val);
			return 0;
		}
		else
			return -1;
	}

	if (strstr(restpacketfilltbuf, Greennet_Url[READ_FUNC])) {
//		cmdtype = READ_FUNC;
		strcat(restpacketfilltbuf,"&");
		strcat(restpacketfilltbuf, Greennet_Cmds[cmd]);

	} else if (strstr(restpacketfilltbuf, Greennet_Url[WRITE_FUNC])) {
//		cmdtype = WRITE_FUNC;
		strcat(restpacketfilltbuf,"&");
		strcat(restpacketfilltbuf, Greennet_Cmds[cmd]);
		strcat(restpacketfilltbuf, "=");
		strcat(restpacketfilltbuf, val);
	} else if (strstr(restpacketfilltbuf, Greennet_Url[WRITEIM_FUNC])) {
//		cmdtype = WRITEIM_FUNC;
		strcat(restpacketfilltbuf,"&");
		strcat(restpacketfilltbuf, Greennet_Cmds[cmd]);
		strcat(restpacketfilltbuf, "=");
		strcat(restpacketfilltbuf, val);
	} else if (strstr(restpacketfilltbuf, Greennet_Url[VIEW_FUNC])) {
//		cmdtype = VIEW_FUNC;
		// TODO: aboves are testing for aaf, not defined for hvac
	} else
		return -1;

	return 0;
}

int restpacket_close(void)
{
	strcat(restpacketfilltbuf, "\r\n\r\n");
	return 0;
}


static int find_cmd_val(char *respstr, CWS_CMDS cmd, char *val, bool func_type)
{
	char *pstr;

	/* response of read fuction */
	if (func_type == false) {
		pstr = strstr(respstr, Greennet_Cmds[cmd]);
		if (pstr) {
			pstr += strlen(Greennet_Cmds[cmd]);
			if (*pstr++ == '=') {
				for (; !((*pstr == '\0') || (*pstr == '&')) ;) {
					//printf("debug %c\r\n",*pstr);
					*val++ = *pstr++;
				}
				*val = '\0';
			} else
				return -1;
		} else
			return -1;
	/* response of write function */
	} else if (func_type == true) {
		if (cmd == CWS_STATUS) {
			if (strstr(respstr, "Good")) {
				*val = '1';
			} else if (strstr(respstr, "Warning")) {
				*val = '2';
			} else if (strstr(respstr, "Fault")) {
				*val = '3';
			} else {
				return -1;
			}
		} else if (cmd == CWS_WARNING) {
			if (strstr(respstr, "No Warning")) {
				*val = '1';
			} else if (strstr(respstr, "Battery Power Low")) {
				*val = '2';
			} else if (strstr(respstr, "Filter Media Low")) {
				*val = '3';
			} else {
				return -1;
			}
		} else if (cmd == CWS_FAULT) {
			if (strstr(respstr, "No Fault")) {
				*val = '1';
			} else if (strstr(respstr, "Loss of Communications")) {
				*val = '2';
			} else if (strstr(respstr, "Battery Requires Change")) {
				*val = '3';
			} else if (strstr(respstr, "Filter Media Not Advancing")) {
				*val = '4';
			} else {
				return -1;
			}
		} else if (cmd == CWS_ROLLPOS) {
			pstr = strstr(respstr, "Roll Position");
			if (pstr) {
				pstr += strlen("Roll Position") + 1/*':'*/;
				*val++ = *pstr++;
				*val++ = *pstr++;
				*val = '\0';
			} else
				return -1;

		} else {
			return -1;
		}

	} else
		return -1;

	return 0;
}


int parse_response(char *respstr, struct cws_control_info *pstatus)
{
	CWS_CMDS cmd = 0; // == CWS_SERIAL;
	char string[15];
	int ret = 0;
	bool respfunc;


	/* find response funciton */
	if (strstr(respstr, "Write Function"))
		respfunc = true;

	/* extract information from the received string */
	for (cmd = 0; cmd < CWS_MAX_CMD; cmd++) {
		memset(string, 0, sizeof(string));
		if (find_cmd_val(respstr, cmd, string, respfunc) == 0) {
			switch (cmd) {
				case CWS_SERIAL:
					strcpy(pstatus->serial_val, string);
					break;

				case CWS_MAC:
					strcpy(pstatus->mac_val, string);
					break;

				case CWS_COMMFREQ:
					pstatus->comm_freq_val = atoi(string);
					break;

				case CWS_SCHEDULE:
					pstatus->schedule_val = atoi(string);
					break;

				case CWS_STATUS:
					pstatus->status_val = atoi(string);
					break;

				case CWS_WARNING:
					pstatus->warning_val = atoi(string);
					break;

				case CWS_FAULT:
					pstatus->fault_val = atoi(string);
					break;

				case CWS_ROLLPOS:
					pstatus->poll_pos_val = atoi(string);
					break;

				default:

					ret = -1;
					break;

			}
		} else
			ret = -1;
	}

	return ret;
}


char s_response_r[MAX_RESP_BUF_SIZE];
char s_response_w[MAX_RESP_BUF_SIZE];


static int gnet_aaf_init(struct device *pd)
{
	/* test parameter for greennet access */
	strcpy(pd->mac, "00409D74D237");
	strcpy(pd->serial, "AAG10022001");
	
	/* default value */
	pd->hvac.msec_to_write = 60 * 1000;	/* 60-sec */
	pd->hvac.msec_to_read = 60 * 1000;	/* 60-min or 30-min*/
	
	pd->read_response = s_response_r;
	pd->write_response = s_response_w;
	
	/* simulation */
	pd->hvac.msec_to_read = 10 * 1000;	// 10-sec for simulation
	pd->hvac.msec_to_write = 15 * 1000;	// 10-sec for simulation
	
	return 0;
}
#endif