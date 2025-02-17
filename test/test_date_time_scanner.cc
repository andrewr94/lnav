/**
 * Copyright (c) 2014, Timothy Stack
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * * Neither the name of Timothy Stack nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <assert.h>
#include <locale.h>

#include "../src/lnav_util.hh"
#include "base/date_time_scanner.hh"
#include "config.h"

static const char* GOOD_TIMES[] = {
    "2017 May 08 Mon 18:57:57.578",
    "May 01 00:00:01",
    "May 10 12:00:01",
    "2014-02-11 16:12:34",
    "2014-02-11 16:12:34.123",
    "05/18/2018 12:00:53 PM",
    "05/18/2018 12:00:53 AM",
};

static const char* BAD_TIMES[] = {
    "1-2-3 1:2:3",

    "2013-22-01 12:01:22",
    "2013-00-01 12:01:22",

    "@4000000043",
};

int
main(int argc, char* argv[])
{
    setenv("TZ", "UTC", 1);

    for (const auto* good_time : GOOD_TIMES) {
        date_time_scanner dts;
        struct timeval tv;
        struct exttm tm;
        const char* rc;

        rc = dts.scan(good_time, strlen(good_time), nullptr, &tm, tv);
        printf("ret %s %p\n", good_time, rc);
        assert(rc != nullptr);

        char ts[64];

        gmtime_r(&tv.tv_sec, &tm.et_tm);
        dts.ftime(ts, sizeof(ts), nullptr, tm);
        printf("orig %s\n", good_time);
        printf("loop %s\n", ts);
        assert(strcmp(ts, good_time) == 0);
    }

    {
        static const char* OLD_TIME = "05/18/1960 12:00:53 AM";
        date_time_scanner dts;
        struct timeval tv;
        struct exttm tm;

        auto rc = dts.scan(OLD_TIME, strlen(OLD_TIME), nullptr, &tm, tv);
        assert(rc != nullptr);
        char ts[64];
        dts.ftime(ts, sizeof(ts), nullptr, tm);
        assert(strcmp(ts, "05/18/1980 12:00:53 AM") == 0);
    }

    {
        date_time_scanner dts;
        struct timeval tv;

        dts.convert_to_timeval("@40000000433225833b6e1a8c", -1, nullptr, tv);
        assert(tv.tv_sec == 1127359865);
        assert(tv.tv_usec == 997071);

        memset(&tv, 0, sizeof(tv));
        dts.convert_to_timeval("@4000000043322583", -1, nullptr, tv);
        assert(tv.tv_sec == 1127359865);
        assert(tv.tv_usec == 0);
    }

    for (const auto* bad_time : BAD_TIMES) {
        date_time_scanner dts;
        struct timeval tv;
        struct exttm tm;

        printf("Checking bad time: %s\n", bad_time);
        assert(dts.scan(bad_time, strlen(bad_time), nullptr, &tm, tv)
               == nullptr);
    }

    {
        const char* en_date = "Jan  1 12:00:00";
        const char* es_date = " 1/Ene/2014:12:00:00 +0000";
        struct timeval en_tv, es_tv;
        struct exttm en_tm, es_tm;
        date_time_scanner dts;

        if (setlocale(LC_TIME, "es_ES.UTF-8") != nullptr) {
            assert(dts.scan(en_date, strlen(en_date), nullptr, &en_tm, en_tv)
                   != nullptr);
            dts.clear();
            assert(dts.scan(es_date, strlen(es_date), nullptr, &es_tm, es_tv)
                   != nullptr);
        }
    }

    {
        const char* en_date = "Jan  1 12:00:00";
        const char* fr_date = "août 19 11:08:37";
        struct timeval en_tv, fr_tv;
        struct exttm en_tm, fr_tm;
        date_time_scanner dts;

        if (setlocale(LC_TIME, "fr_FR.UTF-8") != nullptr) {
            assert(dts.scan(en_date, strlen(en_date), nullptr, &en_tm, en_tv)
                   != nullptr);
            dts.clear();
            assert(dts.scan(fr_date, strlen(fr_date), nullptr, &fr_tm, fr_tv)
                   != nullptr);
        }
    }

    {
        const char* ts = "22:46:03.471";
        const char* fmt[] = {
            "%H:%M:%S.%L",
            nullptr,
        };
        char buf[64];
        date_time_scanner dts;
        struct exttm tm;
        struct timeval tv;

        const auto* ts_end = dts.scan(ts, strlen(ts), fmt, &tm, tv);
        assert(ts_end - ts == 12);
        auto rc = dts.ftime(buf, sizeof(buf), fmt, tm);
        assert(rc == 12);
        assert(strcmp(ts, buf) == 0);
    }

    {
        const char* epoch_str = "ts 1428721664 ]";
        struct exttm tm;
        off_t off = 0;

        memset(&tm, 0, sizeof(tm));
        bool rc = ptime_fmt("ts %s ]", &tm, epoch_str, off, strlen(epoch_str));
        assert(rc);
        assert(tm2sec(&tm.et_tm) == 1428721664);
    }

    {
        const char* epoch_str = "ts 60150c93 ]";
        struct exttm tm;
        off_t off = 0;

        memset(&tm, 0, sizeof(tm));
        bool rc = ptime_fmt("ts %q ]", &tm, epoch_str, off, strlen(epoch_str));
        assert(rc);
        assert(tm2sec(&tm.et_tm) == 1611992211);

        char buf[32];
        ftime_fmt(buf, sizeof(buf), "ts %q ]", tm);
        assert(strcmp(buf, epoch_str) == 0);
    }
}
