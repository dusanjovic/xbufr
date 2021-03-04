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

#include "descriptor.h"
#include "string_utils.h"

#include <iomanip>
#include <iostream>

Descriptor::Descriptor(const int f, const int x, const int y, std::string mnemonic, std::string description)
    : m_fxy(FXY(f, x, y))
    , m_mnemonic(std::move(mnemonic))
    , m_description(std::move(description))
{
}

Descriptor::Descriptor(const FXY fxy)
    : m_fxy(fxy)
{
}

Descriptor::Descriptor(const std::string& s_fxy)
    : m_fxy(FXY(s_fxy))
{
}

FXY Descriptor::fxy() const
{
    return m_fxy;
}

void Descriptor::set_mnemonic(const std::string& mnemonic)
{
    m_mnemonic = mnemonic;
}

const std::string& Descriptor::mnemonic() const
{
    return m_mnemonic;
}

void Descriptor::set_description(const std::string& description)
{
    m_description = description;
}

const std::string& Descriptor::description() const
{
    return m_description;
}
