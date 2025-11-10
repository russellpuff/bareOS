#include <lib/string.h>
#include <lib/bareio.h>
#include <device/rtc.h>
#include <mm/vm.h>
#include <mm/malloc.h>
#include <fs/fs.h>

tzrule localtime;
static const uint8_t month_len[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

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

uint64_t dt_to_seconds(datetime dt) {
	uint64_t days = 0;

	for (uint16_t year = 1970U; year < dt.year; ++year) {
		days += is_leapyear(year) ? 366UL : 365UL;
	}

	for (uint8_t month = 1; month < dt.month; ++month) {
		uint8_t len = month_len[month - 1];
		if (month == 2 && is_leapyear(dt.year)) len = 29;
		days += len;
	}

	days += (uint64_t)(dt.day - 1);

	uint64_t seconds = days * 86400UL;
	seconds += (uint64_t)dt.hour * 3600U;
	seconds += (uint64_t)dt.minute * 60U;
	seconds += (uint64_t)dt.second;

	return seconds;
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

/* Compute the weekday (0 = Sunday) for a Gregorian date. */
static uint8_t calc_weekday(uint16_t year, uint8_t month, uint8_t day) {
	/* Zeller's congruence operates on March = 3 .. February = 14 */
	if (month < 3) {
		month += 12;
		--year;
	}
	uint16_t k = year % 100;
	uint16_t j = year / 100;
	uint8_t h = (uint8_t)((day + (13 * (month + 1)) / 5 + k + (k / 4) + (j / 4) + (5 * j)) % 7);
	/* Convert Zeller's range (0 = Saturday) to 0 = Sunday to match POSIX format */
	return (h + 6) % 7;
}

/* Resolve the day of month for an M.w.d POSIX rule entry. */
static uint8_t resolve_transition_day(uint16_t year, dst_param param) {
	if (param.mon < 1 || param.mon > 12) return 1;
	if (param.wday > 6) param.wday %= 7;

	uint8_t length = month_len[param.mon - 1];
	if (param.mon == 2 && is_leapyear(year)) length = 29;

	uint8_t first = calc_weekday(year, param.mon, 1);
	uint8_t day = 1 + (uint8_t)((7 + param.wday - first) % 7);
	uint8_t week = param.week;
	if (week == 0) week = 1;
	day += (week - 1) * 7;

	if (week >= 5) {
		while (day + 7 <= length) day += 7;
	}

	if (day > length) day -= 7;
	return day;
}

/* Convert a transition specification to its UTC timestamp. */
static uint64_t transition_to_utc(uint16_t year, dst_param param, int32_t offset_min) {
	datetime dt = { 0 };
	dt.year = year;
	dt.month = param.mon;
	dt.day = resolve_transition_day(year, param);
	dt.hour = param.hh;
	dt.minute = 0;
	dt.second = 0;

	/* Local wall time is stored in the rule. Convert to UTC by undoing the offset. */
	int64_t seconds = (int64_t)dt_to_seconds(dt) + ((int64_t)offset_min * 60);
	if (seconds < 0) seconds = 0;
	return (uint64_t)seconds;
}


/* Only returns a single format for now */
void dt_to_string(datetime dt, char* out, uint8_t len) {
	if (len < TIME_BUFF_SZ) return; /* Minimum string size is 34 bytes for this format */

	const char* tz = "UTC";
	uint64_t utc_seconds = dt_to_seconds(dt);
	int32_t offset_min = 0;

	if (localtime.std[0] != '\0') tz = localtime.std;
	offset_min = localtime.std_off_min;

	if (localtime.has_dst) {
		/* Determine whether the current UTC instant is inside the DST window. */
		uint64_t start_utc = transition_to_utc(dt.year, localtime.start, localtime.std_off_min);
		uint64_t end_utc = transition_to_utc(dt.year, localtime.end, localtime.dst_off_min);
		bool in_dst;
		if (start_utc < end_utc) {
			in_dst = utc_seconds >= start_utc && utc_seconds < end_utc;
		}
		else {
			in_dst = utc_seconds >= start_utc || utc_seconds < end_utc;
		}

		if (in_dst) {
			offset_min = localtime.dst_off_min;
			if (localtime.dst[0] != '\0') tz = localtime.dst;
		}
	}

	int64_t shifted = (int64_t)utc_seconds - ((int64_t)offset_min * 60);
	if (shifted < 0) shifted = 0;
	dt = seconds_to_dt((uint64_t)shifted);

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

	ksprintf((byte*)out, "%s %u %u %s:%s:%s %s %s", month, dt.day, dt.year, hr, min, sec, mer, tz);
}

static inline bool is_digit(uint8_t c) { return c >= '0' && c <= '9'; }
static inline bool is_alpha(uint8_t c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/* Parses the second component, the UTC offset 
   Trusts you passed in a valid string for now */
static int32_t parse_minutes(const byte* start, const byte* end) {
	byte* p = (byte*)start;
	uint8_t sign = 1;
	if (*p == '-') sign = -1;

	/* If more than one digit, add a second digit. Never more than two. */
	uint8_t h = *p++ - '0';
	if (p < end && is_digit(*p)) { 
		h = (h * 10) + (*p++ - '0');
	}
	else {
		return h * 60 * sign;
	}

	/* Minutes, if present */
	uint8_t m = 0;
	if (*p++ == ':') {
		m = (p[0] - '0') * 10 + (p[1] - '0');
	}

	return sign * ((h * 60) + m);
}

/* Param for start/end DST is in the format Ma.b.c/d
   Where a is the month, b is week, c is weekday
   /d is optional, it overrides the default 2am time */
static dst_param parse_param(const byte* start, const byte* end) {
	const byte* p = start;
	if (p < end && *p == 'M') ++p;

	dst_param param = { 0 };

	while (p < end && is_digit(*p)) {
		param.mon = (param.mon * 10) + (*p - '0');
		++p;
	}

	if (p < end && *p == '.') ++p;
	while (p < end && is_digit(*p)) {
		param.week = (param.week * 10) + (*p - '0');
		++p;
	}

	if (p < end && *p == '.') ++p;
	while (p < end && is_digit(*p)) {
		param.wday = (param.wday * 10) + (*p - '0');
		++p;
	}

	param.hh = 2;
	if (p < end && *p == '/') {
		++p;
		param.hh = 0;
		while (p < end && is_digit(*p)) {
			param.hh = (param.hh * 10) + (*p - '0');
			++p;
		}
	}

	return param;
}


/* Attempt to change the localtime to a specified tz
   Expected input "EST" or "est" type code 
   Returns 0 on success or 1 on error                */
uint8_t change_localtime(const char* tz) {
	if (tz == NULL) return 1;

	/* Get tzinfo directory */
	uint64_t l = strlen(tz);
	if (l > 7) return 1;
	dirent_t tzinfo;
	if (resolve_dir("/etc/tzinfo", boot_fsd->super.root_dirent, &tzinfo) != 0) return 1;

	/* Open and read the file */
	FILE f;
	if (open(tz, &f, tzinfo) != 0) return 1;
	byte* buffer = malloc(f.inode.size + 1);
	if (buffer == NULL) {
		close(&f);
		return 1;
	}
	read(&f, buffer, f.inode.size);
	close(&f);
	if (f.inode.size == 0) {
		free(buffer);
		return 1;
	}
	buffer[f.inode.size] = '\0';

	tzrule rule;
	memset(&rule, 0, sizeof(tzrule));

	/* Parse 'buffer' into a rule based on POSIX strings. */
	byte* start = buffer;
	byte* end = start;

	/* Get the std code */
	while (*end != '\0' && is_alpha(*end)) ++end;
	uint8_t len = (uint8_t)(end - start);
	if (len >= sizeof(rule.std)) len = sizeof(rule.std) - 1;
	memcpy(rule.std, start, len);
	rule.std[len] = '\0';

	/* Get the UTC offset */
	start = end;
	while (*end != '\0' && !is_alpha(*end) && *end != ',') ++end;
	rule.std_off_min = (int16_t)parse_minutes(start, end);
	rule.dst_off_min = rule.std_off_min;

	/* If there's more, then DST is supported */
	if (*end != '\0') {
		start = end;
		while (*end != '\0' && is_alpha(*end)) ++end;
		len = (uint8_t)(end - start);
		if (len > 0) {
			/* Get dst code */
			if (len >= sizeof(rule.dst)) len = sizeof(rule.dst) - 1;
			memcpy(rule.dst, start, len);
			rule.dst[len] = '\0';
			rule.has_dst = true;
			rule.dst_off_min = rule.std_off_min - 60; /* Default, edge cases not supported */
		}

		/* Get the two dst params */
		if (*end != ',') return 1;
		++end;
		start = end;
		while (*end != '\0' && *end != ',') ++end;
		rule.start = parse_param(start, end);
		if (*end != ',') return 1;
		++end;
		start = end;
		while (*end != '\0') ++end;
		rule.end = parse_param(start, end);
	}

	free(buffer);
	localtime = rule;
	return 0;
}
