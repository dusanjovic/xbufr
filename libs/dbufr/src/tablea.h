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

#include "descriptortablea.h"

#include <vector>

class TableA
{
public:
    TableA() = default;
    void add_descriptor(const DescriptorTableA& desc);
    int get_decriptor(const FXY fxy, DescriptorTableA& desc) const;
    void dump(std::ostream& ostr) const;

    std::vector<DescriptorTableA> m_tablea;

private:
    TableA(const TableA&) = delete;
    TableA& operator=(TableA const&) = delete;
};
