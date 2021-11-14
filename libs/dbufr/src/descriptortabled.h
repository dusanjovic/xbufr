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

#include <vector>

class DescriptorTableD : public Descriptor
{
public:
    DescriptorTableD() = default;
    DescriptorTableD(const int f, const int x, const int y);
    explicit DescriptorTableD(const FXY fxy);
    explicit DescriptorTableD(const std::string& s_fxy);

    DescriptorTableD(DescriptorTableD const&) = default;
    DescriptorTableD(DescriptorTableD&&) noexcept = default;
    DescriptorTableD& operator=(DescriptorTableD const&) = default;
    DescriptorTableD& operator=(DescriptorTableD&&) noexcept = default;

    const std::vector<Descriptor>& sequence() const;
    void add_child(const Descriptor& d);

private:
    std::vector<Descriptor> m_sequence;
};
