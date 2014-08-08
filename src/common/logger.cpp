#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stddef.h>

#include "logger.h"

#define LOGGER_BUFFER_SIZE 1024
static int sLevel = LOGGER_LEVEL_DEBUG;


void log(int level, const char *tag, const char *file, \
	const char *function, int line, const char *fmt, ...)
{
	if (level > sLevel) {
		return ;
	}
	
	char buffer[LOGGER_BUFFER_SIZE];
	struct timeval tv;
    struct timezone tz;
    struct tm *tm;
    time_t t;
	int n;
#if 1
    gettimeofday(&tv, &tz);
    time(&t);
    tm = localtime(&t);
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d.%03d", \
		1900 + tm->tm_year, 1 + tm->tm_mon, tm->tm_mday, \
		tm->tm_hour, tm->tm_min, tm->tm_sec, tv.tv_usec / 1000);

	fprintf(stderr, "\n%s\t(%s, %s, %d)\n", buffer, file, function, line);
#endif
	fprintf(stderr, "%s\t", tag);

	va_list ap;
	va_start(ap, fmt);
    vsnprintf(buffer, LOGGER_BUFFER_SIZE, fmt, ap);
    va_end(ap);

	fprintf(stderr, "%s\n", buffer);
}

void setLogLevel(int level)
{
	sLevel = level;
}