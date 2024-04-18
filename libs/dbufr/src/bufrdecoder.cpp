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

#include "bufrdecoder.h"

#include "bitreader.h"
#include "bitutils.h"
#include "bufrutil.h"
#include "fxy.h"
#include "string_utils.h"
#include "tablea.h"
#include "tableb.h"
#include "tabled.h"
#include "tablef.h"

#include "fmt/format.h"

#include <cassert>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

static const FXY fxy_031021 = FXY(0, 31, 21);

void BUFRDecoder::parse(std::ifstream& ifile, const std::ios::pos_type file_offset)
{
    size_t len_bufr;
    const std::ios::pos_type pos = seek_bufr(ifile, file_offset, len_bufr);

    m_start_pos = file_offset;
    m_end_pos = file_offset + static_cast<std::ios::pos_type>(len_bufr);

    assert(!m_buffer);

    m_buffer = new unsigned char[len_bufr];

    read_bufr(ifile, pos, len_bufr, m_buffer);

    m_number_of_data_subsets = 0;
    m_number_of_data_values = 0;
    m_cur_data_value = 0;

    m_new_data_width = 0;
    m_new_scale = 0;
    m_new_refval_bits = 0;
    m_signify_data_width = 0;
    m_assocaited_field_bits = 0;
    m_increase_scale_ref_width = 0;
    m_new_ccitt_width = 0;
    m_new_reference_values.clear();

    m_construction_of_bitmap = false;
    m_current_bitmap_index = 0;
    m_backward_reference = -1; // undefined

    parse_sections();

    decode_section_0();
    decode_section_1();
    decode_section_2();
    decode_section_3();
    decode_section_5();
}

BUFRDecoder::~BUFRDecoder()
{
    m_tablea = nullptr;
    m_tableb = nullptr;
    m_tabled = nullptr;

    delete[] m_buffer;
    m_buffer = nullptr;
}

void BUFRDecoder::set_tables(TableA* const tablea,
                             TableB* const tableb,
                             TableD* const tabled,
                             TableF* const tablef)
{
    m_tablea = tablea;
    m_tableb = tableb;
    m_tabled = tabled;
    m_tablef = tablef;
}

void BUFRDecoder::parse_sections()
{
    if (m_buffer[0] != 'B' || m_buffer[1] != 'U' || m_buffer[2] != 'F' || m_buffer[3] != 'R') {
        throw std::runtime_error("BUFRMessage::parse_sections() BUFR string is not at the beginning of m_buffer");
    }

    m_message_length = C3UINT(m_buffer + 4);
    m_edition = C1INT(m_buffer + 7);

    m_sec0_offset = 0;
    m_sec0_length = 8;
    DEBUGLN("m_sec0_offset = " << m_sec0_offset << " m_sec0_length = " << m_sec0_length);

    m_sec1_offset = m_sec0_offset + m_sec0_length;
    if (m_sec1_offset >= m_message_length) {
        throw std::runtime_error("BUFRMessage::parse_sections() m_sec1_offset > m_message_length");
    }
    m_sec1_length = C3UINT(m_buffer + m_sec1_offset);
    DEBUGLN("m_sec1_offset = " << m_sec1_offset << " m_sec1_length = " << m_sec1_length);

    m_sec2_offset = m_sec1_offset + m_sec1_length;
    if (m_sec2_offset >= m_message_length) {
        throw std::runtime_error("BUFRMessage::parse_sections() m_sec2_offset > m_message_length");
    }
    m_sec2_length = 0;
    if (m_edition == 3 && C1INT(m_buffer + m_sec1_offset + 7) != 0) {
        m_sec2_length = C3UINT(m_buffer + m_sec2_offset);
    }
    if (m_edition == 4 && C1INT(m_buffer + m_sec1_offset + 9) != 0) {
        m_sec2_length = C3UINT(m_buffer + m_sec2_offset);
    }
    DEBUGLN("m_sec2_offset = " << m_sec2_offset << " m_sec2_length = " << m_sec2_length);

    m_sec3_offset = m_sec2_offset + m_sec2_length;
    if (m_sec3_offset >= m_message_length) {
        throw std::runtime_error("BUFRMessage::parse_sections() m_sec3_offset > m_message_length");
    }
    m_sec3_length = C3UINT(m_buffer + m_sec3_offset);
    DEBUGLN("m_sec3_offset = " << m_sec3_offset << " m_sec3_length = " << m_sec3_length);

    m_sec4_offset = m_sec3_offset + m_sec3_length;
    if (m_sec4_offset >= m_message_length) {
        throw std::runtime_error("BUFRMessage::parse_sections() m_sec4_offset > m_message_length");
    }
    m_sec4_length = C3UINT(m_buffer + m_sec4_offset);
    DEBUGLN("m_sec4_offset = " << m_sec4_offset << " m_sec4_length = " << m_sec4_length);

    m_sec5_offset = m_sec4_offset + m_sec4_length;
    if (m_sec5_offset >= m_message_length) {
        throw std::runtime_error("BUFRMessage::parse_sections() m_sec5_offset > m_message_length");
    }
    m_sec5_length = 4;
    DEBUGLN("m_sec5_offset = " << m_sec5_offset << " m_sec5_length = " << m_sec5_length);

    if (m_sec0_length + m_sec1_length + m_sec2_length + m_sec3_length + m_sec4_length + m_sec5_length != m_message_length) {
        throw std::runtime_error("Error in BUFRMessage::parse_sections() : message_length != len(0+1+2+3+4+5)");
    }

    if (m_buffer[m_sec5_offset + 0] != '7' || m_buffer[m_sec5_offset + 1] != '7' || m_buffer[m_sec5_offset + 2] != '7' || m_buffer[m_sec5_offset + 3] != '7') {
        throw std::runtime_error("BUFRMessage::parse_sections() 7777 string is not at the end of m_buffer");
    }
}

void BUFRDecoder::decode_section_0()
{
    // we already checked for 'BUFR' string and read in message length and edition in parse_sections
}

void BUFRDecoder::dump_section_0(std::ostream& ostr) const
{
    ostr << "" << '\n';
    ostr << "Section 0 - Indicator Section" << '\n';
    ostr << "---------" << '\n';
    ostr << "Length                     " << m_sec0_length << '\n';
    ostr << "Length of BUFR message     " << m_message_length << " [" << m_start_pos << "-" << m_end_pos - 1 << "]" << '\n';
    ostr << "BUFR edition number        " << m_edition << '\n';
    ostr << '\n';
}

void BUFRDecoder::decode_section_1()
{
    const uint8_t* const sec1 = m_buffer + m_sec1_offset;

    if (m_edition == 2 || m_edition == 3) {
        m_master_table_number = C1INT(sec1 + 3);
        m_originating_subcenter = C1INT(sec1 + 4);
        m_originating_center = C1INT(sec1 + 5);
        m_update_sequence = C1INT(sec1 + 6);
        m_optional_sec_present = (sec1[7] & 0x80U) != 0;
        m_data_cat = C1INT(sec1 + 8);
        m_data_int_subcat = 0;
        m_data_loc_subcat = C1INT(sec1 + 9);
        m_master_table_version = C1INT(sec1 + 10);
        m_local_table_version = C1INT(sec1 + 11);
        const int year_of_century = C1INT(sec1 + 12);
        m_year = (year_of_century <= 50 ? year_of_century + 2000 : year_of_century + 1900);
        m_month = C1INT(sec1 + 13);
        m_day = C1INT(sec1 + 14);
        m_hour = C1INT(sec1 + 15);
        m_minute = C1INT(sec1 + 16);
        m_second = 0;
    } else if (m_edition == 4) {
        m_master_table_number = C1INT(sec1 + 3);
        m_originating_center = C2INT(sec1 + 4);
        m_originating_subcenter = C2INT(sec1 + 6);
        m_update_sequence = C1INT(sec1 + 8);
        m_optional_sec_present = (sec1[9] & 0x80U) != 0;
        m_data_cat = C1INT(sec1 + 10);
        m_data_int_subcat = C1INT(sec1 + 11);
        m_data_loc_subcat = C1INT(sec1 + 12);
        m_master_table_version = C1INT(sec1 + 13);
        m_local_table_version = C1INT(sec1 + 14);
        m_year = C2INT(sec1 + 15);
        m_month = C1INT(sec1 + 17);
        m_day = C1INT(sec1 + 18);
        m_hour = C1INT(sec1 + 19);
        m_minute = C1INT(sec1 + 20);
        m_second = C1INT(sec1 + 21);
    } else {
        throw std::runtime_error(fmt::format("BUFRMessage::decode_section_1(): Edition {} is not supported", m_edition));
    }
}

