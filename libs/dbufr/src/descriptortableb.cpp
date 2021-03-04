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

#include "descriptortableb.h"
#include "string_utils.h"

#include <algorithm>

DescriptorTableB::DescriptorTableB(int f,
                                   int x,
                                   int y,
                                   const std::string& mnemonic,
                                   const std::string& description,
                                   const std::string& unit,
                                   int scale,
                                   int reference,
                                   int bit_width)
    : Descriptor(f, x, y, mnemonic, description)
    , m_unit(trim(unit))
    , m_scale(scale)
    , m_reference(reference)
    , m_bit_width(bit_width)
{
    m_is_numeric = (m_unit != "CCITT_IA5" && m_unit != "CCITT IA5" && m_unit != "CCITTIA5");

    std::string upcase_unit = m_unit;
    std::transform(upcase_unit.begin(),
                   upcase_unit.end(),
                   upcase_unit.begin(),
                   ::toupper);

    m_is_code = upcase_unit.substr(0, 4) == "CODE";
    m_is_flag = upcase_unit.substr(0, 4) == "FLAG";
    m_is_data = !m_is_code && !m_is_flag;
}

const std::string& DescriptorTableB::unit() const
{
    return m_unit;
}

int DescriptorTableB::scale() const
{
    return m_scale;
}

int DescriptorTableB::reference() const
{
    return m_reference;
}

int DescriptorTableB::bit_width() const
{
    return m_bit_width;
}

bool DescriptorTableB::is_data() const
{
    return m_is_data;
}

bool DescriptorTableB::is_code() const
{
    return m_is_code;
}

bool DescriptorTableB::is_flag() const
{
    return m_is_flag;
}

bool DescriptorTableB::is_numeric_data() const
{
    return m_is_numeric;
}
