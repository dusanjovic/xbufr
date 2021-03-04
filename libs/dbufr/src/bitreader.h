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

#include <cstddef>
#include <cstdint>
#include <string>

class BitReader
{
public:
    BitReader(const uint8_t* const data,
              const size_t len,
              const size_t off);

    size_t get_pos() const;
    void set_pos(const size_t pos);
    size_t get_remaining_bits() const;

    void skip_bits(const unsigned int bits);
    unsigned int get_int(const unsigned int bits);
    std::string get_string(const unsigned int bits);

private:
    static const size_t octet_width = 8UL;

    const uint8_t* const buffer;
    const size_t length;
    size_t bit_pos;
    const size_t offset;
    const bool debug;

    BitReader(const BitReader&) = delete;
    BitReader& operator=(BitReader const&) = delete;
};
