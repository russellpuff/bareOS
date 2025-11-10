#ifndef H_RTC
#define H_RTC

#include <barelib.h>
#include <dev/time.h>

void init_rtc(void);
uint64_t rtc_read_seconds(void);
datetime rtc_read_datetime(void);
uint8_t change_localtime(const char*);

extern tzrule localtime;

#endif
