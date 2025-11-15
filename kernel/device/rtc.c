#include <lib/bareio.h>
#include <device/rtc.h>
#include <mm/vm.h>
#include <mm/kmalloc.h>
#include <fs/fs.h>
#include <util/string.h>

/* File contains definitions for kernel-side rtc device interaction 
   uapi functions and declarations are located in time.c/h          */

tzrule localtime;

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

uint64_t rtc_read_seconds(void) {
	uint32_t lo = rtc_getoff(TIME_LOW);
	uint32_t hi = rtc_getoff(TIME_HIGH);
	uint64_t nano = ((uint64_t)hi << 32) | lo;
	return nano / NSEC_PER_SEC;
}

datetime rtc_read_datetime(void) {
	return seconds_to_dt(rtc_read_seconds());
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
	byte* buffer = kmalloc(f.inode.size + 1);
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
