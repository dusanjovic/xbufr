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

#include "fxy.h"

#include <cstdint>
#include <string>

class Descriptor
{
public:
    Descriptor() = default;
    Descriptor(const int f, const int x, const int y, std::string mnemonic = "", std::string description = "");
    explicit Descriptor(const FXY fxy);
    explicit Descriptor(const std::string& s_fxy);
    virtual ~Descriptor() = default;
    Descriptor(Descriptor const&) = default;
    Descriptor(Descriptor&&) noexcept = default;
    Descriptor& operator=(Descriptor const&) = default;
    Descriptor& operator=(Descriptor&&) noexcept = default;

    FXY fxy() const;

    void set_mnemonic(const std::string& mnemonic);
    const std::string& mnemonic() const;

    void set_description(const std::string& description);
    const std::string& description() const;

private:
    FXY m_fxy{0};
    std::string m_mnemonic;
    std::string m_description;
};
