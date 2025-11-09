#include <lib/string.h>
#include <lib/bareio.h>
#include <device/rtc.h>
#include <mm/vm.h>

/* The registers for the Google Goldfish RTC are 32-bit wide    *
 * and separated by low and high offsets. Attempting to read    *
 * them as anything except uin32_t triggers a load access fault */

#define GOLDFISH_RTC_BASE 0x101000UL
#define TIME_LOW          0x0
#define TIME_HIGH         0x4
#define ALARM_LOW         0x8
#define ALARM_HIGH        0xc
#define CLEAR_ALARM       0x14
#define ALARM_STATUS      0x18
#define NSEC_PER_SEC      1000000000UL

/* Helper function reads the register at offset */
static uint32_t rtc_getoff(uint64_t off) { 
	return *((volatile uint32_t*)(PA_TO_KVA(GOLDFISH_RTC_BASE) + off));
}

void init_rtc(void) {
	/* Placeholder in case future setup is needed 
	   This serves as a quick verification that
	   the device is exposed to the kernel        */
	(void)rtc_getoff(TIME_LOW);
}

static bool is_leapyear(uint16_t year) {
	if ((year % 4) != 0) return false;
	if ((year % 100) != 0) return true;
	return (year % 400) == 0;
}

uint64_t rtc_read_seconds(void) {
	uint32_t lo = rtc_getoff(TIME_LOW);
	uint32_t hi = rtc_getoff(TIME_HIGH);
	uint64_t nano = ((uint64_t)hi << 32) | lo;
	return nano / NSEC_PER_SEC;
}

datetime rtc_read_datetime(void) {
	return seconds_to_dt(rtc_read_seconds());
}

datetime seconds_to_dt(uint64_t seconds) {
	datetime dt;
	/* Get total days, remainder is less than 1 day */
	uint64_t days = seconds / 86400UL;
	uint32_t rem = (uint32_t)(seconds % 86400UL);
	/* Get hours, minutes, and seconds from the remainder */
	dt.hour = rem / 3600;
	rem %= 3600;
	dt.minute = rem / 60;
	dt.second = rem % 60;

	/* Figure out what year we are on, adjust for leap years */
	uint16_t year = 1970U; /* Unix epoch = Jan 1, 1970 */
	while (1) {
		uint16_t yr_days = is_leapyear(year) ? 366 : 365;
		if (days < yr_days) break;
		days -= yr_days;
		++year;
	}
	dt.year = year;

	/* Lookup table for month lengths */
	static const uint8_t month_len[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	uint8_t month = 0; /* 0 indexed */
	while (month < 12) {
		uint8_t l = month_len[month];
		if (month == 1 && is_leapyear(year)) l = 29; /* Leap day */
		if (days < l) break; /* Target month found */
		days -= l;
		++month;
	}
	dt.month = ++month;
	dt.day = (uint8_t)(++days);

	return dt;
}

static void num_to_month(uint8_t month, char* out) {
	if (out == NULL) return;
	if (month == 0 || month > 12) {
		out[0] = '\0'; 
		return;
	}
	static const char* const months[] = {
		"January", "February", "March", "April", 
		"May", "June", "July", "August", 
		"September", "October", "November", "December"
	};
	const char* src = months[month - 1];
	memcpy(out, src, strlen(src) + 1);
}

/* Only returns a single format for now */
void dt_to_string(datetime dt, char* out, uint8_t len) {
	if (len < TIME_BUFF_SZ) return; /* Minimum string size is 34 bytes for this format */
	/* Prep month */
	char month[10];
	num_to_month(dt.month, month);

	/* Prep hour*/
	char hr[3];
	bool pm = dt.hour >= 12;
	dt.hour = dt.hour % 12;
	if (dt.hour == 0) dt.hour = 12;
	char* ptr = dt.hour < 10 ? hr + 1 : hr;
	ksprintf((byte*)ptr, "%u", dt.hour);
	if (dt.hour < 10) hr[0] = '0';

	/* Get meridian id */
	char mer[3];
	mer[0] = pm ? 'P' : 'A';
	mer[1] = 'M';

	/* Prep minute */
	char min[3];
	ptr = dt.minute < 10 ? min + 1 : min;
	ksprintf((byte*)ptr, "%u", dt.minute);
	if (dt.minute < 10) min[0] = '0';

	/* Prep second */
	char sec[3];
	ptr = dt.second < 10 ? sec + 1 : sec;
	ksprintf((byte*)ptr, "%u", dt.second);
	if (dt.second < 10) sec[0] = '0';

	/* Null terminate */
	sec[2] = min[2] = mer[2] = hr[2] = '\0';

	/* Timezones not supported yet */
	char* tz = "UTC";

	ksprintf((byte*)out, "%s %u %u %s:%s:%s %s %s", month, dt.day, dt.year, hr, min, sec, mer, tz);
}
