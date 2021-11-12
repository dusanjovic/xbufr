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

#include "node.h"

#include <cfloat>
#include <climits>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

class Item
{
public:
    static constexpr int undef_int_value = INT_MAX;
    static constexpr double undef_double_value = DBL_MAX;

    enum class ValueType {
        Unknown,
        Double,
        String
    };

    struct Value {
        std::string s;
        double d{0.0};
        ValueType type{ValueType::Unknown};
    };

    enum class Type {
        Unknown,
        Element,
        Replicator,
        Operator,
        Sequence
    };

    Item() = default;

    Item(const Item&) = delete;
    Item& operator=(Item const&) = delete;

    int as_int() const
    {
        assert(values.size() == 1);
        assert(values[0].type == ValueType::Double);
        return (int)values[0].d;
    }

    const std::string& as_string() const
    {
        assert(values.size() == 1);
        assert(values[0].type == ValueType::String);
        return values[0].s;
    }

    friend std::ostream& operator<<(std::ostream& output, const Item& item)
    {
        if (item.name.empty() && item.unit.empty() && item.description.empty()) {
            return output;
        }

        output << item.name << " ";

        if (item.missing) {
            output << "MISSING ";
        } else {
            if (!item.values.empty()) {
                const Value value = item.values[0];
                if (value.type == ValueType::Double) {
                    output << value.d << " ";
                } else {
                    output << value.s << " ";
                }
            }
        }

        output << item.unit << " ";
        output << item.description;
        return output;
    }

    uint16_t fxy{std::numeric_limits<int16_t>::max()};
    std::string name{};
    std::string mnemonic{};
    std::vector<Value> values{};
    std::string value_tooltip{};
    std::string unit{};
    std::string description{};

    int scale{undef_int_value};
    int ref_value{undef_int_value};
    int bits{undef_int_value};

    bool new_scale{false};
    bool new_ref_value{false};
    bool new_bits{false};

    bool warning{false};
    bool missing{false};

    size_t bits_range_start{0};
    size_t bits_range_end{0};

    Type type{Type::Unknown};
};

using NodeItem = Node<Item>;
