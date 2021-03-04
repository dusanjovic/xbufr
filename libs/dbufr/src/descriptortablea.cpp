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

#include "descriptortablea.h"

#include "string_utils.h"

DescriptorTableA::DescriptorTableA(const int f, const int x, const int /* y */, const std::string& entry, const std::string& description)
    : Descriptor(f, x, string_to_int(entry))
    , m_entry(entry)
{
    set_mnemonic(description.substr(0, 8));
    m_description = description.substr(9, 55);
}

const std::string& DescriptorTableA::entry() const
{
    return m_entry;
}

void DescriptorTableA::set_entry(const std::string& entry)
{
    m_entry = entry;
}