void BUFRDecoder::dump_section_1(std::ostream& ostr) const
{
    ostr << "Section 1 - Identification Section" << '\n';
    ostr << "---------" << '\n';
    ostr << "Length                     " << m_sec1_length << '\n';
    ostr << "Master table number        " << m_master_table_number << '\n';

    if (m_edition == 4) {
        ostr << "Originating center         " << m_originating_center << " " << m_tablef->get_code_meaning(FXY(0, 1, 35), m_originating_center) << '\n'; // C-11
    } else {
        ostr << "Originating center         " << m_originating_center << " " << m_tablef->get_code_meaning(FXY(0, 1, 33), m_originating_center) << '\n'; // C-1
    }

    const uint64_t orig_sub = (uint64_t)FXY(0, 1, 34).as_int() << 32U | m_originating_subcenter;
    std::map<uint64_t, std::string> code_orig_sub;
    code_orig_sub[orig_sub] = "";
    std::map<FXY, double> associated_codes;
    associated_codes[FXY(0, 1, 33)] = m_originating_center;
    associated_codes[FXY(0, 1, 35)] = m_originating_center;
    m_tablef->populate_code_flags(code_orig_sub, associated_codes);

    if (code_orig_sub[orig_sub] != "NOT FOUND") {
        ostr << "Originating subcenter      " << m_originating_subcenter << " " << code_orig_sub[orig_sub] << '\n';
    } else {
        ostr << "Originating subcenter      " << m_originating_subcenter << '\n';
    }

    ostr << "Update sequence            " << m_update_sequence << '\n';
    ostr << "Optional section           " << (m_optional_sec_present ? "Present" : "Missing") << '\n';
    ostr << "Data category              " << m_data_cat << '\n';
    ostr << "Int. Data sub-category     " << m_data_int_subcat << '\n';
    ostr << "Local data sub-category    " << m_data_loc_subcat << '\n';
    ostr << "Master table version       " << m_master_table_version << '\n';
    ostr << "Local table version        " << m_local_table_version << '\n';
    ostr << "Year (4 digits)            " << m_year << '\n';
    ostr << "Month                      " << m_month << '\n';
    ostr << "Day                        " << m_day << '\n';
    ostr << "Hour                       " << m_hour << '\n';
    ostr << "Minute                     " << m_minute << '\n';
    ostr << "Second                     " << m_second << '\n';
    ostr << '\n';
}

void BUFRDecoder::decode_section_2() const
{
    if (m_sec2_length == 0) {
        return;
    }
}

void BUFRDecoder::dump_section_2(std::ostream& ostr) const
{
    ostr << "Section 2 - Optional Section" << '\n';
    ostr << "--------" << '\n';
    ostr << "Length                     " << m_sec2_length << '\n';
    ostr << '\n';
}

void BUFRDecoder::decode_section_3()
{
    const uint8_t* const sec3 = m_buffer + m_sec3_offset;

    m_number_of_data_subsets = C2UINT(sec3 + 4);

    const unsigned int flags = C1UINT(sec3 + 6); // observed flags
    m_flag_observed = (flags & 0x80U) != 0;
    m_flag_compressed = (flags & 0x40U) != 0;

    // find out how many data descriptors are in this section
    m_num_data_descriptors = (unsigned int)(m_sec3_length - 7) / 2;
    m_data_descriptor_list.clear();
    for (unsigned int i = 0; i < m_num_data_descriptors; i++) {
        m_data_descriptor_list.emplace_back(FXY(C2SHORT(sec3 + 7 + 2 * i)));
    }
}

void BUFRDecoder::dump_section_3(std::ostream& ostr) const
{
    ostr << "Section 3 - Data Description Section" << '\n';
    ostr << "---------" << '\n';
    ostr << "Length                     " << m_sec3_length << '\n';
    ostr << "Number of data subsets     " << m_number_of_data_subsets << '\n';
    ostr << "Observed data              " << (m_flag_observed ? "Yes" : "No") << '\n';
    ostr << "Compressed data            " << (m_flag_compressed ? "Yes" : "No") << '\n';
    ostr << "Number of data descriptors " << m_num_data_descriptors << '\n';
    ostr << " - - - - - -" << '\n';
    for (unsigned int i = 0; i < m_num_data_descriptors; i++) {
        ostr << std::setw(3) << i + 1 << " " << m_data_descriptor_list[i].as_str() << '\n';
    }
    ostr << " - - - - - -" << '\n';
    ostr << '\n';
}

void BUFRDecoder::decode_section_4(NodeItem* const nodeitem)
{
    if (m_number_of_data_subsets == 0) {
        // std::cerr << "BUFRDecoder::decode_section_4: trying to decode message with 0 subsets" << '\n';
        return;
    }

    if (!m_decoded) {
        const uint8_t* const sec4 = m_buffer + m_sec4_offset;

        // skip 4 octets at the beginning of section (length)
        BitReader br(sec4 + 4, (m_sec4_length - 4) * 8, (m_sec4_offset + 4) * 8);

        const unsigned int num_of_subset = m_flag_compressed ? 1 : m_number_of_data_subsets;

        for (unsigned int n = 0; n < num_of_subset; n++) {

            // 94.5.3.9 If a BUFR message is made up of more than one subset,
            //          each subset shall be treated as though it was the first subset encountered.
            m_new_data_width = 0;
            m_new_scale = 0;
            m_new_refval_bits = 0;
            m_signify_data_width = 0;
            m_assocaited_field_bits = 0;
            m_increase_scale_ref_width = 0;
            m_new_ccitt_width = 0;

            m_new_reference_values.clear();

            m_construction_of_bitmap = false;
            m_current_bitmap_index = 0;
            m_backward_reference = -1; // undefined
            m_expanded_descriptors_for_bitmap.clear();

            if (m_flag_compressed) {
                read_descriptor_list(m_data_descriptor_list, 1, br, 0, nodeitem);
                m_subset_nodes.push_back(nodeitem);
            } else {
                NodeItem* subset_nodeitem = nodeitem->add_child();
                Item& subset_item = subset_nodeitem->data();
                std::stringstream ostr;
                ostr << "Subset: " << n + 1;
                subset_item.name = ostr.str();
                subset_item.description = "";

                read_descriptor_list(m_data_descriptor_list, 1, br, 0, subset_nodeitem);
                m_subset_nodes.push_back(subset_nodeitem);
            }
        }

        assert(32 + br.get_pos() + br.get_remaining_bits() == m_sec4_length * 8);
        assert(m_subset_nodes.size() == num_of_subset);

        collect_code_flags(nodeitem);
        m_tablef->populate_code_flags(m_code_meaning, m_loaded_b_descriptors);
        build_code_flags(nodeitem);

        m_decoded = true;
    }
}

void BUFRDecoder::get_values_for_subset(std::vector<std::vector<const NodeItem*>>& values_data_nodes, const unsigned int subset_num)
{
    // subset_num is 1-based
    if ((subset_num - 1) >= m_subset_nodes.size()) {
        // probably just an "empty" message
        return;
    }

    m_number_of_data_values = 0;
    m_cur_data_value = 0;

    NodeItem* subset_nodeitem = m_subset_nodes[subset_num - 1];
    count_data_values(subset_nodeitem);
    const unsigned int num_of_columns = m_flag_compressed ? m_number_of_data_subsets : 1;
    values_data_nodes.resize(m_number_of_data_values);
    for (auto& v_row : values_data_nodes) {
        v_row.resize(num_of_columns);
    }

    update_data_values(subset_nodeitem, values_data_nodes);
    assert(m_cur_data_value == m_number_of_data_values);
}

void BUFRDecoder::dump_section_4(std::ostream& ostr) const
{
    ostr << "Section 4 - Data Section" << '\n';
    ostr << "---------" << '\n';
    ostr << "Length                     " << m_sec4_length << '\n';
    ostr << '\n';
}

void BUFRDecoder::decode_section_5()
{
    const uint8_t* const sec5 = m_buffer + m_sec5_offset;
    if (sec5[0] != '7' || sec5[1] != '7' || sec5[2] != '7' || sec5[3] != '7') {
        throw std::runtime_error("Can not find 7777");
    }
}

void BUFRDecoder::dump_section_5(std::ostream& ostr) const
{
    const uint8_t* const sec5 = m_buffer + m_sec5_offset;

    ostr << "Section 5 - End Section" << '\n';
    ostr << "---------" << '\n';
    ostr << "Length                     " << m_sec5_length << '\n';
    ostr << "7777                       " << sec5[0] << sec5[1] << sec5[2] << sec5[3] << '\n';
    ostr << '\n';
}

