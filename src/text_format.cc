/**
 * Copyright (c) 2017, Timothy Stack
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
 * @file text_format.cc
 */

#include "text_format.hh"

#include "config.h"
#include "pcrepp/pcrepp.hh"
#include "yajl/api/yajl_parse.h"

text_format_t
detect_text_format(const char* str, size_t len)
{
    // XXX This is a pretty crude way of detecting format...
    static const pcrepp PYTHON_MATCHERS = pcrepp(
        "(?:"
        "^\\s*def\\s+\\w+\\([^)]*\\):[^\\n]*$|"
        "^\\s*try:[^\\n]*$"
        ")",
        PCRE_MULTILINE);

    static const pcrepp RUST_MATCHERS = pcrepp(R"(
(?:
^\s*use\s+[\w+:\{\}]+;$|
^\s*(?:pub)?\s+(?:const|enum|fn)\s+\w+.*$|
^\s*impl\s+\w+.*$
)
)",
                                               PCRE_MULTILINE);

    static const pcrepp JAVA_MATCHERS = pcrepp(
        "(?:"
        "^package\\s+|"
        "^import\\s+|"
        "^\\s*(?:public)?\\s*class\\s*(\\w+\\s+)*\\s*{"
        ")",
        PCRE_MULTILINE);

    static const pcrepp C_LIKE_MATCHERS = pcrepp(
        "(?:"
        "^#\\s*include\\s+|"
        "^#\\s*define\\s+|"
        "^\\s*if\\s+\\([^)]+\\)[^\\n]*$|"
        "^\\s*(?:\\w+\\s+)*class \\w+ {"
        ")",
        PCRE_MULTILINE);

    static const pcrepp SQL_MATCHERS = pcrepp(
        "(?:"
        "select\\s+.+\\s+from\\s+|"
        "insert\\s+into\\s+.+\\s+values"
        ")",
        PCRE_MULTILINE | PCRE_CASELESS);

    static const pcrepp XML_MATCHERS = pcrepp(
        "(?:"
        R"(<\?xml(\s+\w+\s*=\s*"[^"]*")*\?>|)"
        R"(</?\w+(\s+\w+\s*=\s*"[^"]*")*\s*>)"
        ")",
        PCRE_MULTILINE | PCRE_CASELESS);

    text_format_t retval = text_format_t::TF_UNKNOWN;
    pcre_input pi(str, 0, len);
    pcre_context_static<30> pc;

    {
        auto_mem<yajl_handle_t> jhandle(yajl_free);

        jhandle = yajl_alloc(nullptr, nullptr, nullptr);
        if (yajl_parse(jhandle, (unsigned char*) str, len) == yajl_status_ok) {
            return text_format_t::TF_JSON;
        }
    }

    if (PYTHON_MATCHERS.match(pc, pi)) {
        return text_format_t::TF_PYTHON;
    }

    if (RUST_MATCHERS.match(pc, pi)) {
        return text_format_t::TF_RUST;
    }

    if (JAVA_MATCHERS.match(pc, pi)) {
        return text_format_t::TF_JAVA;
    }

    if (C_LIKE_MATCHERS.match(pc, pi)) {
        return text_format_t::TF_C_LIKE;
    }

    if (SQL_MATCHERS.match(pc, pi)) {
        return text_format_t::TF_SQL;
    }

    if (XML_MATCHERS.match(pc, pi)) {
        return text_format_t::TF_XML;
    }

    return retval;
}
