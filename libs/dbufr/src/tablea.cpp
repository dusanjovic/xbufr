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

#include "tablea.h"

#include <iomanip>
#include <iostream>

void TableA::add_descriptor(const DescriptorTableA& desc)
{
    m_tablea.push_back(desc);
}

int TableA::get_decriptor(const FXY fxy, DescriptorTableA& desc) const
{
    for (const auto& desc_a : m_tablea) {
        if (fxy == desc_a.fxy()) {
            desc = desc_a;
            return 1;
        }
    }
    return 0;
}

void TableA::dump(std::ostream& ostr) const
{
    (void)m_tablea; // to prevent clang-tidy to convet this to static method

    ostr << ".------------------------------------------------------------------------------." << '\n';
    ostr << "| ------------   USER DEFINITIONS FOR TABLE-A TABLE-B TABLE D   -------------- |" << '\n';
    ostr << "|------------------------------------------------------------------------------|" << '\n';
    ostr << "| MNEMONIC | NUMBER | DESCRIPTION                                              |" << '\n';
    ostr << "|----------|--------|----------------------------------------------------------|" << '\n';
    /*
    ostr << "|          |        |                                                          |" << '\n';
    for (const auto& a_entry : m_tablea) {
        int f, x, y;
        a_entry.fxy().fxy(f, x, y);
        ostr << std::setfill(' ') << "| " << std::setw(8) << a_entry.mnemonic()
             << " | " << std::setfill('0') << std::setw(1) << "A" << std::setw(2) << x << std::setw(3) << y << std::setfill(' ')
             << " | " << std::setw(55) << a_entry.description()
             << "  | "
             << '\n';
    }
    */
}
