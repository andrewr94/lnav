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
 *
 * @file readline_highlighters.cc
 */

#include "readline_highlighters.hh"

#include "base/string_util.hh"
#include "config.h"
#include "pcrepp/pcrepp.hh"
#include "shlex.hh"
#include "sql_help.hh"
#include "sql_util.hh"

static void readline_sqlite_highlighter_int(attr_line_t& al, int x, int skip);

static bool
check_re_prev(const std::string& line, int x)
{
    bool retval = false;

    if ((x > 0 && line[x - 1] != ')' && line[x - 1] != ']' && line[x - 1] != '*'
         && line[x - 1] != '?' && line[x - 1] != '+')
        && (x < 2 || line[x - 2] != '\\'))
    {
        retval = true;
    }

    return retval;
}

static bool
is_bracket(const std::string& str, int index, bool is_lit)
{
    if (is_lit && str[index - 1] == '\\') {
        return true;
    }
    if (!is_lit && str[index - 1] != '\\') {
        return true;
    }
    return false;
}

static void
find_matching_bracket(attr_line_t& al, int x, char left, char right)
{
    view_colors& vc = view_colors::singleton();
    int matching_bracket_attrs
        = A_BOLD | A_REVERSE | vc.attrs_for_role(view_colors::VCR_OK);
    int missing_bracket_attrs
        = A_BOLD | A_REVERSE | vc.attrs_for_role(view_colors::VCR_ERROR);
    bool is_lit = (left == 'Q');
    const std::string& line = al.get_string();
    int depth = 0;

    if (x < 0 || x > (int) line.length()) {
        return;
    }

    if (line[x] == right && is_bracket(line, x, is_lit)) {
        for (int lpc = x - 1; lpc > 0; lpc--) {
            if (line[lpc] == right && is_bracket(line, lpc, is_lit)) {
                depth += 1;
            } else if (line[lpc] == left && is_bracket(line, lpc, is_lit)) {
                if (depth == 0) {
                    al.get_attrs().emplace_back(
                        line_range(lpc, lpc + 1),
                        view_curses::VC_STYLE.value(matching_bracket_attrs));
                    break;
                } else {
                    depth -= 1;
                }
            }
        }
    }

    if (line[x] == left && is_bracket(line, x, is_lit)) {
        for (size_t lpc = x + 1; lpc < line.length(); lpc++) {
            if (line[lpc] == left && is_bracket(line, lpc, is_lit)) {
                depth += 1;
            } else if (line[lpc] == right && is_bracket(line, lpc, is_lit)) {
                if (depth == 0) {
                    al.get_attrs().emplace_back(
                        line_range(lpc, lpc + 1),
                        view_curses::VC_STYLE.value(matching_bracket_attrs));
                    break;
                } else {
                    depth -= 1;
                }
            }
        }
    }

    int first_left = -1;

    depth = 0;

    for (size_t lpc = 1; lpc < line.length(); lpc++) {
        if (line[lpc] == left && is_bracket(line, lpc, is_lit)) {
            depth += 1;
            if (first_left == -1) {
                first_left = lpc;
            }
        } else if (line[lpc] == right && is_bracket(line, lpc, is_lit)) {
            if (depth > 0) {
                depth -= 1;
            } else {
                al.get_attrs().emplace_back(
                    line_range(is_lit ? lpc - 1 : lpc, lpc + 1),
                    view_curses::VC_STYLE.value(missing_bracket_attrs));
            }
        }
    }

    if (depth > 0) {
        al.get_attrs().emplace_back(
            line_range(is_lit ? first_left - 1 : first_left, first_left + 1),
            view_curses::VC_STYLE.value(missing_bracket_attrs));
    }
}

static char
safe_read(const std::string& str, std::string::size_type index)
{
    if (index < str.length()) {
        return str.at(index);
    }

    return 0;
}