void BUFRDecoder::read_descriptor_list(const std::vector<FXY>& descriptor_list,
                                       const unsigned int iterations,
                                       BitReader& br,
                                       const int indent,
                                       NodeItem* parent_nodeitem)
{
    const std::string ind(indent * 2, ' ');

    for (unsigned int iter = 0; iter < iterations; iter++) {

        size_t desc = 0;
        while (desc < descriptor_list.size()) {

            const FXY current_descriptor = descriptor_list[desc];
            const int f = current_descriptor.f();

            const std::string fxy_s = current_descriptor.as_str();

            DEBUG("[" << fxy_s << "]");
            if (iterations > 1) { // iteration of replication
                DEBUG("-r" << iter);
            }
            DEBUG(" ");

            // create an item for this descriptor.
            NodeItem* descriptor_nodeitem = parent_nodeitem->add_child();
            Item& item = descriptor_nodeitem->data();

            item.name = fxy_s;
            if (iterations > 1) { // iteration of replication
                item.name += fmt::format(" ({})", iter);
            }

            DEBUG(ind << fxy_s << " ");

            if (f == 0) { // element descriptor
                item.type = Item::Type::Element;
                read_element_descriptor(current_descriptor,
                                        br,
                                        item,
                                        indent);
            } else if (f == 1) { // replication descriptor
                item.type = Item::Type::Replicator;
                read_replication_descriptor(current_descriptor,
                                            br,
                                            item,
                                            parent_nodeitem,
                                            indent,
                                            descriptor_list,
                                            desc);

            } else if (f == 2) { // operator descriptor
                item.type = Item::Type::Operator;
                const FXY desc_next = (desc + 1) < descriptor_list.size() ? descriptor_list.at(desc + 1) : FXY(0);
                read_operator_descriptor(current_descriptor,
                                         desc_next,
                                         br,
                                         item,
                                         parent_nodeitem,
                                         indent);
            } else if (f == 3) { // sequence descriptor
                item.type = Item::Type::Sequence;
                read_sequence_descriptor(current_descriptor,
                                         br,
                                         item,
                                         descriptor_nodeitem,
                                         indent,
                                         descriptor_list,
                                         desc);
            } else {
                throw std::runtime_error(fmt::format("Error BUFRMessage::read_descriptor_list: unknown descriptor f = ", f));
            }

            desc++;
        } // end of while desc
    }     // end of for iter
}

void BUFRDecoder::read_element_descriptor(const FXY fxy,
                                          BitReader& br,
                                          Item& item,
                                          const int indent,
                                          bool bit_width_plus_one)
{
    item.values.clear();

    const std::string ind(indent * 2, ' ');

    const int f = fxy.f();

    if (f != 0) {
        throw std::runtime_error(fmt::format("Error BUFRMessage::read_element_descriptor: not an element descriptor {}", fxy.as_str()));
    }

    if (m_construction_of_bitmap) {
        // during the bit-map construction all elements
        // must be the element descriptor for the data present indicator (031031)
        assert(fxy.as_str() == "031031");
    }

    const DescriptorTableB& desc = m_tableb->get_decriptor(fxy);

    m_expanded_descriptors_for_bitmap.push_back(fxy);
    if (m_backward_reference < 0) {
        // add a label
        item.name += fmt::format(" [{}]", m_expanded_descriptors_for_bitmap.size() - 1);
    }

    item.fxy = desc.fxy().as_int();
    item.name = item.name + " " + desc.mnemonic();
    item.mnemonic = desc.mnemonic();
    item.unit = desc.unit();
    item.description = desc.description();

    item.bits_range_start = br.get_pos();

    if (!desc.is_numeric_data()) { // character element

        // Apply operator 2 04 YYY (character)
        if (m_assocaited_field_bits > 0 && fxy != fxy_031021) {
            const unsigned int associated_field = br.get_int(m_assocaited_field_bits);
            (void)associated_field; // NOT USED YET
        }

        // Apply operator 2 08 YYY
        const int char_bit_width = m_new_ccitt_width > 0 ? m_new_ccitt_width : desc.bit_width();

        const std::string char_element = br.get_string(char_bit_width);
        item.bits = char_bit_width;

        if (m_flag_compressed && m_number_of_data_subsets > 0) {
            // Section 4 - Data section Note (2)
            // For character data, NBINC (octets here) shell contain the number
            // of octets occupied by the character element I_1, I_2, etc
            const unsigned int octets = br.get_int(6);

            if (octets > 0) {
                // 94.6.3 (2)(i) ... for character data the first value in the set shall be set to all bits zero;
                // assert that all bits in char_element are indeed '\0'.
                for (const char c : char_element) {
                    (void)c;
                    assert(c == '\0' || c == '0'); // NOTE: allow '0' in addition to '\0'. some messages are not following the standard
                }
                for (unsigned int n = 0; n < m_number_of_data_subsets; n++) {
                    const std::string char_element_i = br.get_string(octets * 8);
                    DEBUG(ind << desc.mnemonic() << " = " << std::setw(12) << char_element_i);
                    Item::Value value;
                    value.type = Item::ValueType::String;
                    value.s = char_element_i;
                    item.values.emplace_back(std::move(value));
                }
            } else {
                // 94.6.3 (2)(i) ... however, if the character data values in all subsets are identical,
                //                   the first value shall represent the character string;
                DEBUG(ind << desc.mnemonic() << " = " << std::setw(12) << char_element);
                // m_number_of_data_subsets identical values
                for (unsigned int n = 0; n < m_number_of_data_subsets; n++) {
                    Item::Value value;
                    value.type = Item::ValueType::String;
                    value.s = char_element;
                    item.values.emplace_back(std::move(value));
                }
            }
        } else {
            DEBUG(ind << desc.mnemonic() << " = " << char_element);
            Item::Value value;
            value.type = Item::ValueType::String;
            value.s = char_element;
            item.values.emplace_back(std::move(value));
        }

    } else { // numeric element, non CCITT IA5

        // Apply operator 2 04 YYY (non-character)
        if (m_assocaited_field_bits > 0 && fxy != fxy_031021) {

            const unsigned int associated_field = br.get_int(m_assocaited_field_bits);
            (void)associated_field; // NOT USED YET

            if (m_flag_compressed && m_number_of_data_subsets > 0) {

                const unsigned int bits = br.get_int(6);
                unsigned int increment = 0;
                for (unsigned int n = 0; n < m_number_of_data_subsets; n++) {
                    if (bits > 0) {
                        increment = br.get_int(bits);
                    }
                }
                (void)increment; // NOT USED YET
            }
        }

        int scale = desc.scale();
        int reference = desc.reference();
        int bit_width = desc.bit_width();

        item.ref_value = reference;
        item.scale = scale;
        item.bits = bit_width;

        if (desc.is_data()) {
            // Apply operators 2 01 YYY and 2 02 YYY
            scale += m_new_scale;
            bit_width += m_new_data_width;
            item.scale = scale;
            item.bits = bit_width;
            if (m_new_scale != 0) {
                item.new_scale = true;
            }
            if (m_new_data_width != 0) {
                item.new_bits = true;
            }

            // Apply operator 2 25 255
            // if the element is being read as part of "Difference statistical values marker operator"
            // See BUFR_TableC
            if (bit_width_plus_one) {
                reference = -int_pow(2, bit_width);
                bit_width++;
                item.ref_value = reference;
                item.bits = bit_width;
            }
        }

        // Apply operator 2 03 YYY
        if (m_new_refval_bits > 0 && m_new_refval_bits != 255) {
            // Subsequent element descriptors define new reference
            // values for corresponding Table B entries. Each new
            // reference value is represented by YYY bits in the Data
            // section. Definition of new reference values is concluded
            // by coding this operator with YYY = 255.

            item.bits_range_start = br.get_pos();

            // must read 'm_new_refval' bits and that is a new reference value for this descriptor
            // until it's concluded by m_new_refval_bits
            int new_ref = br.get_int(m_new_refval_bits);

            // Negative reference values shall be represented by a positive
            // integer with the left-most bit (bit 1) set to 1.
            const int top_bit_mask = 1U << (m_new_refval_bits - 1);
            if ((new_ref & top_bit_mask) != 0) {
                new_ref = -(new_ref & ~top_bit_mask);
            }
            m_new_reference_values[desc.fxy()] = new_ref;
            DEBUG('\n');

            item.description = fmt::format("reference value for this descriptor ({}) changed to {}", fxy.as_str(), new_ref);

            item.bits_range_end = br.get_pos() - 1;

            Item::Value value;
            value.type = Item::ValueType::Double;
            value.d = new_ref;
            item.values.emplace_back(std::move(value));

            // add/repeat (m_number_of_data_subsets-1) values of new_ref, just to have the same
            // number of elements in item.values of this row, as in actual data rows
            if (m_flag_compressed && m_number_of_data_subsets > 0) { // compressed, multiple values
                for (unsigned int n = 1; n < m_number_of_data_subsets; n++) {
                    Item::Value value_additional;
                    value_additional.type = Item::ValueType::Double;
                    value_additional.d = new_ref;
                    item.values.emplace_back(std::move(value_additional));
                }
            }

            return; // RETURN RETURN
        }

        if (m_new_refval_bits == 0 || m_new_refval_bits == 255) {
            // end use of modified references
        } else {
            throw std::runtime_error(fmt::format("Error BUFRMessage::read_element_descriptor: Unknown value for m_new_refval_bits {}", m_new_refval_bits));
        }

        if (m_new_reference_values.find(desc.fxy()) != m_new_reference_values.end()) {
            reference = m_new_reference_values.at(desc.fxy());
            item.new_ref_value = true;
            item.ref_value = reference;
        }

        // Apply operator 2 07 YYY
        if (m_increase_scale_ref_width > 0 && desc.is_data()) {
            // 1. Add YYY to the existing scale factor
            scale = scale + m_increase_scale_ref_width;
            // 2. Multiply the existing reference value by 10^YYY
            reference = reference * int_pow(10, m_increase_scale_ref_width);
            // 3.  Calculate ((10 x YYY) + 2) ÷ 3, disregard any
            //     fractional remainder and add the result to the
            //     existing bit width.
            const int add_bit_width = ((10 * m_increase_scale_ref_width) + 2) / 3;
            bit_width = bit_width + add_bit_width;

            item.ref_value = reference;
            item.scale = scale;
            item.bits = bit_width;
            item.new_ref_value = true;
            item.new_scale = true;
            item.new_bits = true;
        }

        DEBUG(ind);

        const double dscale = std::pow(10.0, -scale);

        // Apply operator 2 06 YYY
        if (m_signify_data_width > 0) {
            bit_width = m_signify_data_width;
            m_signify_data_width = 0;
        }

        // 94.6.3 (2)(i)
        // enc_value is a minimum value for a set.
        // a 'local reference value' R_0

        const int64_t enc_value = br.get_int(bit_width);

        if (m_flag_compressed && m_number_of_data_subsets > 0) { // compressed, multiple values

            // Section 4 - Data section Note (2)
            // bits is NBINC
            const unsigned int bits = br.get_int(6);

            if (bits > 0 && is_all_ones_64(enc_value, bit_width)) {
                throw std::runtime_error(fmt::format("Error BUFRMessage::read_element_descriptor:\nNumber bits for increments must be 0 for missing data. It is {}.\nDescriptor {}", bits, fxy.as_str()));
            }

            for (unsigned int n = 0; n < m_number_of_data_subsets; n++) {
                // If NBINC = 0, all values of element I are equal to R_0
                // in such cases, the increments shell be omitted
                unsigned int increment = 0;
                if (bits > 0) {
                    increment = br.get_int(bits);
                }
                // 94.1.7  When a local reference value for a set of element values for compressed data is represented
                //         as all bits set to 1, this shall imply that all values in the set are missing.
                if (is_all_ones_64(enc_value, bit_width)) {
                    item.missing = true;
                    if (m_construction_of_bitmap && (bits > 0 || n == 0)) {
                        assert(bit_width == 1);
                        m_bitmap.push_back((int)enc_value);
                    }
                    if (bits > 0 || n == 0) {
                        DEBUG("MISSING ");
                    }
                } else {
                    const double v = (enc_value + reference + increment) * dscale;
                    Item::Value value;
                    value.type = Item::ValueType::Double;
                    value.d = v;
                    item.values.emplace_back(std::move(value));
                    if (bits > 0 || n == 0) {
                        DEBUG(v << " ");
                    }
                    if (m_construction_of_bitmap && (bits > 0 || n == 0)) {
                        // maybe we can use here enc_value. make sure reference is 0.
                        m_bitmap.push_back((int)v);
                    }
                }
            }

        } else { // non compressed, single value

            if (is_all_ones_64(enc_value, bit_width)) {
                if (m_construction_of_bitmap) {
                    assert(bit_width == 1);
                    m_bitmap.push_back((int)enc_value);
                }
                item.missing = true;
                DEBUG("MISSING");
            } else {
                const double v = (enc_value + reference) * dscale;
                Item::Value value;
                value.type = Item::ValueType::Double;
                value.d = v;
                item.values.emplace_back(std::move(value));
                if (m_construction_of_bitmap) {
                    // maybe we can use here enc_value. make sure reference is 0.
                    m_bitmap.push_back((int)v);
                }
                DEBUG(v);
            }
        }
    }

    item.bits_range_end = br.get_pos() - 1;

    DEBUGLN(" " << desc.unit() << " " << desc.description());

    // save this (numeric) element in a map of already loaded elements
    if (m_data_cat != 11 && !item.missing && desc.is_numeric_data()) {
        // always insert (overwrite)
        m_loaded_b_descriptors[desc.fxy()] = item.values[0].d;
    }
}

