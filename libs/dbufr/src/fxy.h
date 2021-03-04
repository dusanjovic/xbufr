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

#include "string_utils.h"

#include <cstdint>
#include <iomanip>
#include <sstream>

class FXY
{
public:
    constexpr explicit FXY(uint16_t a)
        : v(a)
    {
    }
    constexpr FXY(const int f, const int x, const int y)
        : v((f << 14) | (x << 8) | y)
    {
    }
    explicit FXY(const std::string& s_fxy)
    {
        if (s_fxy.length() == 6) {
            // FXXYYY
            v = (string_to_int(s_fxy.substr(0, 1)) << 14) | (string_to_int(s_fxy.substr(1, 2)) << 8) | string_to_int(s_fxy.substr(3, 3));
        } else if (s_fxy.length() == 8) {
            // F-XX-YYY
            v = (string_to_int(s_fxy.substr(0, 1)) << 14) | (string_to_int(s_fxy.substr(2, 2)) << 8) | string_to_int(s_fxy.substr(5, 3));
        } else {
            std::ostringstream ostr;
            ostr << "Error in FXY::FXY(const std::string& s_fxy):";
            ostr << " s_fxy.length() != 6 or s_fxy.length() != 8";
            ostr << "|" << s_fxy << "|";
            throw std::runtime_error(ostr.str());
        }
    }
    bool operator==(const FXY& b) const
    {
        return v == b.v;
    }
    bool operator!=(const FXY& b) const
    {
        return v != b.v;
    }
    bool operator<(const FXY& b) const
    {
        return v < b.v;
    }
    constexpr int f() const
    {
        return (v >> 14) & 0x3;
    }
    constexpr int x() const
    {
        return (v >> 8) & 0x3f;
    }
    constexpr int y() const
    {
        return v & 0xff;
    }
    void fxy(int& f, int& x, int& y) const
    {
        f = this->f();
        x = this->x();
        y = this->y();
        return;
    }
    constexpr uint16_t as_int() const
    {
        return v;
    }
    std::string as_str() const
    {
        std::ostringstream strstrm;
        strstrm << std::setfill('0')
                << std::setw(1) << f()
                << std::setw(2) << x()
                << std::setw(3) << y();
        return strstrm.str();
    }

private:
    uint16_t v;
};
