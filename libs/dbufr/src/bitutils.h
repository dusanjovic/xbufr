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

#include <array>
#include <bitset>
#include <cassert>
#include <cstdint>

static const std::array<unsigned int, 33> bitmask = {
    0U,
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

// 4 bytes conversion
inline int C4INT(const unsigned char* buf)
{
    return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}
inline void INT4C(const int i, unsigned char* buf)
{
    buf[0] = (i >> 24) & 255;
    buf[1] = (i >> 16) & 255;
    buf[2] = (i >> 8) & 255;
    buf[3] = i & 255;
}

// 3 bytes conversion
inline int C3INT(const unsigned char* buf)
{
    return (buf[0] << 16) + (buf[1] << 8) + buf[2];
}
inline unsigned int C3UINT(const unsigned char* buf)
{
    return (buf[0] << 16) + (buf[1] << 8) + buf[2];
}
template <std::size_t SIZE>
inline unsigned int C3UINT(const std::array<unsigned char, SIZE> buf)
{
    assert(3 <= SIZE);
    return (buf[0] << 16) + (buf[1] << 8) + buf[2];
}
inline void INT3C(const int i, unsigned char* buf)
{
    buf[0] = (i >> 16) & 255;
    buf[1] = (i >> 8) & 255;
    buf[2] = i & 255;
}

// 2 bytes conversion
inline int C2INT(const unsigned char* buf)
{
    return (buf[0] << 8) + buf[1];
}
inline unsigned int C2UINT(const unsigned char* buf)
{
    return (buf[0] << 8) + buf[1];
}
inline void INT2C(const int i, unsigned char* buf)
{
    buf[0] = (i >> 8) & 255;
    buf[1] = i & 255;
}
inline short C2SHORT(const unsigned char* buf)
{
    return (short)((buf[0] << 8) + buf[1]);
}

// 1 byte conversion
inline int C1INT(const unsigned char* buf)
{
    return (int)(buf[0]);
}
inline unsigned int C1UINT(const unsigned char* buf)
{
    return (unsigned int)(buf[0]);
}
inline void INT1C(const int i, unsigned char* buf)
{
    buf[0] = (unsigned char)i;
}

inline int INT2(const unsigned char a, const unsigned char b)
{
    return ((1 - (int)((unsigned)(a & 0x80) >> 6)) * ((a & 127) << 8) + b);
}
inline void INT2S(const int i, unsigned char* buf)
{
    const unsigned int k = (i >= 0) ? i : (-i) | (1U << 15);
    buf[0] = (k >> 8) & 255;
    buf[1] = k & 255;
}

inline int INT3(const unsigned char a, const unsigned char b, const unsigned char c)
{
    return ((1 - (int)((unsigned)(a & 0x80) >> 6)) * ((a & 127) << 16) + (b << 8) + c);
}
inline void INT3S(const int i, unsigned char* buf)
{
    const unsigned int k = (i >= 0) ? i : (-i) | (1U << 23);
    buf[0] = (k >> 16) & 255;
    buf[1] = (k >> 8) & 255;
    buf[2] = k & 255;
}

inline bool is_all_ones_32(const uint32_t value, const int bit_width)
{
    const int mask = bitmask[bit_width];
    return ((value & mask) ^ mask) == 0;
}

inline bool is_all_ones_64(const uint64_t value, const int bit_width)
{
    const int mask = bitmask[bit_width];
    return ((value & mask) ^ mask) == 0;
}

// 64
inline std::string showbits(const int64_t a)
{
    return std::bitset<64>(a).to_string();
}
inline std::string showbits(const uint64_t a)
{
    return std::bitset<64>(a).to_string();
}

// 32
inline std::string showbits(const int32_t a)
{
    return std::bitset<32>(a).to_string();
}
inline std::string showbits(const uint32_t a)
{
    return std::bitset<32>(a).to_string();
}

// 16
inline std::string showbits(const int16_t a)
{
    return std::bitset<16>(a).to_string();
}
inline std::string showbits(const uint16_t a)
{
    return std::bitset<16>(a).to_string();
}

// 8
inline std::string showbits(const int8_t a)
{
    return std::bitset<8>(a).to_string();
}
inline std::string showbits(const uint8_t a)
{
    return std::bitset<8>(a).to_string();
}

inline int int_pow(const int x, const int p)
{
    int i = 1;
    for (int j = 0; j < p; j++) {
        i *= x;
    }
    return i;
}