void BUFRDecoder::read_replication_descriptor(const FXY fxy,
                                              BitReader& br,
                                              Item& item,
                                              NodeItem* const parent_nodeitem,
                                              const int indent,
                                              const std::vector<FXY>& descriptor_list,
                                              size_t& desc)
{
    const std::string ind(indent * 2, ' ');

    const int x = fxy.x();
    const int y = fxy.y();

    unsigned int niter = y; // if y == 0, niter will be assigned later
    if (y == 0) {           // delayed

        DEBUGLN(ind << fxy.as_str() << " delayed replication operator " << x << " descriptors replicated .... ");

        const size_t desc_next = desc + 1;
        const FXY next_desc = descriptor_list[desc_next];
        int f_next;
        int x_next;
        int y_next;
        next_desc.fxy(f_next, x_next, y_next);

        NodeItem* delayed_nodeitem = parent_nodeitem->add_child();
        Item& item_next = delayed_nodeitem->data();
        item_next.name = next_desc.as_str();
        item_next.type = Item::Type::Replicator;
        item_next.bits_range_start = br.get_pos();

        if (!(f_next == 0 && x_next == 31)) {
            throw std::runtime_error("the descriptor after the delayed replication operator is not 0 31 YYY");
        }

        std::string description_str;
        if (y_next == 0) {
            niter = br.get_int(1);
            description_str = fmt::format("delayed (1-bit delay) replication operator {} descriptors replicated {} times", x, niter);
        } else if (y_next == 1) {
            niter = br.get_int(8);
            description_str = fmt::format("delayed (8-bit delay) replication operator {} descriptors replicated {} times", x, niter);
        } else if (y_next == 2) {
            niter = br.get_int(16);
            description_str = fmt::format("delayed (16-bit delay) replication operator {} descriptors replicated {} times", x, niter);
        } else if (y_next == 11) {
            // FIXME data_repetition_factor must be used to repeat that many times X (eks) data elements
            const unsigned int data_repetition_factor = br.get_int(8);
            std::cerr << next_desc.as_str() << " data_repetition_factor " << data_repetition_factor << " NOT USED YET" << '\n';
            (void)data_repetition_factor; // NOT USED YET
            assert(false);
            niter = 1;
        } else if (y_next == 12) {
            // FIXME data_repetition_factor must be used to repeat that many times X (eks) data elements
            const unsigned int data_repetition_factor = br.get_int(16);
            (void)data_repetition_factor; // NOT USED YET
            assert(false);
            niter = 1;
        } else {
            throw std::runtime_error(fmt::format("Unknown delayed replication f, x, y ", f_next, x_next, y_next));
        }

        // See FM 94 BUFR - 94.5.5.3
        // ... entities described by N element descriptors
        // (including element descriptors for delayed replication, if present)
        m_expanded_descriptors_for_bitmap.push_back(next_desc);

        desc = desc + 1;

        DEBUG("[" << next_desc.as_str() << "] ");
        DEBUGLN(ind << next_desc.as_str() << " delayed replication operator " << x << " descriptors repeated " << niter << " times");

        item_next.description = description_str;

        item_next.bits_range_end = br.get_pos() - 1;

    } else {
        DEBUGLN(ind << fxy.as_str() << " standard replication operator " << x << " descriptors replicated " << y << " times");
    }

    item.description = fmt::format("delayed replication operator {} descriptors replicated ...", x);

    // construct iter_list consisting of the next 'x' descriptors
    std::vector<FXY> iter_list;

    if (niter > 0) {
        for (int i = 0; i < x; i++) {
            if (desc + 1 + i >= descriptor_list.size()) {
                throw std::runtime_error(" replication operator: desc+1+i > descriptor_list.size()");
            }
            iter_list.push_back(descriptor_list[desc + 1 + i]);
        }
    }

    desc = desc + x; // x descriptors will be consumed by this replication

    // maybe this replication is right after bit-map construction
    if (m_construction_of_bitmap) {
        m_bitmap.clear(); // do we need to clear previously defined bitmap?
    }

    if (m_flag_compressed && y == 0) {
        const unsigned int bits = br.get_int(6);
        if (bits > 0) {
            for (unsigned int n = 0; n < m_number_of_data_subsets; n++) {
                read_descriptor_list(iter_list, niter, br, indent, parent_nodeitem);
            }
        } else {
            read_descriptor_list(iter_list, niter, br, indent, parent_nodeitem);
        }
    } else {
        read_descriptor_list(iter_list, niter, br, indent, parent_nodeitem);
    }

    if (m_construction_of_bitmap) {
        // end of data present bit-map construction
        assert(m_bitmap.size() == niter);
        m_construction_of_bitmap = false;
    }
}

