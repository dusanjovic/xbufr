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

#include "descriptortabled.h"

#include <string>

DescriptorTableD::DescriptorTableD(const int f, const int x, const int y)
    : Descriptor(f, x, y)
{
}

DescriptorTableD::DescriptorTableD(const FXY fxy)
    : Descriptor(fxy)
{
}

DescriptorTableD::DescriptorTableD(const std::string& s_fxy)
    : Descriptor(s_fxy)
{
}

const std::vector<Descriptor>& DescriptorTableD::sequence() const
{
    return m_sequence;
}

void DescriptorTableD::add_child(const Descriptor& d)
{
    m_sequence.push_back(d);
}
