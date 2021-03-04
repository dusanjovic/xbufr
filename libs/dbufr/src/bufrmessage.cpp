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

#include "bufrmessage.h"
#include "bufrdecoder.h"

BUFRMessage::~BUFRMessage()
{
    delete m_decoder;
}

void BUFRMessage::parse(std::ifstream& ifile,
                        const std::ios::pos_type file_offset)
{
    if (m_parsed) {
        return;
    }
    m_decoder = new BUFRDecoder();
    m_decoder->parse(ifile, file_offset);
    m_parsed = true;
}

bool BUFRMessage::is_parsed() const
{
    return m_parsed;
}

int BUFRMessage::number_of_subsets() const
{
    return flag_compressed() ? 1 : number_of_data_subsets();
}

void BUFRMessage::set_tables(TableA* const tablea,
                             TableB* const tableb,
                             TableD* const tabled,
                             TableF* const tablef)
{
    assert(m_decoder);
    m_decoder->set_tables(tablea, tableb, tabled, tablef);
}

int BUFRMessage::load_tables()
{
    assert(m_decoder);
    return m_decoder->load_tables();
}

void BUFRMessage::dump_section_0(std::ostream& ostr) const
{
    assert(m_decoder);
    m_decoder->dump_section_0(ostr);
}

void BUFRMessage::dump_section_1(std::ostream& ostr) const
{
    assert(m_decoder);
    m_decoder->dump_section_1(ostr);
}

void BUFRMessage::dump_section_2(std::ostream& ostr) const
{
    assert(m_decoder);
    m_decoder->dump_section_2(ostr);
}

void BUFRMessage::dump_section_3(std::ostream& ostr) const
{
    assert(m_decoder);
    m_decoder->dump_section_3(ostr);
}

void BUFRMessage::dump_section_4(std::ostream& ostr) const
{
    assert(m_decoder);
    m_decoder->dump_section_4(ostr);
}

void BUFRMessage::dump_section_5(std::ostream& ostr) const
{
    assert(m_decoder);
    m_decoder->dump_section_5(ostr);
}

void BUFRMessage::decode_data(NodeItem* const nodeitem)
{
    assert(m_decoder);
    m_decoder->decode_section_4(nodeitem);
}

void BUFRMessage::get_values_for_subset(std::vector<std::vector<const NodeItem*>>& values_data_nodes,
                                        const unsigned int subset_num)
{
    assert(m_decoder);
    m_decoder->get_values_for_subset(values_data_nodes, subset_num);
}

int BUFRMessage::master_table_number() const
{
    assert(m_decoder);
    return m_decoder->m_master_table_number;
}

int BUFRMessage::originating_center() const
{
    assert(m_decoder);
    return m_decoder->m_originating_center;
}

int BUFRMessage::originating_subcenter() const
{
    assert(m_decoder);
    return m_decoder->m_originating_subcenter;
}

int BUFRMessage::data_cat() const
{
    assert(m_decoder);
    return m_decoder->m_data_cat;
}

int BUFRMessage::master_table_version() const
{
    assert(m_decoder);
    return m_decoder->m_master_table_version;
}

int BUFRMessage::local_table_version() const
{
    assert(m_decoder);
    return m_decoder->m_local_table_version;
}

unsigned int BUFRMessage::number_of_data_subsets() const
{
    assert(m_decoder);
    return m_decoder->m_number_of_data_subsets;
}

bool BUFRMessage::flag_observed() const
{
    assert(m_decoder);
    return m_decoder->m_flag_observed;
}

bool BUFRMessage::flag_compressed() const
{
    assert(m_decoder);
    return m_decoder->m_flag_compressed;
}