void BUFRDecoder::read_operator_descriptor(const FXY fxy,
                                           const FXY fxy_next,
                                           BitReader& br,
                                           Item& item,
                                           NodeItem* const parent_nodeitem,
                                           const int indent)
{
    const int x = fxy.x();
    const int y = fxy.y();

    std::string operator_str;

    // The operations specified by operator descriptors 2 01, 2 02, 2 03,2 04, 2 07 and 2 08
    // remain defined until cancelled or until the end of the data subset.

    // Cancellation of the use of the redefined value shall be effected by the inclusion
    // of the appropriate operand with Y set to 0. The value shall then revert to the original Table B value

    if (x == 1) {
        // Add (YYY–128) bits to the data width given for each data element in Table B,
        // other than CCITT IA5 (character) data, code or flag tables.
        operator_str = fmt::format("operator 2 01 YYY - Change width of data field. y = {}", y);
        DEBUGLN(operator_str << y);
        if (y == 0) {
            m_new_data_width = 0;
        } else {
            m_new_data_width = y - 128;
        }
    } else if (x == 2) {
        // Add YYY–128 to the scale for each data element in Table B,
        // other than CCITT IA5 (character) data, code or flag tables.
        operator_str = fmt::format("operator 2 02 YYY - Change scale of data. y = {}", y);
        DEBUGLN(operator_str);
        if (y == 0) {
            m_new_scale = 0;
        } else {
            m_new_scale = y - 128;
        }
    } else if (x == 3) {
        // Subsequent element descriptors define new reference values
        // for corresponding Table B entries. Each new reference value
        // is represented by YYY bits in the Data section.
        // Definition of new reference values is concluded by coding this operator
        // with YYY = 255.
        // Negative reference values shall be represented by a positive integer
        // with the left-most bit (bit 1) set to 1.
        operator_str = fmt::format("operator 2 03 YYY - Change reference values. y = {}", y);
        DEBUGLN(operator_str);
        m_new_refval_bits = y;
    } else if (x == 4) {
        // Precede each data element with YYY bits of information.
        // This operation associates a data field (e.g. quality control information)
        // of YYY bits with each data element.

        // Note (6): When the descriptor 2 04 YYY is to be used, it shall precede the first
        // of the data descriptors to which it applies.
        operator_str = fmt::format("operator 2 04 YYY - Add associated field. y = {}", y);
        DEBUGLN(operator_str);
        m_assocaited_field_bits = y;
        if (y > 0) {
            // Note (7): The data description operator 2 04 YYY, other than 2 04 000,
            // shall be followed immediately by the descriptor 0 31 021 to indicate
            // the meaning of the associated field.

            // Peek at the next descriptor. It must be 0-31-021.
            if (fxy_next != fxy_031021) {
                throw std::runtime_error("next descriptor after 2-04-YYY is not 031021");
            }
        }
    } else if (x == 5) {
        // YYY characters (CCITT International Alphabet No. 5) are inserted
        // as a data field of YYY x 8 bits in length.
        // The operator 2 05 permits the inclusion of plain language.
        operator_str = fmt::format("operator 2 05 YYY - Signify character operator YYY x 8 bits. y = {}", y);
        DEBUGLN(operator_str);
        // Apply operator 2 05 YYY
        item.bits_range_start = br.get_pos();
        const std::string sig_char = br.get_string(y * 8);
        item.bits_range_end = br.get_pos() - 1;
        Item::Value value;
        value.type = Item::ValueType::String;
        value.s = sig_char;
        item.values.emplace_back(std::move(value));
        DEBUGLN(sig_char);
    } else if (x == 6) {
        // YYY bits of data are described by the immediately following (local?) descriptor.
        // The operator 2 06 YYY allows for the inclusion of local descriptors in a message,
        // with their associated data, which can then be by-passed by a receiver of the message.
        // It can be applied to element descriptors (F=0) only.
        operator_str = fmt::format("operator 2 06 YYY - Signify data width for the immediately following local descriptor y = {}", y);
        DEBUGLN(operator_str);
        m_signify_data_width = y;
    } else if (x == 7) {
        // For Table B elements, which are not CCITT IA5 (character data), code tables, or flag tables:
        //    1. Add YYY to the existing scale factor
        //    2. Multiply the existing reference value by 10^YYY
        //    3. Calculate ((10 x YYY) + 2) ÷ 3, disregard any fractional remainder
        //       and add the result to the existing bit width.
        operator_str = fmt::format("operator 2 07 YYY - Increase scale, reference value and data width. y = {}", y);
        DEBUGLN(operator_str);
        m_increase_scale_ref_width = y;
    } else if (x == 8) {
        // YYY characters from CCITT International Alphabet No. 5 (representing YYY x 8 bits in length)
        // replace the specified data width given for each CCITT IA5 element in Table B.
        operator_str = fmt::format("operator 2 08 YYY - Change width of CCITT IA5 field. y = {}", y);
        DEBUGLN(operator_str);
        m_new_ccitt_width = y * 8;
    } else if (x == 22) {
        // The values of Class 33 elements which follow relate to the data
        // defined by the data present bit-map.
        operator_str = fmt::format("operator 2 22 YYY quality information follows. y = {}", y);
        DEBUGLN(operator_str);
        if (y == 0) {
            m_current_bitmap_index = 0;
            if (m_backward_reference < 0) { // if it's not set, set it
                m_backward_reference = (int)m_expanded_descriptors_for_bitmap.size();
            }
        } else {
            throw std::runtime_error(fmt::format("Unknown operand (YYY) for operator 2 22 YYY. y = ", y));
        }
    } else if (x == 23) {
        // The substituted values which follow relate to the data
        // defined by the data present bit-map.
        operator_str = fmt::format("operator 2 23 YYY substituted values operator. y = {}", y);
        DEBUGLN(operator_str);
        if (y == 0) {
            m_current_bitmap_index = 0;
            if (m_backward_reference < 0) { // if it's not set, set it
                m_backward_reference = (int)m_expanded_descriptors_for_bitmap.size();
            }
        } else if (y == 255) {
            // FIXME
        } else {
            throw std::runtime_error(fmt::format("Unknown operand (YYY) for operator 2 23 YYY. y = ", y));
        }
    } else if (x == 24) {
        //
        operator_str = fmt::format("operator 2 24 YYY first order statistical values marker operator. y = {}", y);
        DEBUGLN(operator_str);
        if (y == 0) {
            m_current_bitmap_index = 0;
            if (m_backward_reference < 0) { // if it's not set, set it
                m_backward_reference = (int)m_expanded_descriptors_for_bitmap.size();
            }
        } else if (y == 255) {
            // read elements based on bitmap
            read_next_bitmap_element(parent_nodeitem, br, indent);
        } else {
            throw std::runtime_error(fmt::format("Unknown operand (YYY) for operator 2 24 YYY. y = ", y));
        }
    } else if (x == 25) {
        operator_str = fmt::format("operator 2 25 YYY Difference statistical values marker operator. y = {}", y);
        DEBUGLN(operator_str);
        if (y == 0) {
            m_current_bitmap_index = 0;
            if (m_backward_reference < 0) { // if it's not set, set it
                m_backward_reference = (int)m_expanded_descriptors_for_bitmap.size();
            }
        } else if (y == 255) {
            // read elements based on bitmap
            // difference statistical values shall be represented as defined by this element descriptor,
            // but with a reference value of –2^n and a data width of (n+1), where n is the data width
            // given by the original descriptor
            const bool bit_width_plus_one = true;
            read_next_bitmap_element(parent_nodeitem, br, indent, bit_width_plus_one);
        } else {
            throw std::runtime_error(fmt::format("Unknown operand (YYY) for operator 2 25 YYY. y = ", y));
        }
    } else if (x == 32) {
        operator_str = fmt::format("operator 2 32 YYY Replaced/retained values follow define data present bitmap. y = {}", y);
        DEBUGLN(operator_str);
        if (y == 0) {
            m_current_bitmap_index = 0;
            if (m_backward_reference < 0) { // if it's not set, set it
                m_backward_reference = (int)m_expanded_descriptors_for_bitmap.size();
            }
        } else if (y == 255) {
            // read elements based on bitmap
            read_next_bitmap_element(parent_nodeitem, br, indent);
        } else {
            throw std::runtime_error(fmt::format("Unknown operand (YYY) for operator 2 32 YYY. y = ", y));
        }
    } else if (x == 35) {
        operator_str = fmt::format("operator 2 35 YYY Cancel backward data reference");
        DEBUGLN(operator_str);
        m_expanded_descriptors_for_bitmap.clear();
        m_backward_reference = -1;
    } else if (x == 36) {
        operator_str = fmt::format("operator 2 36 YYY define data present bit-map");
        DEBUGLN(operator_str);
        assert(y == 0);
        // the next descriptor must be 1-01-YYY.
        int next_f;
        int next_x;
        int next_y;
        fxy_next.fxy(next_f, next_x, next_y);
        if (next_f != 1 || next_x != 1) {
            throw std::runtime_error("next descriptor after 2-36-YYY is not 1-01-YYY");
        }

        // read data present bit-map

        // FM 94 BUFR I.2 – BUFR Reg — 6
        // 94.5.5.3
        // A data present bit-map shall be defined as a set of N one bit values corresponding to N data entities
        // described by N element descriptors (including element descriptors for delayed replication, if  present);
        // the data description of a data present bit-map is comprised of a replication operator followed by
        // the element descriptor for the data present indicator.

        m_construction_of_bitmap = true;

    } else if (x == 37) {
        if (y == 0) {
            operator_str = fmt::format("operator 2 37 000 use defined data present bitmap");
            DEBUGLN(operator_str);
        } else if (y == 255) {
            operator_str = fmt::format("operator 2 37 255 cancel use defined data present bitmap");
            DEBUGLN(operator_str);
        } else {
            throw std::runtime_error(fmt::format("Unknown operand (YYY) for operator 2 37 YYY. y = {}", y));
        }
    } else {
        throw std::runtime_error(fmt::format("Unsupported operator 2 {} {}", x, y));
    }

    item.description = operator_str;
}