static void
readline_regex_highlighter_int(attr_line_t& al, int x, int skip)
{
    view_colors& vc = view_colors::singleton();
    int special_char
        = (A_BOLD | vc.attrs_for_role(view_colors::VCR_RE_SPECIAL));
    int class_attrs = (A_BOLD | vc.attrs_for_role(view_colors::VCR_SYMBOL));
    int repeated_char_attrs = vc.attrs_for_role(view_colors::VCR_RE_REPEAT);
    int bracket_attrs = vc.attrs_for_role(view_colors::VCR_OK);
    int error_attrs
        = (A_BOLD | A_REVERSE | vc.attrs_for_role(view_colors::VCR_ERROR));

    static const char* brackets[] = {"[]",
                                     "{}",
                                     "()",
                                     "QE",

                                     nullptr};

    auto& line = al.get_string();
    bool backslash_is_quoted = false;

    for (size_t lpc = skip; lpc < line.length(); lpc++) {
        if (line[lpc - 1] != '\\') {
            switch (line[lpc]) {
                case '^':
                case '$':
                case '*':
                case '+':
                case '|':
                case '.':
                    al.get_attrs().emplace_back(
                        line_range(lpc, lpc + 1),
                        view_curses::VC_STYLE.value(special_char));

                    if ((line[lpc] == '*' || line[lpc] == '+')
                        && check_re_prev(line, lpc)) {
                        al.get_attrs().emplace_back(
                            line_range(lpc - 1, lpc),
                            view_curses::VC_STYLE.value(repeated_char_attrs));
                    }
                    break;
                case '?': {
                    struct line_range lr(lpc, lpc + 1);

                    if (line[lpc - 1] == '(') {
                        switch (line[lpc + 1]) {
                            case ':':
                            case '!':
                            case '>':
                            case '<':
                            case '#':
                                lr.lr_end += 1;
                                break;
                        }
                        al.get_attrs().emplace_back(
                            lr, view_curses::VC_STYLE.value(bracket_attrs));
                    } else {
                        al.get_attrs().emplace_back(
                            lr, view_curses::VC_STYLE.value(special_char));

                        if (check_re_prev(line, lpc)) {
                            al.get_attrs().emplace_back(
                                line_range(lpc - 1, lpc),
                                view_curses::VC_STYLE.value(
                                    repeated_char_attrs));
                        }
                    }
                    break;
                }

                case '(':
                case ')':
                case '{':
                case '}':
                case '[':
                case ']':
                    al.get_attrs().emplace_back(
                        line_range(lpc, lpc + 1),
                        view_curses::VC_STYLE.value(bracket_attrs));
                    break;
            }
        }
        if (line[lpc - 1] == '\\') {
            if (backslash_is_quoted) {
                backslash_is_quoted = false;
                continue;
            }
            switch (line[lpc]) {
                case '\\':
                    backslash_is_quoted = true;
                    al.with_attr(
                        string_attr(line_range(lpc - 1, lpc + 1),
                                    view_curses::VC_STYLE.value(special_char)));
                    break;
                case 'd':
                case 'D':
                case 'h':
                case 'H':
                case 'N':
                case 'R':
                case 's':
                case 'S':
                case 'v':
                case 'V':
                case 'w':
                case 'W':
                case 'X':

                case 'A':
                case 'b':
                case 'B':
                case 'G':
                case 'Z':
                case 'z':
                    al.get_attrs().emplace_back(
                        line_range(lpc - 1, lpc + 1),
                        view_curses::VC_STYLE.value(class_attrs));
                    break;
                case ' ':
                    al.get_attrs().emplace_back(
                        line_range(lpc - 1, lpc + 1),
                        view_curses::VC_STYLE.value(error_attrs));
                    break;
                case '0':
                case 'x':
                    if (safe_read(line, lpc + 1) == '{') {
                        al.with_attr(string_attr(
                            line_range(lpc - 1, lpc + 1),
                            view_curses::VC_STYLE.value(special_char)));
                    } else if (isdigit(safe_read(line, lpc + 1))
                               && isdigit(safe_read(line, lpc + 2)))
                    {
                        al.with_attr(string_attr(
                            line_range(lpc - 1, lpc + 3),
                            view_curses::VC_STYLE.value(special_char)));
                    } else {
                        al.with_attr(string_attr(
                            line_range(lpc - 1, lpc + 1),
                            view_curses::VC_STYLE.value(error_attrs)));
                    }
                    break;
                case 'Q':
                case 'E':
                    al.with_attr(string_attr(
                        line_range(lpc - 1, lpc + 1),
                        view_curses::VC_STYLE.value(bracket_attrs)));
                    break;
                default:
                    if (isdigit(line[lpc])) {
                        al.get_attrs().emplace_back(
                            line_range(lpc - 1, lpc + 1),
                            view_curses::VC_STYLE.value(special_char));
                    }
                    break;
            }
        }
    }

    for (int lpc = 0; brackets[lpc]; lpc++) {
        find_matching_bracket(al, x, brackets[lpc][0], brackets[lpc][1]);
    }
}

void
readline_regex_highlighter(attr_line_t& al, int x)
{
    readline_regex_highlighter_int(al, x, 1);
}

