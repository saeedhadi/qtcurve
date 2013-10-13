/***************************************************************************
 *   Copyright (C) 2013~2013 by Yichao Yu                                  *
 *   yyc1992@gmail.com                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.              *
 ***************************************************************************/

#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>

static QtcLogLevel log_level = QTC_LOG_ERROR;
static bool output_color = false;

static inline bool
_qtcEnvTrue(const char *env)
{
    if (env && *env &&
        (strcasecmp(env, "1") == 0 || strcasecmp(env, "t") == 0 ||
         strcasecmp(env, "true") == 0 || strcasecmp(env, "on") == 0)) {
        return true;
    }
    return false;
}

static inline bool
_qtcEnvFalse(const char *env)
{
    if (env && *env &&
        (strcasecmp(env, "0") == 0 || strcasecmp(env, "f") == 0 ||
         strcasecmp(env, "false") == 0 || strcasecmp(env, "off") == 0)) {
        return true;
    }
    return false;
}

static inline void
_qtcCheckLogLevelReal()
{
    const char *env_debug = getenv("QTCURVE_DEBUG");
    if (_qtcEnvTrue(env_debug)) {
        log_level = QTC_LOG_DEBUG;
        return;
    }
    const char *env_level = getenv("QTCURVE_LEVEL");
    if (env_level && *env_level) {
        if (strcasecmp(env_level, "debug") == 0) {
            log_level = QTC_LOG_DEBUG;
        } else if (strcasecmp(env_level, "info") == 0) {
            log_level = QTC_LOG_INFO;
        } else if (strcasecmp(env_level, "warning") == 0 ||
                   strcasecmp(env_level, "warn") == 0) {
            log_level = QTC_LOG_WARN;
        } else if (strcasecmp(env_level, "error") == 0) {
            log_level = QTC_LOG_ERROR;
        }
    }
    if (_qtcEnvFalse(env_debug) && log_level <= QTC_LOG_DEBUG) {
        log_level = QTC_LOG_INFO;
    }
}

static inline void
_qtcCheckLogColorReal()
{
    const char *env_color = getenv("QTCURVE_LOG_COLOR");
    if (_qtcEnvTrue(env_color)) {
        output_color = true;
    } else if (_qtcEnvFalse(env_color)) {
        output_color = false;
    } else if (isatty(2)) {
        output_color = true;
    } else {
        output_color = false;
    }
}

static inline void
_qtcLogInit()
{
    static bool log_inited = false;
    if (qtcUnlikely(!log_inited)) {
        _qtcCheckLogLevelReal();
        _qtcCheckLogColorReal();
        log_inited = true;
    }
}

QTC_EXPORT QtcLogLevel
_qtcCheckLogLevel()
{
    _qtcLogInit();
    return log_level;
}

QTC_EXPORT bool
_qtcCheckLogColor()
{
    _qtcLogInit();
    return output_color;
}

QTC_EXPORT void
_qtcLogV(QtcLogLevel level, const char *fname, int line, const char *func,
         const char *fmt, va_list ap)
{
    _qtcLogInit();
    if (qtcUnlikely(level < log_level || level < 0 || level > QTC_LOG_ERROR))
        return;

    static const char *color_codes[] = {
        [QTC_LOG_DEBUG] = "\e[01;32m",
        [QTC_LOG_INFO] = "\e[01;34m",
        [QTC_LOG_WARN] = "\e[01;33m",
        [QTC_LOG_ERROR] = "\e[01;31m",
    };

    static const char *log_prefixes[] = {
        [QTC_LOG_DEBUG] = "qtcDebug-",
        [QTC_LOG_INFO] = "qtcInfo-",
        [QTC_LOG_WARN] = "qtcWarn-",
        [QTC_LOG_ERROR] = "qtcError-",
    };

    const char *color_prefix =
        output_color ? color_prefix = color_codes[(int)level] : "";
    const char *log_prefix = log_prefixes[(int)level];

    fprintf(stderr, "%s%s%d (%s:%d) %s ", color_prefix, log_prefix, getpid(),
            fname, line, func);
    vfprintf(stderr, fmt, ap);
    if (output_color) {
        fwrite("\e[0m", strlen("\e[0m"), 1, stderr);
    }
}

QTC_EXPORT void
_qtcLog(QtcLogLevel level, const char *fname, int line, const char *func,
        const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    _qtcLogV(level, fname, line, func, fmt, ap);
    va_end(ap);
}