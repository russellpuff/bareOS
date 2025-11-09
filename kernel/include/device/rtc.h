#ifndef H_RTC
#define H_RTC

#include <lib/barelib.h>

#define TIME_BUFF_SZ 34

/* Functions for using the Goldfish RTC device provided by QEMU */
typedef struct {
	uint16_t year;   /* 4-digit year */
	uint8_t  month;  /* 1-12 */
	uint8_t  day;    /* 1-31 */
	uint8_t  hour;   /* 0-23 */
	uint8_t  minute; /* 0-59 */
	uint8_t  second; /* 0-59 */
} datetime;

void init_rtc(void);
uint64_t rtc_read_seconds(void);
datetime rtc_read_datetime(void);
datetime seconds_to_dt(uint64_t);
void dt_to_string(datetime, char*, uint8_t);

#endif
