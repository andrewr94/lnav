/**
 * Copyright (c) 2015, Timothy Stack
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

#include "pretty_printer.hh"

#include "base/string_util.hh"
#include "config.h"

void
pretty_printer::append_to(attr_line_t& al)
{
    pcre_context_static<30> pc;
    data_token_t dt;

    this->pp_scanner->reset();
    while (this->pp_scanner->tokenize2(pc, dt)) {
        element el(dt, pc);

        switch (dt) {
            case DT_XML_EMPTY_TAG:
                if (this->pp_is_xml && this->pp_line_length > 0) {
                    this->start_new_line();
                }
                this->pp_values.emplace_back(el);
                if (this->pp_is_xml) {
                    this->start_new_line();
                }
                continue;
            case DT_XML_OPEN_TAG:
                if (this->pp_is_xml) {
                    this->start_new_line();
                    this->write_element(el);
                    this->descend();
                } else {
                    this->pp_values.emplace_back(el);
                }
                continue;
            case DT_XML_CLOSE_TAG:
                this->flush_values();
                this->ascend();
                this->write_element(el);
                this->start_new_line();
                continue;
            case DT_LCURLY:
            case DT_LSQUARE:
            case DT_LPAREN:
                this->flush_values(true);
                this->pp_values.emplace_back(el);
                this->descend();
                continue;
            case DT_RCURLY:
            case DT_RSQUARE:
            case DT_RPAREN:
                this->flush_values();
                if (this->pp_body_lines.top()) {
                    this->start_new_line();
                }
                this->ascend();
                this->write_element(el);
                continue;
            case DT_COMMA:
                if (this->pp_depth > 0) {
                    this->flush_values(true);
                    this->write_element(el);
                    this->start_new_line();
                    continue;
                }
                break;
            case DT_WHITE:
                if (this->pp_values.empty() && this->pp_depth == 0
                    && this->pp_line_length == 0) {
                    this->pp_leading_indent = el.e_capture.length();
                    continue;
                }
                break;
            default:
                break;
        }
        this->pp_values.emplace_back(el);
    }
    while (this->pp_depth > 0) {
        this->ascend();
    }
    this->flush_values();

    attr_line_t combined;
    combined.get_string() = this->pp_stream.str();
    combined.get_attrs() = this->pp_attrs;

    if (!al.empty()) {
        al.append("\n");
    }
    al.append(combined);
}

void
pretty_printer::write_element(const pretty_printer::element& el)
{
    if (this->pp_leading_indent == 0 && this->pp_line_length == 0
        && el.e_token == DT_WHITE)
    {
        if (this->pp_depth == 0) {
            this->pp_soft_indent += el.e_capture.length();
        }
        return;
    }
    if (((this->pp_leading_indent == 0)
         || (this->pp_line_length <= this->pp_leading_indent))
        && el.e_token == DT_LINE)
    {
        this->pp_soft_indent = 0;
        if (this->pp_line_length > 0) {
            this->pp_line_length = 0;
            this->pp_stream << std::endl;
            this->pp_body_lines.top() += 1;
        }
        return;
    }
    pcre_input& pi = this->pp_scanner->get_input();
    if (this->pp_line_length == 0) {
        this->append_indent();
    }
    ssize_t start_size = this->pp_stream.tellp();
    if (el.e_token == DT_QUOTED_STRING) {
        auto_mem<char> unquoted_str((char*) malloc(el.e_capture.length() + 1));
        const char* start = pi.get_substr_start(&el.e_capture);
        unquote(unquoted_str.in(), start, el.e_capture.length());
        data_scanner ds(unquoted_str.in());
        string_attrs_t sa;
        pretty_printer str_pp(
            &ds, sa, this->pp_leading_indent + this->pp_depth * 4);
        attr_line_t result;
        str_pp.append_to(result);
        if (result.get_string().find('\n') != std::string::npos) {
            switch (start[0]) {
                case 'r':
                case 'u':
                    this->pp_stream << start[0];
                    this->pp_stream << start[1] << start[1];
                    break;
                default:
                    this->pp_stream << start[0] << start[0];
                    break;
            }
            this->pp_stream << std::endl << result.get_string();
            if (result.empty() || result.get_string().back() != '\n') {
                this->pp_stream << std::endl;
            }
            this->pp_stream << start[el.e_capture.length() - 1]
                            << start[el.e_capture.length() - 1];
        } else {
            this->pp_stream << pi.get_substr(&el.e_capture);
        }
    } else {
        this->pp_stream << pi.get_substr(&el.e_capture);
        int shift_amount
            = start_size - el.e_capture.c_begin - this->pp_shift_accum;
        shift_string_attrs(this->pp_attrs, el.e_capture.c_begin, shift_amount);
        this->pp_shift_accum = start_size - el.e_capture.c_begin;
    }
    this->pp_line_length += el.e_capture.length();
    if (el.e_token == DT_LINE) {
        this->pp_line_length = 0;
        this->pp_body_lines.top() += 1;
    }
}

void
pretty_printer::append_indent()
{
    this->pp_stream << std::string(
        this->pp_leading_indent + this->pp_soft_indent, ' ');
    this->pp_soft_indent = 0;
    if (this->pp_stream.tellp() == this->pp_leading_indent) {
        return;
    }
    for (int lpc = 0; lpc < this->pp_depth; lpc++) {
        this->pp_stream << "    ";
    }
}

bool
pretty_printer::flush_values(bool start_on_depth)
{
    bool retval = false;

    while (!this->pp_values.empty()) {
        {
            element& el = this->pp_values.front();
            this->write_element(this->pp_values.front());
            if (start_on_depth
                && (el.e_token == DT_LSQUARE || el.e_token == DT_LCURLY)) {
                if (this->pp_line_length > 0) {
                    this->pp_stream << std::endl;
                }
                this->pp_line_length = 0;
            }
        }
        this->pp_values.pop_front();
        retval = true;
    }
    return retval;
}

void
pretty_printer::start_new_line()
{
    bool has_output;

    if (this->pp_line_length > 0) {
        this->pp_stream << std::endl;
        this->pp_line_length = 0;
    }
    has_output = this->flush_values();
    if (has_output && this->pp_line_length > 0) {
        this->pp_stream << std::endl;
    }
    this->pp_line_length = 0;
    this->pp_body_lines.top() += 1;
}

void
pretty_printer::ascend()
{
    if (this->pp_depth > 0) {
        int lines = this->pp_body_lines.top();
        this->pp_depth -= 1;
        this->pp_body_lines.pop();
        this->pp_body_lines.top() += lines;
    } else {
        this->pp_body_lines.top() = 0;
    }
}

void
pretty_printer::descend()
{
    this->pp_depth += 1;
    this->pp_body_lines.push(0);
}
