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

#include "fxy.h"
#include "item.h"

#include <fstream>
#include <map>
#include <vector>

class BitReader;
class TableA;
class TableB;
class TableD;
class TableF;

class BUFRDecoder
{
public:
    BUFRDecoder() = default;
    virtual ~BUFRDecoder();

    void set_tables(TableA* const tablea,
                    TableB* const tableb,
                    TableD* const tabled,
                    TableF* const tablef);

    void decode_section_4(NodeItem* const nodeitem);

    void get_values_for_subset(std::vector<std::vector<const NodeItem*>>& values_data_nodes,
                               const unsigned int subset_num = 0);

    int load_tables();

    void dump_section_0(std::ostream& ostr) const;
    void dump_section_1(std::ostream& ostr) const;
    void dump_section_2(std::ostream& ostr) const;
    void dump_section_3(std::ostream& ostr) const;
    void dump_section_4(std::ostream& ostr) const;
    void dump_section_5(std::ostream& ostr) const;

    // section 0
    size_t m_message_length{0};
    int m_edition{0};

    // section 1
    int m_master_table_number{0};
    int m_originating_center{0};
    int m_originating_subcenter{0};
    int m_update_sequence{0};
    bool m_optional_sec_present{false};
    int m_data_cat{0};
    int m_data_int_subcat{0};
    int m_data_loc_subcat{0};
    int m_master_table_version{0};
    int m_local_table_version{0};
    int m_year{0};
    int m_month{0};
    int m_day{0};
    int m_hour{0};
    int m_minute{0};
    int m_second{0};

    // section 3
    unsigned int m_number_of_data_subsets{0};
    bool m_flag_observed{false};
    bool m_flag_compressed{false};
    unsigned int m_num_data_descriptors{0};
    std::vector<FXY> m_data_descriptor_list;

    void parse(std::ifstream& ifile, const std::ios::pos_type file_offset);

private:
    BUFRDecoder(const BUFRDecoder&) = delete;
    BUFRDecoder& operator=(BUFRDecoder const&) = delete;

    uint8_t* m_buffer{nullptr};

    size_t m_sec0_offset{0};
    size_t m_sec1_offset{0};
    size_t m_sec2_offset{0};
    size_t m_sec3_offset{0};
    size_t m_sec4_offset{0};
    size_t m_sec5_offset{0};

    size_t m_sec0_length{0};
    size_t m_sec1_length{0};
    size_t m_sec2_length{0};
    size_t m_sec3_length{0};
    size_t m_sec4_length{0};
    size_t m_sec5_length{0};

    void parse_sections();

    void decode_section_0();
    void decode_section_1();
    void decode_section_2() const;
    void decode_section_3();
    void decode_section_5();

    void read_table_a_ncep(std::vector<FXY>& descriptor_list, BitReader& br);
    void read_table_a_ecmwf(std::vector<FXY>& descriptor_list, BitReader& br);

    void read_descriptor_list(const std::vector<FXY>& descriptor_list,
                              const unsigned int iterations,
                              BitReader& br,
                              const int indent,
                              NodeItem* parent_nodeitem);

    void read_element_descriptor(const FXY fxy,
                                 BitReader& br,
                                 Item& item,
                                 const int indent,
                                 bool bit_width_plus_one = false);

    void read_replication_descriptor(const FXY fxy,
                                     BitReader& br,
                                     Item& item,
                                     NodeItem* const parent_nodeitem,
                                     const int indent,
                                     const std::vector<FXY>& descriptor_list,
                                     size_t& desc);

    void read_operator_descriptor(const FXY fxy,
                                  const FXY fxy_next,
                                  BitReader& br,
                                  Item& item,
                                  NodeItem* const parent_nodeitem,
                                  const int indent);

    void read_sequence_descriptor(const FXY fxy,
                                  BitReader& br,
                                  Item& item,
                                  NodeItem* const descriptor_nodeitem,
                                  const int indent,
                                  const std::vector<FXY>& descriptor_list,
                                  size_t& desc);

    TableA* m_tablea{nullptr};
    TableB* m_tableb{nullptr};
    TableD* m_tabled{nullptr};
    TableF* m_tablef{nullptr};

    int m_new_data_width{0};
    int m_new_scale{0};
    int m_new_refval_bits{0};
    int m_signify_data_width{0};
    int m_assocaited_field_bits{0};
    int m_increase_scale_ref_width{0};
    int m_new_ccitt_width{0};

    std::map<FXY, int> m_new_reference_values{};
    std::map<FXY, double> m_loaded_b_descriptors{};

    bool m_construction_of_bitmap{false};
    std::vector<int> m_bitmap{};

    std::vector<FXY> m_expanded_descriptors_for_bitmap{};
    unsigned int m_current_bitmap_index{0};
    int m_backward_reference{-1}; // undefined
    int get_next_bitmap_index();
    void read_next_bitmap_element(NodeItem* parent_nodeitem, BitReader& br, const int indent, bool bit_width_plus_one = false);

    size_t m_start_pos{0};
    size_t m_end_pos{0};

    // Values data
    void count_data_values(NodeItem* ni);
    void update_data_values(NodeItem* ni, std::vector<std::vector<const NodeItem*>>& values_data_nodes);
    int m_number_of_data_values{0};
    int m_cur_data_value{0};

    std::vector<NodeItem*> m_subset_nodes{};
    bool m_decoded{false};

    std::map<uint64_t, std::string> m_code_meaning{};
    void collect_code_flags(NodeItem* ni);
    void build_code_flags(NodeItem* ni);
};

#define NO_DEBUG

#ifndef NO_DEBUG
#define DEBUGLN(x) std::cout << x << '\n'
#define DEBUG(x) std::cout << x
#else
#define DEBUGLN(x)
#define DEBUG(x)
#endif
