#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>

#include "timeutils.h"

static time_t		        _startTime;
static struct tm *		    _localTime;
static uint64_t				_usec;


static char * _getUptime(uint32_t uptimeSeconds) {
	static char szUptime[128];
	uint32_t	t;
	int			days;
	int			hours;
	int			minutes;
	int			seconds;

	t = uptimeSeconds;

	days = t / SECONDS_PER_DAY;
	t = t % SECONDS_PER_DAY;

	hours = t / SECONDS_PER_HOUR;
	t = t % SECONDS_PER_HOUR;

	minutes = t / SECONDS_PER_MINUTE;
	t = t % SECONDS_PER_MINUTE;

	seconds = t;

	sprintf(
		szUptime, 
		"%d %s, %d %s, %d %s, %d %s", 
		days, 
		(days == 1 ? "day" : "days"),
		hours, 
		(hours == 1 ? "hour" : "hours"),
		minutes, 
		(minutes == 1 ? "minute" : "minutes"),
		seconds,
		(seconds == 1 ? "second" : "seconds"));

	return szUptime;
}

void tmUpdate(void) {
	struct timeval		tv;
	time_t				t;

	gettimeofday(&tv, NULL);

	t = time(NULL);

	_usec = tv.tv_usec;
	_localTime = localtime(&t);
}

void tmInitialiseUptimeClock(void) {
	_startTime = time(0);
}

char * tmGetUptime(void) {
	time_t		t;
	uint32_t	seconds;

	t = time(0);

	seconds = (uint32_t)difftime(t, _startTime);

	return _getUptime(seconds);
}

char * tmGetTimeStamp(char * pszBuffer, size_t bufferLen, bool includeMicroseconds) {
	tmUpdate();

	if (includeMicroseconds) {
		snprintf(
			pszBuffer,
            bufferLen,
			"%d-%02d-%02d %02d:%02d:%02d.%06d",
			tmGetYear(),
			tmGetMonth(),
			tmGetDay(),
			tmGetHour(),
			tmGetMinute(),
			tmGetSecond(),
			tmGetMicrosecond());
	}
	else {
		snprintf(
			pszBuffer,
            bufferLen,
			"%d-%02d-%02d %02d:%02d:%02d",
			tmGetYear(),
			tmGetMonth(),
			tmGetDay(),
			tmGetHour(),
			tmGetMinute(),
			tmGetSecond());
	}

	return pszBuffer;
}

char * tmGetSimpleTimeStamp(char * pszBuffer, size_t bufferLen) {
	return tmGetTimeStamp(pszBuffer, bufferLen, false);
}

int tmGetYear(void) {
	return _localTime->tm_year + 1900;
}

int tmGetMonth(void) {
	return _localTime->tm_mon + 1;
}

int tmGetDay(void) {
	return _localTime->tm_mday;
}

int tmGetDayOfWeek(void) {
	return _localTime->tm_wday + 1;
}

int tmGetHour(void) {
	return _localTime->tm_hour;
}

int tmGetMinute(void) {
	return _localTime->tm_min;
}

int tmGetSecond(void) {
	return _localTime->tm_sec;
}

int tmGetMicrosecond(void) {
	return _usec;
}
