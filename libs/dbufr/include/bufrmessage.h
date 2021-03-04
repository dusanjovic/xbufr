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

#include "item.h"

#include <fstream>

class BUFRDecoder;
class TableA;
class TableB;
class TableD;
class TableF;

class BUFRMessage
{
public:
    BUFRMessage() = default;
    ~BUFRMessage();

    BUFRMessage& operator=(BUFRMessage&& other) noexcept
    {
        m_parsed = other.m_parsed;
        m_decoder = other.m_decoder;
        other.m_parsed = {};
        other.m_decoder = {};
        return *this;
    }

    BUFRMessage(BUFRMessage&& other) noexcept
    {
        m_parsed = other.m_parsed;
        m_decoder = other.m_decoder;
        other.m_parsed = {};
        other.m_decoder = {};
    }

    void parse(std::ifstream& ifile, const std::ios::pos_type file_offset);

    bool is_parsed() const;

    int number_of_subsets() const;

    void set_tables(TableA* const tablea,
                    TableB* const tableb,
                    TableD* const tabled,
                    TableF* const tablef);

    int load_tables();

    void dump_section_0(std::ostream& ostr) const;
    void dump_section_1(std::ostream& ostr) const;
    void dump_section_2(std::ostream& ostr) const;
    void dump_section_3(std::ostream& ostr) const;
    void dump_section_4(std::ostream& ostr) const;
    void dump_section_5(std::ostream& ostr) const;

    void decode_data(NodeItem* const nodeitem);

    void get_values_for_subset(std::vector<std::vector<const NodeItem*>>& values_data_nodes,
                               const unsigned int subset_num = 0);

    // section 1
    int master_table_number() const;
    int originating_center() const;
    int originating_subcenter() const;
    int data_cat() const;
    int master_table_version() const;
    int local_table_version() const;

    // section 3
    unsigned int number_of_data_subsets() const;
    bool flag_observed() const;
    bool flag_compressed() const;

private:
    bool m_parsed{false};
    BUFRDecoder* m_decoder{nullptr};

    BUFRMessage(const BUFRMessage&) = delete;
    BUFRMessage& operator=(BUFRMessage const&) = delete;
};