void BUFRDecoder::read_sequence_descriptor(const FXY fxy,
                                           BitReader& br,
                                           Item& item,
                                           NodeItem* const descriptor_nodeitem,
                                           const int indent,
                                           const std::vector<FXY>& descriptor_list,
                                           size_t& desc)
{
    const int x = fxy.x();
    const int y = fxy.y();

    std::string sequence_str;

    if (x == 0 && y == 3) { // TableD

        /*
           one entry in table D has the following structure:

          6 bytes /        64 bytes           \ 1byte  /               nchild times 6bytes                 \
         +-------------------------------------------------------------------------------------------------+
         |F|XX|YYY|  line_1    |   line_2     | nchild |F|XX|YYY|F|XX|YYY|F|XX|YYY|   .  .   .    |F|XX|YYY|
         +-------------------------------------------------------------------------------------------------+
          3_00_003          2_05_064           0_31_001     0_00_030_nchild_times
         */

        Item& f_item = descriptor_nodeitem->add_child()->data();
        const FXY f_fxy(0, 0, 10);
        f_item.fxy = f_fxy.as_int();
        f_item.name = f_fxy.as_str();
        f_item.type = Item::Type::Element;
        read_element_descriptor(f_fxy, br, f_item, indent);
        const int desc_d_f = string_to_int(f_item.as_string());

        Item& x_item = descriptor_nodeitem->add_child()->data();
        const FXY x_fxy(0, 0, 11);
        x_item.fxy = x_fxy.as_int();
        x_item.name = x_fxy.as_str();
        x_item.type = Item::Type::Element;
        read_element_descriptor(x_fxy, br, x_item, indent);
        const int desc_d_x = string_to_int(x_item.as_string());

        Item& y_item = descriptor_nodeitem->add_child()->data();
        const FXY y_fxy(0, 0, 12);
        y_item.fxy = y_fxy.as_int();
        y_item.name = y_fxy.as_str();
        y_item.type = Item::Type::Element;
        read_element_descriptor(y_fxy, br, y_item, indent);
        const int desc_d_y = string_to_int(y_item.as_string());

        DescriptorTableD desc_d(desc_d_f, desc_d_x, desc_d_y);
        desc_d.set_mnemonic(FXY(desc_d_f, desc_d_x, desc_d_y).as_str());
        desc_d.set_description(FXY(desc_d_f, desc_d_x, desc_d_y).as_str());

        int f_next;
        int x_next;
        int y_next;

        if (m_originating_center == 7) { // NCEP
            desc++;                      // this next descriptor should be 2_05_YYY where YYY is the number of characters. 64 in this case
            descriptor_list[desc].fxy(f_next, x_next, y_next);
            if (f_next != 2 || x_next != 5 || y_next != 64) {
                throw std::runtime_error("didn't find descriptor 2_05_064");
            }

            Item& oper_item = descriptor_nodeitem->add_child()->data();
            const FXY oper_fxy(f_next, x_next, y_next);
            oper_item.fxy = oper_fxy.as_int();
            oper_item.name = oper_fxy.as_str();
            oper_item.type = Item::Type::Operator;
            read_operator_descriptor(oper_fxy, FXY(0), br, oper_item, nullptr, indent);

            desc_d.set_mnemonic(oper_item.as_string().substr(0, 8));
            desc_d.set_description(trim(oper_item.as_string().substr(9, 55)));
        }

        desc++; // this next descriptor should be 1_01_000
        descriptor_list[desc].fxy(f_next, x_next, y_next);
        if (f_next != 1 || x_next != 1 || y_next != 0) {
            throw std::runtime_error("didn't find iterator 1_01_000");
        }

        Item& iterator_item = descriptor_nodeitem->add_child()->data();
        iterator_item.fxy = FXY(f_next, x_next, y_next).as_int();
        iterator_item.name = FXY(f_next, x_next, y_next).as_str();
        iterator_item.type = Item::Type::Replicator;
        iterator_item.description = fmt::format("delayed replication operator {} descriptors replicated ...", x_next);

        desc++; // this next descriptor should be 0_31_YYY
        descriptor_list[desc].fxy(f_next, x_next, y_next);
        unsigned int nchild = 0;
        if (f_next != 0 || x_next != 31) {
            throw std::runtime_error("didn't find delayed replicator 0_31_YYY");
        }

        Item& rep_item = descriptor_nodeitem->add_child()->data();
        rep_item.name = FXY(f_next, x_next, y_next).as_str();
        rep_item.type = Item::Type::Replicator;
        rep_item.bits_range_start = br.get_pos();
        if (y_next == 1) {
            nchild = br.get_int(8);
            rep_item.description = fmt::format("delayed (8-bit delay) replication operator {} descriptors replicated {} times", 1, nchild);
        } else if (y_next == 2) {
            nchild = br.get_int(16);
            rep_item.description = fmt::format("delayed (16-bit delay) replication operator {} descriptors replicated {} times", 1, nchild);
        } else {
            throw std::runtime_error("didn't find delayed replicator 0_31_YYY YYY=1 or YYY=2");
        }
        rep_item.bits_range_end = br.get_pos() - 1;

        desc++; // this next descriptor should be 0_00_030
        descriptor_list[desc].fxy(f_next, x_next, y_next);
        if (f_next != 0 || x_next != 0 || y_next != 30) {
            throw std::runtime_error("didn't find 00_00_030");
        }
        DEBUG(" " << desc_d_f << desc_d_x << desc_d_y << " [" << nchild << "] ... ");
        const FXY sub_fxy(f_next, x_next, y_next);
        for (unsigned int child = 0; child < nchild; child++) {

            Item& child_item = descriptor_nodeitem->add_child()->data();
            child_item.fxy = sub_fxy.as_int();
            child_item.name = sub_fxy.as_str();
            child_item.type = Item::Type::Element;
            read_element_descriptor(sub_fxy, br, child_item, indent);
            const std::string s_fxy = child_item.as_string();

            desc_d.add_child(Descriptor(s_fxy));
        }
        DEBUGLN(" finished reading  [" << nchild << "]");

        m_tabled->add_descriptor(desc_d);

    } else if (x == 0 && y == 4) { // Table B entry

        auto read_table_b_entry = [&](FXY a_fxy) {
            Item& i = descriptor_nodeitem->add_child()->data();
            i.fxy = a_fxy.as_int();
            i.name = a_fxy.as_str();
            i.type = Item::Type::Element;
            read_element_descriptor(a_fxy, br, i, indent);
            return i.as_string();
        };

        //const size_t entry_start_bit = br.get_pos();
        const int f_elem = string_to_int(read_table_b_entry(FXY(0, 0, 10)));
        const int x_elem = string_to_int(read_table_b_entry(FXY(0, 0, 11)));
        const int y_elem = string_to_int(read_table_b_entry(FXY(0, 0, 12)));
        const std::string line1 = read_table_b_entry(FXY(0, 0, 13));
        const std::string line2 = read_table_b_entry(FXY(0, 0, 14));
        const std::string units = read_table_b_entry(FXY(0, 0, 15));
        const std::string scalesign = read_table_b_entry(FXY(0, 0, 16));
        int scale = string_to_int(read_table_b_entry(FXY(0, 0, 17)));
        const std::string refsign = read_table_b_entry(FXY(0, 0, 18));
        int reference = string_to_int(read_table_b_entry(FXY(0, 0, 19)));
        const int width = string_to_int(read_table_b_entry(FXY(0, 0, 20)));
        // const size_t entry_end_bit = br.get_pos();
        // assert(entry_end_bit - entry_start_bit == 95 * 8);

        if (scalesign == "-") {
            scale = -scale;
        }
        if (refsign == "-") {
            reference = -reference;
        }

        const std::string description = line1 + line2;
        if (m_originating_center == 7) {
            m_tableb->add_descriptor(DescriptorTableB(f_elem, x_elem, y_elem, line1.substr(0, 8), trim(description.substr(9, 55)), units, scale, reference, width));
        } else {
            m_tableb->add_descriptor(DescriptorTableB(f_elem, x_elem, y_elem, "", trim(description), units, scale, reference, width));
        }

    } else if (x == 60 && (y == 1 || y == 2 || y == 3 || y == 4)) { // one of DRP* descriptors

        item.bits_range_start = br.get_pos();
        int niter;
        if (y == 1) {
            niter = br.get_int(16);
            sequence_str = fmt::format("DRP16BIT niter = {}", niter);
            DEBUGLN(sequence_str);
        } else if (y == 2) {
            niter = br.get_int(8);
            sequence_str = fmt::format("DRP8BIT niter = {}", niter);
            DEBUGLN(sequence_str);
        } else if (y == 3) {
            niter = br.get_int(8);
            sequence_str = fmt::format("DRPSTAK niter = {}", niter);
            DEBUGLN(sequence_str);
        } else if (y == 4) {
            niter = br.get_int(1);
            sequence_str = fmt::format("DRP1BIT niter = {}", niter);
            DEBUGLN(sequence_str);
        } else {
            throw std::runtime_error("Unknown 3-60-YYY sequence - DRP* descriptor");
        }

        item.bits_range_end = br.get_pos() - 1;

        if (niter > 0) {
            std::vector<FXY> iter_list;
            iter_list.push_back(descriptor_list[desc + 1]);

            if (m_flag_compressed) {
                const unsigned int bits = br.get_int(6);
                if (bits > 0) {
                    for (unsigned int n = 0; n < m_number_of_data_subsets; n++) {
                        read_descriptor_list(iter_list, niter, br, indent, descriptor_nodeitem);
                    }
                } else {
                    read_descriptor_list(iter_list, niter, br, indent, descriptor_nodeitem);
                }
            } else {
                read_descriptor_list(iter_list, niter, br, indent, descriptor_nodeitem);
            }
        }

        desc++;

    } else {

        const DescriptorTableD& desc_d = m_tabled->get_decriptor(descriptor_list[desc]);
        const std::vector<Descriptor>& sequence = desc_d.sequence();
        std::vector<FXY> sub_sequence;
        sub_sequence.reserve(sequence.size());
        for (const auto& s : sequence) {
            sub_sequence.emplace_back(s.fxy());
        }

        DEBUGLN(desc_d.mnemonic() << " -->");
        sequence_str = desc_d.description();

        read_descriptor_list(sub_sequence, 1, br, indent + 1, descriptor_nodeitem);
    }

    item.description = sequence_str;
}

