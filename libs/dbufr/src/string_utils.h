/*
  xbufr - bufr file viewer

  Copyright (c) 2015 - present, Dusan Jovic

  This file is part of xbufr.

  xbufr is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  xbufr is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with xbufr.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <algorithm>
#include <bitset>
#include <cassert>
#include <cctype>
#include <functional>
#include <iostream>
#include <list>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class BadConversion : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

inline double string_to_double(std::string const& s)
{
    std::istringstream i(s);
    double x;
    if (!(i >> x)) {
        throw BadConversion("string_to_double(\"" + s + "\")");
    }
    return x;
}

inline int string_to_int(std::string const& s)
{
    std::istringstream i(s);
    int x;
    if (!(i >> x)) {
        throw BadConversion("string_to_int(\"" + s + "\")");
    }
    return x;
}

inline std::string int_to_bitstring(const int i, const int bits)
{
    assert(bits <= 32);
    return std::bitset<32>(i).to_string().substr(32 - bits, 32);
}

static inline std::string to_escaped(std::string const& s)
{
    std::string out;
    for (auto i = s.begin(); i != s.end(); ++i) {
        auto c = *i;
        switch (c) {
        case '\'':
            out += "\'\'";
            break;
        case '\\':
            out += "\\";
            break;
        case '\t':
            out += "\t";
            break;
        case '\r':
            out += "\r";
            break;
        case '\n':
            out += "\n";
            break;
        default:
            out += c;
            break;
        }
    }
    return out;
}

// trim from start
static inline std::string& ltrim(std::string& s)
{
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
    return s;
}

// trim from end
static inline std::string& rtrim(std::string& s)
{
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(),
            s.end());
    return s;
}

// trim from both ends
static inline std::string& trim(std::string& s)
{
    return ltrim(rtrim(s));
}

// trim from start
static inline std::string ltrim(const std::string& str)
{
    std::string s = str;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
                return !std::isspace(ch);
            }));
    return s;
}

// trim from end
static inline std::string rtrim(const std::string& str)
{
    std::string s = str;
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
                return !std::isspace(ch);
            }).base(),
            s.end());
    return s;
}

// trim from both ends
static inline std::string trim(const std::string& s)
{
    return ltrim(rtrim(s));
}

static inline void trim_right(std::string& s, const std::string& t)
{
    s.erase(s.find_last_not_of(t) + 1);
}

static inline void split(const std::string& s, char delim, std::vector<std::string>& v)
{
    size_t i = 0;
    size_t pos = s.find(delim);
    if (pos == std::string::npos) {
        v.push_back(s);
        return;
    }
    while (pos != std::string::npos) {
        v.push_back(s.substr(i, pos - i));
        i = ++pos;
        pos = s.find(delim, pos);

        if (pos == std::string::npos) {
            v.push_back(s.substr(i, s.length()));
        }
    }
}

static inline void split(const std::string& text, const std::string& separators, std::list<std::string, std::allocator<std::string> /* */>& words)
{
    size_t n = text.length();
    size_t start = text.find_first_not_of(separators);

    while (start < n) {
        size_t stop = text.find_first_of(separators, start);
        if (stop > n) {
            stop = n;
        }
        words.push_back(text.substr(start, stop - start));
        start = text.find_first_not_of(separators, stop + 1);
    }
}

static inline std::string get_left_of_delim(std::string const& str, std::string const& delim)
{
    return str.substr(0, str.find(delim));
}

static inline std::string get_right_of_delim(std::string const& str, std::string const& delim)
{
    return str.substr(str.find(delim) + delim.size());
}

static bool both_are_spaces(char lhs, char rhs)
{
    return (lhs == rhs) && (lhs == ' ');
}

static inline std::string collapse_spaces(std::string const& str)
{
    std::string s = str;
    auto new_end = std::unique(s.begin(), s.end(), both_are_spaces);
    s.erase(new_end, s.end());
    return s;
}

inline bool ends_with(std::string const& name, std::string const& suffix)
{
    if (suffix.size() > name.size()) {
        return false;
    }
    return std::equal(suffix.rbegin(), suffix.rend(), name.rbegin());
}

inline int nth_occurrence(const std::string& str, const std::string& findMe, int nth)
{
    size_t pos = 0;
    int cnt = 0;

    while (cnt != nth) {
        pos += 1;
        pos = str.find(findMe, pos);
        if (pos == std::string::npos)
            return -1;
        cnt++;
    }
    return (int)pos;
}

inline std::string erase_all_substr(const std::string& str, const std::string& substr)
{
    std::string newstr = str;
    size_t pos;
    while ((pos = newstr.find(substr)) != std::string::npos) {
        newstr.erase(pos, substr.length());
    }
    return newstr;
}
