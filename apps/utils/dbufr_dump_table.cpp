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

#include "bufrfile.h"

#include <iostream>
#include <sstream>

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: dbufr_dump_table <bufr_file>" << '\n';
        return 1;
    }

    try {
        const BUFRFile bf(argv[1]);
        if (bf.has_builtin_tables()) {
            bf.dump_tables(std::cout);
        } else {
            std::cout << argv[1] << " is not NCEP bufr file" << '\n';
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return -1;
    } catch (...) {
        std::cerr << "exception" << '\n';
        return -1;
    }
    return 0;
}
