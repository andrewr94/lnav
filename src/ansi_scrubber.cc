/**
 * Copyright (c) 2013, Timothy Stack
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
 * @file ansi_scrubber.cc
 */

#include <algorithm>

#include "ansi_scrubber.hh"

#include "base/opt_util.hh"
#include "config.h"
#include "pcrepp/pcrepp.hh"
#include "view_curses.hh"

static pcrepp&
ansi_regex()
{
    static pcrepp retval("\x1b\\[([\\d=;\\?]*)([a-zA-Z])");

    return retval;
}

void
scrub_ansi_string(std::string& str, string_attrs_t& sa)
{
    pcre_context_static<60> context;
    pcrepp& regex = ansi_regex();
    pcre_input pi(str);

    replace(str.begin(), str.end(), '\0', ' ');
    while (regex.match(context, pi)) {
        pcre_context::capture_t* caps = context.all();
        struct line_range lr;
        bool has_attrs = false;
        attr_t attrs = 0;
        auto bg = nonstd::optional<int>();
        auto fg = nonstd::optional<int>();
        auto role = nonstd::optional<int>();
        size_t lpc;

        switch (pi.get_substr_start(&caps[2])[0]) {
            case 'm':
                for (lpc = caps[1].c_begin;
                     lpc != std::string::npos && lpc < (size_t) caps[1].c_end;)
                {
                    int ansi_code = 0;

                    if (sscanf(&(str[lpc]), "%d", &ansi_code) == 1) {
                        if (90 <= ansi_code && ansi_code <= 97) {
                            ansi_code -= 60;
                            attrs |= A_STANDOUT;
                        }
                        if (30 <= ansi_code && ansi_code <= 37) {
                            fg = ansi_code - 30;
                        }
                        if (40 <= ansi_code && ansi_code <= 47) {
                            bg = ansi_code - 40;
                        }
                        switch (ansi_code) {
                            case 1:
                                attrs |= A_BOLD;
                                break;

                            case 2:
                                attrs |= A_DIM;
                                break;

                            case 4:
                                attrs |= A_UNDERLINE;
                                break;

                            case 7:
                                attrs |= A_REVERSE;
                                break;
                        }
                    }
                    lpc = str.find(';', lpc);
                    if (lpc != std::string::npos) {
                        lpc += 1;
                    }
                }
                has_attrs = true;
                break;

            case 'C': {
                unsigned int spaces = 0;

                if (sscanf(&(str[caps[1].c_begin]), "%u", &spaces) == 1
                    && spaces > 0) {
                    str.insert(
                        (std::string::size_type) caps[0].c_end, spaces, ' ');
                }
                break;
            }

            case 'H': {
                unsigned int row = 0, spaces = 0;

                if (sscanf(&(str[caps[1].c_begin]), "%u;%u", &row, &spaces) == 2
                    && spaces > 1) {
                    int ispaces = spaces - 1;
                    if (ispaces > caps[0].c_begin) {
                        str.insert((unsigned long) caps[0].c_end,
                                   ispaces - caps[0].c_begin,
                                   ' ');
                    }
                }
                break;
            }

            case 'O': {
                int role_int;

                if (sscanf(&(str[caps[1].c_begin]), "%d", &role_int) == 1) {
                    if (role_int >= 0 && role_int < view_colors::VCR__MAX) {
                        role = role_int;
                        has_attrs = true;
                    }
                }
                break;
            }
        }
        str.erase(str.begin() + caps[0].c_begin, str.begin() + caps[0].c_end);

        if (has_attrs) {
            for (auto rit = sa.rbegin(); rit != sa.rend(); rit++) {
                if (rit->sa_range.lr_end != -1) {
                    break;
                }
                rit->sa_range.lr_end = caps[0].c_begin;
            }
            lr.lr_start = caps[0].c_begin;
            lr.lr_end = -1;
            if (attrs) {
                sa.emplace_back(lr, view_curses::VC_STYLE.value(attrs));
            }
            role | [&lr, &sa](int r) {
                sa.emplace_back(lr, view_curses::VC_ROLE.value(r));
            };
            fg | [&lr, &sa](int color) {
                sa.emplace_back(lr, view_curses::VC_FOREGROUND.value(color));
            };
            bg | [&lr, &sa](int color) {
                sa.emplace_back(lr, view_curses::VC_BACKGROUND.value(color));
            };
        }

        pi.reset(str);
    }
}

void
add_ansi_vars(std::map<std::string, std::string>& vars)
{
    vars["ansi_csi"] = ANSI_CSI;
    vars["ansi_norm"] = ANSI_NORM;
    vars["ansi_bold"] = ANSI_BOLD_START;
    vars["ansi_underline"] = ANSI_UNDERLINE_START;
    vars["ansi_black"] = ANSI_COLOR(COLOR_BLACK);
    vars["ansi_red"] = ANSI_COLOR(COLOR_RED);
    vars["ansi_green"] = ANSI_COLOR(COLOR_GREEN);
    vars["ansi_yellow"] = ANSI_COLOR(COLOR_YELLOW);
    vars["ansi_blue"] = ANSI_COLOR(COLOR_BLUE);
    vars["ansi_magenta"] = ANSI_COLOR(COLOR_MAGENTA);
    vars["ansi_cyan"] = ANSI_COLOR(COLOR_CYAN);
    vars["ansi_white"] = ANSI_COLOR(COLOR_WHITE);
}
