#ifndef H_TIME
#define H_TIME

#include <barelib.h>

#define TIME_BUFF_SZ 34

typedef enum { GET_SEC, GET_TZ } get_type;

/* Options for rtc ecall request */
typedef struct {
	byte* buffer;
	uint8_t length;
	get_type type;
} rtc_dev_opts;

typedef struct {
	uint8_t mon, week, wday, hh;
} dst_param;

/* POSIX timezone rule string. Must be in the format:
   std[offset][dst[offset][,start[/time],end[/time]]]
   Where dst is optional and /time is optional        */
typedef struct {
	char std[8];         /* Standard identifier of the timezone (e.g. "EST") */
	char dst[8];         /* DST identifier (e.g. "EDT" or "" if none)        */
	int16_t std_off_min; /* Offset in minutes east of UTC                    */
	int16_t dst_off_min; /* Offset in minutes for DST                        */
	dst_param start;     /* Start day and time of DST in POSIX format */
	dst_param end;       /* End day and time of DST                   */
	bool has_dst;
} tzrule;

typedef struct {
	uint16_t year;   /* 4-digit year */
	uint8_t  month;  /* 1-12 */
	uint8_t  day;    /* 1-31 */
	uint8_t  hour;   /* 0-23 */
	uint8_t  minute; /* 0-59 */
	uint8_t  second; /* 0-59 */
} datetime;

datetime seconds_to_dt(uint64_t);
uint64_t dt_to_seconds(datetime);
void dt_to_string(datetime, char*, uint8_t);
uint64_t rtc_read(void);
int8_t rtc_chtz(char*);
tzrule rtc_gettz(void);

#endif
