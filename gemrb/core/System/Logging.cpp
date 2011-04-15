/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2011 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "System/Logging.h"

#include <cstdio>
#include <cstdarg>

#ifdef ANDROID
#include <android/log.h>
#endif

void print(const char *message, ...)
{
	va_list ap;

	va_start(ap, message);
	int size = vsnprintf(NULL, 0, message, ap);
	va_end(ap);

	if (size < 0)
		return;

	char *buff = new char[size+1];
	va_start(ap, message);
	vsprintf(buff, message, ap);
	va_end(ap);

#if (!defined(WIN32) || defined(__MINGW32__)) && !defined(ANDROID)
	// NOTE: We could do this without the allocation, but this path gets tested more.
	printf("%s", buff);
#elif defined(ANDROID)
	__android_log_print(ANDROID_LOG_INFO, "print:", "%s", message);
#else
	cprintf("%s", message);
#endif
	free(buff);
	va_end(ap);
}