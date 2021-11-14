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

#include "bufrutil.h"
#include "bitutils.h"

#include <array>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

std::ios::pos_type seek_bufr(std::ifstream& file, const std::ios::pos_type start_pos, size_t& len_bufr)
{
    std::array<unsigned char, 4> buffer{};
    std::array<unsigned char, 4> buf7777{};

    // look for "BUFR" in the first 1024 bytes starting at start_pos

    std::ios::pos_type pos = start_pos;
    len_bufr = 0;

    file.seekg(0, std::ios::end);
    std::ios::pos_type file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    while (!file.eof() && pos < start_pos + (std::ios::off_type)1024 && pos < file_size - (std::ios::off_type)4) {

        file.seekg(pos, std::ios::beg);
        file.read((char*)&buffer, 4);

        if (buffer[0] == 'B' && buffer[1] == 'U' && buffer[2] == 'F' && buffer[3] == 'R') {

            file.read((char*)&buffer, 4);
            if ((int)buffer[3] == 2 || (int)buffer[3] == 3 || (int)buffer[3] == 4) {

                len_bufr = C3UINT(buffer);

                // minimum number of bits in any BUFR message is 368
                // see "Guide to WMO Table Driven Code Forms"
                if (len_bufr < 46) {
                    throw std::runtime_error("len_bufr < 46");
                }

                if (pos + (std::ios::pos_type)len_bufr <= file_size) {
                    file.seekg(pos + (std::ios::pos_type)(len_bufr - 4));
                    file.read((char*)&buf7777, 4);
                    if (buf7777[0] == '7' && buf7777[1] == '7' && buf7777[2] == '7' && buf7777[3] == '7') {
                        return pos;
                    }
                    throw std::runtime_error("Can not find 7777");
                }
                throw std::runtime_error("EOF before 7777");
            }
            std::ostringstream ostr;
            ostr << "seek_bufr error: unknown bufr edition " << (int)buffer[3];
            throw std::runtime_error(ostr.str());
        }
        if (!file.eof()) {
            pos = pos + (std::ios::off_type)1;
        }
    }
    return -1;
}

void read_bufr(std::ifstream& file, const std::ios::pos_type pos, const size_t len_bufr, uint8_t* buffer)
{
    if (!file.good()) {
        throw std::runtime_error("read_bufr error: !file.good()");
    }

    file.seekg(pos, std::ios::beg);
    if (file.fail()) {
        throw std::runtime_error("read_bufr error: file.fail() after seekg");
    }

    file.read((char*)buffer, len_bufr);
    if (file.fail()) {
        throw std::runtime_error("read_bufr error: file.fail() after read");
    }
}