void
readline_command_highlighter(attr_line_t& al, int x)
{
    static const pcrepp RE_PREFIXES(
        R"(^:(filter-in|filter-out|delete-filter|enable-filter|disable-filter|highlight|clear-highlight|create-search-table\s+[^\s]+\s+))");
    static const pcrepp SH_PREFIXES(
        "^:(eval|open|append-to|write-to|write-csv-to|write-json-to)");
    static const pcrepp SQL_PREFIXES("^:(filter-expr|mark-expr)");
    static const pcrepp IDENT_PREFIXES("^:(tag|untag|delete-tags)");
    static const pcrepp COLOR_PREFIXES("^:(config)");
    static const pcrepp COLOR_RE("(#(?:[a-fA-F0-9]{6}|[a-fA-F0-9]{3}))");

    view_colors& vc = view_colors::singleton();
    int keyword_attrs = (A_BOLD | vc.attrs_for_role(view_colors::VCR_KEYWORD));

    const auto& line = al.get_string();
    pcre_context_static<30> pc;
    pcre_input pi(line);
    size_t ws_index;

    ws_index = line.find(' ');
    auto command = line.substr(0, ws_index);
    if (ws_index != std::string::npos) {
        al.get_attrs().emplace_back(line_range(1, ws_index),
                                    view_curses::VC_STYLE.value(keyword_attrs));
    }
    if (RE_PREFIXES.match(pc, pi)) {
        readline_regex_highlighter_int(al, x, 1 + pc[0]->length());
    }
    pi.reset(line);
    if (SH_PREFIXES.match(pc, pi)) {
        readline_shlex_highlighter(al, x);
    }
    pi.reset(line);
    if (SQL_PREFIXES.match(pc, pi)) {
        readline_sqlite_highlighter_int(al, x, 1 + pc[0]->length());
    }
    pi.reset(line);
    if (COLOR_PREFIXES.match(pc, pi)) {
        pi.reset(line);
        if (COLOR_RE.match(pc, pi)) {
            pcre_context::capture_t* cap = pc[0];
            auto hash_color = pi.get_substr(cap);

            styling::color_unit::from_str(hash_color)
                .then([&](const auto& rgb_fg) {
                    al.get_attrs().emplace_back(
                        line_range{cap->c_begin, cap->c_begin + 1},
                        view_curses::VC_ROLE.value(
                            view_colors::VCR_COLOR_HINT));
                });
        }
    }
    pi.reset(line);
    if (IDENT_PREFIXES.match(pc, pi) && ws_index != std::string::npos) {
        size_t start = ws_index, last;

        do {
            for (; start < line.length() && isspace(line[start]); start++)
                ;
            for (last = start; last < line.length() && !isspace(line[last]);
                 last++)
                ;
            struct line_range lr {
                (int) start, (int) last
            };

            if (lr.length() > 0 && !lr.contains(x) && !lr.contains(x - 1)) {
                std::string value(lr.substr(line), lr.sublen(line));

                if ((command == ":tag" || command == ":untag"
                     || command == ":delete-tags")
                    && !startswith(value, "#"))
                {
                    value = "#" + value;
                }
                al.get_attrs().emplace_back(
                    lr, view_curses::VC_STYLE.value(vc.attrs_for_ident(value)));
            }

            start = last;
        } while (start < line.length());
    }
}

static void
readline_sqlite_highlighter_int(attr_line_t& al, int x, int skip)
{
    static const auto keyword_re_str = sql_keyword_re();
    static pcrepp keyword_pcre(keyword_re_str.c_str(), PCRE_CASELESS);
    static pcrepp string_literal_pcre("'[^']*('(?:'[^']*')*|$)");
    static pcrepp ident_pcre(
        "(?:\\$|:)?(\\b[a-z_]\\w*)|\"([^\"]+)\"|\\[([^\\]]+)]", PCRE_CASELESS);

    static const char* brackets[] = {"[]",
                                     "()",

                                     nullptr};

    auto& vc = view_colors::singleton();

    int keyword_attrs = vc.attrs_for_role(view_colors::VCR_KEYWORD);
    int symbol_attrs = vc.attrs_for_role(view_colors::VCR_SYMBOL);
    int string_attrs = vc.attrs_for_role(view_colors::VCR_STRING);
    int error_attrs = vc.attrs_for_role(view_colors::VCR_ERROR) | A_REVERSE;

    pcre_context_static<30> pc;
    pcre_input pi(al.get_string(), skip);
    auto& line = al.get_string();

    if (startswith(line, ";.")) {
        auto space = line.find(' ');
        struct line_range lr {
            2, -1
        };

        if (space != std::string::npos) {
            lr.lr_end = space;
        }
        al.get_attrs().emplace_back(lr,
                                    view_curses::VC_STYLE.value(keyword_attrs));
        return;
    }

    while (ident_pcre.match(pc, pi)) {
        pcre_context::capture_t* cap = pc.first_valid();
        int attrs = vc.attrs_for_ident(pi.get_substr_start(cap), cap->length());
        struct line_range lr(cap->c_begin, cap->c_end);

        if (line[cap->c_end] == '(') {
        } else if (!lr.contains(x) && !lr.contains(x - 1)) {
            al.get_attrs().emplace_back(lr, view_curses::VC_STYLE.value(attrs));
        }
    }

    pi.reset(line, skip);

    while (keyword_pcre.match(pc, pi)) {
        pcre_context::capture_t* cap = pc.all();

        al.get_attrs().emplace_back(line_range(cap->c_begin, cap->c_end),
                                    view_curses::VC_STYLE.value(keyword_attrs));
    }

    for (size_t lpc = skip; lpc < line.length(); lpc++) {
        switch (line[lpc]) {
            case '*':
            case '<':
            case '>':
            case '=':
            case '!':
            case '-':
            case '+':
                al.get_attrs().emplace_back(
                    line_range(lpc, lpc + 1),
                    view_curses::VC_STYLE.value(symbol_attrs));
                break;
        }
    }

    pi.reset(line, skip);

    while (string_literal_pcre.match(pc, pi)) {
        pcre_context::capture_t* cap = pc.all();
        struct line_range lr(cap->c_begin, cap->c_end);
        string_attrs_t& sa = al.get_attrs();

        remove_string_attr(sa, lr);

        if (line[cap->c_end - 1] != '\'') {
            sa.emplace_back(line_range(cap->c_begin, cap->c_begin + 1),
                            view_curses::VC_STYLE.value(error_attrs));
            lr.lr_start += 1;
        }
        sa.emplace_back(lr, view_curses::VC_STYLE.value(string_attrs));
    }

    for (int lpc = 0; brackets[lpc]; lpc++) {
        find_matching_bracket(al, x, brackets[lpc][0], brackets[lpc][1]);
    }
}

