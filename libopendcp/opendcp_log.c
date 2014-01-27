/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2013 Terrence Meiczinger, All Rights Reserved

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "opendcp.h"

#if _MSC_VER
#define snprintf _snprintf
#endif

#define OPENDCP_LOG_MAX_CALLBACKS 5

typedef struct {
    int level;
    void (*callback)(char *message);
} opendcp_log_callback_t;

static int opendcp_log_callback_count = 0;
static opendcp_log_callback_t opendcp_log_callbacks[OPENDCP_LOG_MAX_CALLBACKS];

void opendcp_log_init(int level);
char *opendcp_log_timestamp();

void opendcp_log_print_message(const char *msg) {
    fprintf(stdout, "%s\n", msg);
}

void opendcp_log(int level, const char *file, const char *function, int line,  const char *fmt, ...) {
    char msg[255];
    va_list vl;
    va_start(vl, fmt);
    int x;

    snprintf(msg, sizeof(msg), "%s | %5s | %-30s | %-5.5d | ", opendcp_log_timestamp(), OPENDCP_LOGLEVEL_NAME[level], function, line);
    vsnprintf(msg + strlen(msg), sizeof(msg) - strlen(msg), fmt, vl);
    va_end(vl);

    for (x = 0; x < opendcp_log_callback_count; x++) {
        if (level <= opendcp_log_callbacks[x].level) {
            opendcp_log_callbacks[x].callback(msg);
        }
    }
}

void opendcp_log_register_callback(int level, void *function) {
    if (opendcp_log_callback_count >= OPENDCP_LOG_MAX_CALLBACKS) {
        return;
    }

    opendcp_log_callbacks[opendcp_log_callback_count].level = level;
    opendcp_log_callbacks[opendcp_log_callback_count++].callback = function;

    return;
}

char *opendcp_log_timestamp() {
    time_t time_ptr;
    struct tm *time_struct;
    static char buffer[30];

    time(&time_ptr);
    time_struct = localtime(&time_ptr);
    strftime(buffer, 30, "%Y%m%d%I%M%S", time_struct);

    return buffer;
}

void opendcp_log_init(int level) {
    opendcp_log_register_callback(level, opendcp_log_print_message);
}