int BUFRDecoder::get_next_bitmap_index()
{
    for (unsigned int i = m_current_bitmap_index; i < m_bitmap.size(); i++) {
        if (m_bitmap[i] == 0) {
            m_current_bitmap_index = i + 1;
            return i;
        }
    }
    return -1; // not found next 0 index, maybe all are MISSING (ie. 1)
}

void BUFRDecoder::read_next_bitmap_element(NodeItem* parent_nodeitem, BitReader& br, const int indent, bool bit_width_plus_one)
{
    const int bm_index = get_next_bitmap_index();
    if (bm_index >= 0) {
        const size_t back_idx = m_backward_reference - m_bitmap.size() + bm_index;
        const FXY bm_desc = m_expanded_descriptors_for_bitmap[back_idx];
        NodeItem* bitmap_nodeitem = parent_nodeitem->add_child();
        Item& item_bm = bitmap_nodeitem->data();
        item_bm.name = fmt::format("{} -> [{}]", bm_desc.as_str(), back_idx);
        item_bm.type = Item::Type::Element;
        read_element_descriptor(bm_desc, br, item_bm, indent, bit_width_plus_one);
    }
}

void BUFRDecoder::collect_code_flags(NodeItem* ni)
{
    const Item& item = ni->data();
    const FXY fxy(item.fxy);

    if (item.type == Item::Type::Element) { // only elements
        const DescriptorTableB& desc = m_tableb->get_decriptor(fxy);

        if (desc.is_code()) {
            // look up code/flag table if this descriptor is a code/flag
            if (!item.missing) {
                assert(!item.values.empty());
                assert(item.values[0].type == Item::ValueType::Double);
                assert(item.values[0].d < INT_MAX);
                const uint64_t f = (uint64_t)fxy.as_int() << 32 | (unsigned int)item.values[0].d;
                m_code_meaning[f] = "";
            }
        } else if (desc.is_flag()) {
            if (!item.missing) {
                assert(!item.values.empty());
                assert(item.values[0].type == Item::ValueType::Double);
                const int single_value = (int)item.values[0].d;

                // In all flag tables within the BUFR specification, bits are numbered from 1 to N from the most significant to least
                // significant within a data of N bits, i.e. bit No.1 is the leftmost and bit No. N is the rightmost bit within the data width.

                const std::string flag_bits = int_to_bitstring(single_value, item.bits);

                for (int i = 0; i < item.bits; i++) {
                    if (flag_bits[i] == '1') {
                        const uint64_t f = (uint64_t)fxy.as_int() << 32 | (i + 1);
                        m_code_meaning[f] = "";
                    }
                }
            }
        }
    }

    if (ni->has_children()) {
        for (unsigned int i = 0; i < ni->num_children(); i++) {
            collect_code_flags(ni->child(i));
        }
    }
}

void BUFRDecoder::build_code_flags(NodeItem* ni)
{
    Item& item = ni->data();
    const FXY fxy(item.fxy);

    if (item.type == Item::Type::Element) { // only elements
        const DescriptorTableB& desc = m_tableb->get_decriptor(fxy);

        if (desc.is_code()) {
            // look up code/flag table if this descriptor is a code/flag
            if (!item.missing) {
                assert(!item.values.empty());
                assert(item.values[0].type == Item::ValueType::Double);
                assert(item.values[0].d < INT_MAX);
                const uint64_t f = (uint64_t)fxy.as_int() << 32 | (unsigned int)item.values[0].d;
                item.value_tooltip = m_code_meaning[f];
            } else {
                item.value_tooltip = "CODE is missing";
            }
        } else if (desc.is_flag()) {
            if (!item.missing) {
                assert(!item.values.empty());
                assert(item.values[0].type == Item::ValueType::Double);
                const int single_value = (int)item.values[0].d;

                // In all flag tables within the BUFR specification, bits are numbered from 1 to N from the most significant to least
                // significant within a data of N bits, i.e. bit No.1 is the leftmost and bit No. N is the rightmost bit within the data width.

                const std::string flag_bits = int_to_bitstring(single_value, item.bits);

                std::string tooltip_str;

                // The bit No. N (least significant bit) is set to 1 only if all the bits are set to 1 within the data width of the flag table to
                // represent a missing value.
                if (!is_all_ones_32(single_value, item.bits)) {
                    if (single_value % 2 != 0) {
                        item.warning = true;
                        tooltip_str += fmt::format("POTENTIAL BUG (double check)\n\n");
                    }
                }
                tooltip_str += fmt::format("({}) {}", single_value, flag_bits);

                for (int i = 0; i < item.bits; i++) {
                    if (flag_bits[i] == '1') {
                        const uint64_t f = (uint64_t)fxy.as_int() << 32U | (i + 1);
                        tooltip_str += fmt::format("\n{} {}", i + 1, m_code_meaning[f]);
                    }
                }
                item.value_tooltip = tooltip_str;
            } else {
                item.value_tooltip = "FLAG is missing";
            }
        }
    }

    if (ni->has_children()) {
        for (unsigned int i = 0; i < ni->num_children(); i++) {
            build_code_flags(ni->child(i));
        }
    }
}

