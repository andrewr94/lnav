/**
 * Copyright (c) 2020, Timothy Stack
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
 *
 * @file view_helpers.hh
 */

#ifndef lnav_view_helpers_hh
#define lnav_view_helpers_hh

#include "bookmarks.hh"
#include "help_text.hh"
#include "vis_line.hh"

class textview_curses;

/** The different views available. */
typedef enum {
    LNV_LOG,
    LNV_TEXT,
    LNV_HELP,
    LNV_HISTOGRAM,
    LNV_DB,
    LNV_SCHEMA,
    LNV_PRETTY,
    LNV_SPECTRO,

    LNV__MAX
} lnav_view_t;

extern const char* lnav_view_strings[LNV__MAX + 1];

nonstd::optional<lnav_view_t> view_from_string(const char* name);

bool ensure_view(textview_curses* expected_tc);
bool ensure_view(lnav_view_t expected);
bool toggle_view(textview_curses* toggle_tc);
void layout_views();

nonstd::optional<vis_line_t> next_cluster(
    vis_line_t (bookmark_vector<vis_line_t>::*f)(vis_line_t) const,
    const bookmark_type_t* bt,
    vis_line_t top);
bool moveto_cluster(vis_line_t (bookmark_vector<vis_line_t>::*f)(vis_line_t)
                        const,
                    const bookmark_type_t* bt,
                    vis_line_t top);
void previous_cluster(const bookmark_type_t* bt, textview_curses* tc);
vis_line_t search_forward_from(textview_curses* tc);

#endif
