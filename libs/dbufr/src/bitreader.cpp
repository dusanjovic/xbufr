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

#include "bitreader.h"

#include "fmt/format.h"

#include <array>
#include <cassert>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <vector>

BitReader::BitReader(const uint8_t* const data, const size_t len, const size_t off)
    : buffer(data)
    , length(data != nullptr ? len : 0)
    , bit_pos(0)
    , offset(off)
    , debug(false)
{
}

size_t BitReader::get_pos() const
{
    return bit_pos;
}

void BitReader::set_pos(const size_t pos)
{
    bit_pos = pos;
}

size_t BitReader::get_remaining_bits() const
{
    return length - bit_pos;
}

void BitReader::skip_bits(const unsigned int bits)
{
    if (debug) {
        std::cerr << "BitReader::skip " << bit_pos + offset << " bits " << bits << '\n';
    }

    if (bit_pos + bits > length) {
        throw std::runtime_error(fmt::format("BitReader::skip can not go past the end of the buffer. cursor_position: {} bits: {} length: {}", bit_pos, bits, length));
    }

    bit_pos += bits;
}

unsigned int BitReader::get_int(const unsigned int bits)
{
    if (debug) {
        std::cerr << "\nBitReader::get_int " << bit_pos + offset << " bits " << bits;
    }

    if (bits <= 0) {
        throw std::runtime_error(fmt::format("BitReader::get_int bits <= 0. bits = {}", bits));
    }

    if (bits > 32) {
        throw std::runtime_error(fmt::format("BitReader::get_int bits > 32. bits = {}", bits));
    }

    if (bit_pos + bits > length) {
        throw std::runtime_error(fmt::format("BitReader::get_int can not go past the end of the buffer. cursor_position: {} bits: {} length: {}", bit_pos, bits, length));
    }

    static const std::array<unsigned int, 33> bitmask = {
        0x00000000U,
        0x00000001U,
        0x00000003U,
        0x00000007U,
        0x0000000fU,
        0x0000001fU,
        0x0000003fU,
        0x0000007fU,
        0x000000ffU,
        0x000001ffU,
        0x000003ffU,
        0x000007ffU,
        0x00000fffU,
        0x00001fffU,
        0x00003fffU,
        0x00007fffU,
        0x0000ffffU,
        0x0001ffffU,
        0x0003ffffU,
        0x0007ffffU,
        0x000fffffU,
        0x001fffffU,
        0x003fffffU,
        0x007fffffU,
        0x00ffffffU,
        0x01ffffffU,
        0x03ffffffU,
        0x07ffffffU,
        0x0fffffffU,
        0x1fffffffU,
        0x3fffffffU,
        0x7fffffffU,
        0xffffffffU};

    //   octet
    // ******** ******** ******** ********
    //    +++++ ++++++++ ++                   bits = 15

    size_t octet = bit_pos >> 3UL;            // Which octet the word starts in, faster than 'bit_pos / 8'
    const size_t startbit = bit_pos & 0x07UL; // Offset from start of octet to start of word, faster than 'bit_pos % 8'

    unsigned int ival = 0;

    if (bits + startbit <= octet_width) {
        // Word to be extracted is within a single octet
        ival = (buffer[octet] >> (octet_width - bits - startbit)) & bitmask[bits];
    } else {
        // Extract bits in first octet
        size_t mybits = octet_width - startbit;
        ival = buffer[octet++] & bitmask[mybits];
        // Extract complete octets
        while (bits - mybits >= octet_width) {
            ival = (ival << octet_width) | buffer[octet++];
            mybits += octet_width;
        }
        // Extract remaining bits
        const size_t lastbits = bits - mybits;
        if (lastbits > 0) {
            ival <<= lastbits;
            ival |= (buffer[octet] >> (octet_width - lastbits)) & bitmask[lastbits];
        }
    }

    bit_pos += bits;

    if (debug) {
        std::cerr << " ival = " << ival << '\n';
    }

    return ival;
}

std::string BitReader::get_string(const unsigned int bits)
{
    if (debug) {
        std::cerr << "BitReader::get_string " << bit_pos + offset << " bits " << bits << '\n';
    }

    if (bits <= 0) {
        throw std::runtime_error("BitReader::get_string bits <= 0");
    }

    if (bits % octet_width != 0) {
        throw std::runtime_error("BitReader::get_string bits not a multiple of 8");
    }

    if (bit_pos + bits > length) {
        throw std::runtime_error(fmt::format("BitReader::get_string can not go past the end of the buffer. cursor_position: {} bits: {} length: {}", bit_pos, bits, length));
    }

    const size_t octet = bit_pos >> 3UL;    // Which octet the word starts in, faster than 'bit_pos / 8'
    const size_t lshift = bit_pos & 0x07UL; // Offset from start of octet to start of word, faster than 'bit_pos % 8'
    const size_t len = bits >> 3UL;         // faster than 'bits / 8';
    auto str = std::vector<unsigned char>(len + 1);

    if (lshift == 0) {
        for (size_t i = 0; i < len; i++) {
            str[i] = buffer[octet + i];
            assert((str[i] & 0x10000000) == 0); // is International Alphabet No. 5
        }
    } else {
        const size_t rshift = octet_width - lshift;
        for (size_t i = 0; i < len; i++) {
            str[i] = (buffer[octet + i] << lshift) | (buffer[octet + i + 1] >> rshift);
            assert((str[i] & 0x10000000) == 0); // is International Alphabet No. 5
        }
    }
    str[len] = '\0';

    bit_pos += bits;

    for (size_t i = 0; i < len; i++) {
        if (str[i] != 0xff) {
            std::string s(str.begin(), str.end() - 1);
            return s;
        }
    }
    // all bits in all octets are 1. it's a missing string
    return std::string("MISSING");
}