void
readline_sqlite_highlighter(attr_line_t& al, int x)
{
    readline_sqlite_highlighter_int(al, x, 0);
}

void
readline_shlex_highlighter(attr_line_t& al, int x)
{
    view_colors& vc = view_colors::singleton();
    int special_char = (A_BOLD | vc.attrs_for_role(view_colors::VCR_SYMBOL));
    int error_attrs = vc.attrs_for_role(view_colors::VCR_ERROR) | A_REVERSE;
    int string_attrs = vc.attrs_for_role(view_colors::VCR_STRING);
    const auto& str = al.get_string();
    pcre_context::capture_t cap;
    shlex_token_t token;
    int quote_start = -1;
    shlex lexer(str);

    while (lexer.tokenize(cap, token)) {
        switch (token) {
            case shlex_token_t::ST_ERROR:
                al.with_attr(
                    string_attr(line_range(cap.c_begin, cap.c_end),
                                view_curses::VC_STYLE.value(error_attrs)));
                break;
            case shlex_token_t::ST_TILDE:
            case shlex_token_t::ST_ESCAPE:
                al.with_attr(
                    string_attr(line_range(cap.c_begin, cap.c_end),
                                view_curses::VC_STYLE.value(special_char)));
                break;
            case shlex_token_t::ST_DOUBLE_QUOTE_START:
            case shlex_token_t::ST_SINGLE_QUOTE_START:
                quote_start = cap.c_begin;
                break;
            case shlex_token_t::ST_DOUBLE_QUOTE_END:
            case shlex_token_t::ST_SINGLE_QUOTE_END:
                al.with_attr(
                    string_attr(line_range(quote_start, cap.c_end),
                                view_curses::VC_STYLE.value(string_attrs)));
                quote_start = -1;
                break;
            case shlex_token_t::ST_VARIABLE_REF:
            case shlex_token_t::ST_QUOTED_VARIABLE_REF: {
                int extra = token == shlex_token_t::ST_VARIABLE_REF ? 0 : 1;
                auto ident = str.substr(cap.c_begin + 1 + extra,
                                        cap.length() - 1 - extra * 2);
                int attrs = vc.attrs_for_ident(ident);

                al.with_attr(string_attr(
                    line_range(cap.c_begin, cap.c_begin + 1 + extra),
                    view_curses::VC_STYLE.value(special_char)));
                al.with_attr(string_attr(
                    line_range(cap.c_begin + 1 + extra, cap.c_end - extra),
                    view_curses::VC_STYLE.value(
                        x == cap.c_end || cap.contains(x) ? special_char
                                                          : attrs)));
                if (extra) {
                    al.with_attr(
                        string_attr(line_range(cap.c_end - 1, cap.c_end),
                                    view_curses::VC_STYLE.value(special_char)));
                }
                break;
            }
            case shlex_token_t::ST_WHITESPACE:
                break;
        }
    }

    if (quote_start != -1) {
        al.with_attr(string_attr(line_range(quote_start, quote_start + 1),
                                 view_curses::VC_STYLE.value(error_attrs)));
    }
}
