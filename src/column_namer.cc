/**
 * Copyright (c) 2019, Timothy Stack
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
 * @file column_namer.cc
 */

#include <algorithm>

#include "column_namer.hh"

#include "base/lnav_log.hh"
#include "base/string_util.hh"
#include "config.h"
#include "sql_util.hh"

bool
column_namer::existing_name(const std::string& in_name) const
{
    if (std::binary_search(
            std::begin(sql_keywords), std::end(sql_keywords), toupper(in_name)))
    {
        return true;
    }

    if (std::find(this->cn_builtin_names.begin(),
                  this->cn_builtin_names.end(),
                  in_name)
        != this->cn_builtin_names.end())
    {
        return true;
    }

    if (std::find(this->cn_names.begin(), this->cn_names.end(), in_name)
        != this->cn_names.end())
    {
        return true;
    }

    return false;
}

std::string
column_namer::add_column(const std::string& in_name)
{
    auto base_name = in_name;
    std::string retval;
    int num = 0;

    if (in_name.empty()) {
        base_name = "col";
    }

    retval = base_name;

    auto counter_iter = this->cn_name_counters.find(retval);
    if (counter_iter != this->cn_name_counters.end()) {
        num = ++counter_iter->second;
        retval = fmt::format(FMT_STRING("{}_{}"), base_name, num);
    }

    while (this->existing_name(retval)) {
        if (num == 0) {
            this->cn_name_counters[retval] = num;
        }

        log_debug("column name already exists: %s", retval.c_str());
        retval = fmt::format(FMT_STRING("{}_{}"), base_name, num);
        num += 1;
    }

    this->cn_names.emplace_back(retval);

    return retval;
}
