#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "debug.h"

void debug(const char *fmt, ...)
{
	va_list logmsg;
	va_start(logmsg, fmt);

	vsyslog(LOG_DEBUG, fmt, logmsg);

	va_end(logmsg);
}