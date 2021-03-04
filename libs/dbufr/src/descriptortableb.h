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

#include "descriptor.h"

class DescriptorTableB : public Descriptor
{
public:
    DescriptorTableB() = default;
    DescriptorTableB(int f,
                     int x,
                     int y,
                     const std::string& mnemonic,
                     const std::string& description,
                     const std::string& unit,
                     int scale,
                     int reference,
                     int bit_width);

    const std::string& unit() const;
    int scale() const;
    int reference() const;
    int bit_width() const;

    bool is_data() const;
    bool is_code() const;
    bool is_flag() const;
    bool is_numeric_data() const;

private:
    std::string m_unit;
    int m_scale{0};
    int m_reference{0};
    int m_bit_width{0};

    bool m_is_code{false};
    bool m_is_flag{false};
    bool m_is_numeric{false};
    bool m_is_data{false};
};
