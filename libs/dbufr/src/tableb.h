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

#include "descriptortableb.h"
#include "sqlite3.h"

#include <map>
#include <vector>

//#define USE_VECTOR

class TableB
{
public:
    TableB();

    void set_versions(const int master_table_number,
                      const int master_table_version,
                      const int originating_center,
                      const int originating_subcenter,
                      const int local_table_version);

    void add_descriptor(const DescriptorTableB& desc);
    const DescriptorTableB& get_decriptor(const FXY fxy) const;
    bool search_decriptor(const FXY fxy, DescriptorTableB& desc) const;
    bool exists_decriptor(const FXY fxy) const;

    void dump1(std::ostream& ostr);
    void dump2(std::ostream& ostr);

    const std::string& get_master_table_name() const;
    const std::string& get_local_table_name() const;

    bool read_from_db();

    bool read_from_file_eccodes(sqlite3* db,
                                const bool is_master,
                                const std::string& fname);
    bool read_from_file_ncep(sqlite3* db,
                             const bool is_master,
                             const std::string& fname);

    bool load_table(sqlite3* db, bool is_master);

private:
    TableB(const TableB&) = delete;
    TableB& operator=(TableB const&) = delete;

    std::vector<FXY> m_insertion_order;
#ifdef USE_VECTOR
    std::vector<DescriptorTableB> m_tableb;
#else
    std::map<FXY, DescriptorTableB> m_tableb;
#endif

    int m_master_table_number{-1};
    int m_master_table_version{-1};
    int m_originating_center{-1};
    int m_originating_subcenter{-1};
    int m_local_table_version{-1};

    std::string b_master_table_name;
    std::string b_local_table_name;

    int new_entries{0};
    int skiped_entries{0};

    static void create_table(sqlite3* db, const std::string& table_name);

    void insert_row(sqlite3* db,
                    const std::string& table_name,
                    const bool is_master,
                    const int f,
                    const int x,
                    const int y,
                    const std::string& unit,
                    const std::string& bits,
                    const std::string& mnemonic,
                    const std::string& name,
                    const std::string& refval,
                    const std::string& scale,
                    const std::string& origin);

    static std::string build_insert_string(const std::string& origin,
                                           const std::string& refval,
                                           const std::string& name,
                                           const std::string& bits,
                                           const std::string& fxy_string,
                                           const std::string& table_name,
                                           const std::string& scale,
                                           const std::string& mnemonic,
                                           const std::string& unit);
};