void BUFRDecoder::count_data_values(NodeItem* ni)
{
    const Item& item = ni->data();

    if (item.type == Item::Type::Element) { // only elements
        m_number_of_data_values++;
    }

    if (ni->has_children()) {
        for (unsigned int i = 0; i < ni->num_children(); i++) {
            count_data_values(ni->child(i));
        }
    }
}

void BUFRDecoder::update_data_values(NodeItem* ni, std::vector<std::vector<const NodeItem*>>& values_data_nodes)
{
    const Item& item = ni->data();

    if (item.type == Item::Type::Element) { // only elements
        if (!m_flag_compressed) {
            // assert(item.values.size() == 1);
            values_data_nodes[m_cur_data_value][0] = ni;
        } else {
            for (unsigned int j = 0; j < m_number_of_data_subsets; j++) {
                values_data_nodes[m_cur_data_value][j] = ni;
            }
        }
        m_cur_data_value++;
    }

    if (ni->has_children()) {
        for (unsigned int i = 0; i < ni->num_children(); i++) {
            update_data_values(ni->child(i), values_data_nodes);
        }
    }
}

int BUFRDecoder::load_tables()
{
    if (m_data_cat == 11) {
        const unsigned char* sec4 = m_buffer + m_sec4_offset;
        BitReader br(sec4 + 4, (m_sec4_length - 4) * 8, (m_sec4_offset + 4) * 8);
        if (m_number_of_data_subsets > 0) {
            if (m_originating_center == 7) {
                read_table_a_ncep(m_data_descriptor_list, br);
            } else if (m_originating_center == 98) {
                read_table_a_ecmwf(m_data_descriptor_list, br);
            } else {
                throw std::runtime_error("Trying to load Table A. Unknown originating center.");
            }
            auto* dummy_nodeitem = new NodeItem;
            read_descriptor_list(m_data_descriptor_list, 1, br, 0, dummy_nodeitem);
            delete dummy_nodeitem;
        }
        return 1; // keep reading following messages
    }
    // this assumes that we have read in all bufr messages with data_cat 11
    // ie. bufr messages that contain bufr tables
    // at this moment m_table[a,b,d] contain A,B and D BUFR tables
    return 0;
}

void BUFRDecoder::read_table_a_ncep(std::vector<FXY>& descriptor_list, BitReader& br)
{
    // assumes that the order of the descriptors in descriptor_list is:

    // 01_03_000 - delayed replication of 3 data descriptors
    // 00_31_001 - this many times (delayed descriptor replication factor) ie. number of iterations of the next 3 elem.
    // 00_00_001 - Table A: entry
    // 00_00_002 - Table A: data category description, line 1
    // 00_00_003 - Table A: data category description, line 2

    // this is specific to NCEP

    int f;
    int x;
    int y;
    int desc_idx;

    desc_idx = 0;
    descriptor_list[desc_idx].fxy(f, x, y);
    if (f != 1 || x != 3 || y != 0) {
        throw std::runtime_error("Error in BUFRMessage::read_table_a_ncep: didn't find iterator 01_03_000 ");
    }

    desc_idx = 1;
    descriptor_list[desc_idx].fxy(f, x, y);
    if (f != 0 || x != 31) {
        throw std::runtime_error("Error in BUFRMessage::read_table_a_ncep: didn't find iterator 00_31_YYY");
    }

    int niter;
    if (y == 0) { // 1-bit delay
        niter = br.get_int(1);
    } else if (y == 1) { // 8-bit delay
        niter = br.get_int(8);
    } else if (y == 2) { // 16-bit delay
        niter = br.get_int(16);
    } else {
        throw std::runtime_error("Error in BUFRMessage::read_table_a_ncep: unknown delayed replication");
    }

    // We will now read niter times 3 descriptors [00_00_001, 00_00_002 and 00_00_003]
    // and create Table A descriptors and add them to the Table A. Each group of these three
    // descriptors define one entry in Table A.

    for (int iter = 0; iter < niter; iter++) {

        desc_idx = 2;
        descriptor_list[desc_idx].fxy(f, x, y);
        if (f != 0 || x != 0 || y != 1) {
            throw std::runtime_error("Error in BUFRMessage::read_table_a_ncep: didn't find 00_00_001");
        }
        Item item_table_a_entry;
        read_element_descriptor(descriptor_list[desc_idx], br, item_table_a_entry, 0);
        const std::string table_a_entry = item_table_a_entry.as_string();

        desc_idx = 3;
        descriptor_list[desc_idx].fxy(f, x, y);
        if (f != 0 || x != 0 || y != 2) {
            throw std::runtime_error("Error in BUFRMessage::read_table_a_ncep: didn't find 00_00_002");
        }
        Item item_line1;
        read_element_descriptor(descriptor_list[desc_idx], br, item_line1, 0);
        const std::string table_a_line1 = item_line1.as_string();

        desc_idx = 4;
        descriptor_list[desc_idx].fxy(f, x, y);
        if (f != 0 || x != 0 || y != 3) {
            throw std::runtime_error("Error in BUFRMessage::read_table_a_ncep: didn't find 00_00_003");
        }
        Item item_line2;
        read_element_descriptor(descriptor_list[desc_idx], br, item_line2, 0);
        const std::string table_a_line2 = item_line2.as_string();

        m_tablea->add_descriptor(DescriptorTableA(0, 0, 0, table_a_entry, table_a_line1 + table_a_line2));
    }
    // table A has been loaded now.

    // remove first 5 elements from the descriptor list that have been used for table A.
    descriptor_list.erase(descriptor_list.begin(), descriptor_list.begin() + 5);
}

void BUFRDecoder::read_table_a_ecmwf(std::vector<FXY>& descriptor_list, BitReader& br)
{
    // assumes that the order of the descriptors in descriptor_list is:
    // 00_00_001
    // 00_00_002
    // 00_00_003
    // 00_00_004
    // 00_00_005
    // 00_00_006
    // 00_00_008
    // ....

    int f;
    int x;
    int y;
    int desc_idx;

    desc_idx = 0;
    descriptor_list[desc_idx].fxy(f, x, y);
    if (f != 0 || x != 0 || y != 1) {
        throw std::runtime_error("Error in BUFRMessage::read_table_a_ecmwf: didn't find 00_00_001");
    }
    Item item_table_a_entry;
    read_element_descriptor(descriptor_list[desc_idx], br, item_table_a_entry, 0);
    const std::string table_a_entry = item_table_a_entry.as_string();

    desc_idx = 1;
    descriptor_list[desc_idx].fxy(f, x, y);
    if (f != 0 || x != 0 || y != 2) {
        throw std::runtime_error("Error in BUFRMessage::read_table_a_ecmwf: didn't find 00_00_002");
    }
    Item item_line1;
    read_element_descriptor(descriptor_list[desc_idx], br, item_line1, 0);
    const std::string table_a_line1 = item_line1.as_string();

    desc_idx = 2;
    descriptor_list[desc_idx].fxy(f, x, y);
    if (f != 0 || x != 0 || y != 3) {
        throw std::runtime_error("Error in BUFRMessage::read_table_a_ecmwf: didn't find 00_00_003");
    }
    Item item_line2;
    read_element_descriptor(descriptor_list[desc_idx], br, item_line2, 0);
    const std::string table_a_line2 = item_line2.as_string();

    m_tablea->add_descriptor(DescriptorTableA(0, 0, 0, table_a_entry, table_a_line1 + table_a_line2));

    Item item_rest;
    read_element_descriptor(descriptor_list[3], br, item_rest, 0);

    read_element_descriptor(descriptor_list[4], br, item_rest, 0);

    read_element_descriptor(descriptor_list[5], br, item_rest, 0);

    read_element_descriptor(descriptor_list[6], br, item_rest, 0);

    // table A has been loaded now.

    // remove first 7 elements from the descriptor list that have been used for table A.
    descriptor_list.erase(descriptor_list.begin(), descriptor_list.begin() + 7);
}
