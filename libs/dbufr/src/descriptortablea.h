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

class DescriptorTableA : public Descriptor
{
public:
    DescriptorTableA(const int f, const int x, const int y, const std::string& entry, const std::string& description);

    DescriptorTableA(DescriptorTableA const&) = default;
    DescriptorTableA(DescriptorTableA&&) noexcept = default;
    DescriptorTableA& operator=(DescriptorTableA const&) = default;
    DescriptorTableA& operator=(DescriptorTableA&&) noexcept = default;
};
