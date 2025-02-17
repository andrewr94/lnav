/**
 * Copyright (c) 2007-2012, Timothy Stack
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

#ifndef lnav_top_status_source_hh
#define lnav_top_status_source_hh

#include <string>

#include "listview_curses.hh"
#include "statusview_curses.hh"

class top_status_source : public status_data_source {
public:
    typedef enum {
        TSF_TIME,
        TSF_PARTITION_NAME,
        TSF_VIEW_NAME,
        TSF_STITCH_VIEW_FORMAT,
        TSF_FORMAT,
        TSF_STITCH_FORMAT_FILENAME,
        TSF_FILENAME,

        TSF__MAX
    } field_t;

    top_status_source();

    size_t statusview_fields() override
    {
        return TSF__MAX;
    };

    status_field& statusview_value_for_field(int field) override
    {
        return this->tss_fields[field];
    };

    void update_time(const struct timeval& current_time);

    void update_time();

    void update_filename(listview_curses* lc);

    void update_view_name(listview_curses* lc);

private:
    status_field tss_fields[TSF__MAX];
};

#endif
