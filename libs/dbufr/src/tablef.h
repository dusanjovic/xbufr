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
#include "sqlite3.h"

#include <map>
#include <string>

class TableF
{
public:
    TableF() = default;
    ~TableF();

    void set_versions(const int master_table_number,
                      const int master_table_version,
                      const int originating_center,
                      const int originating_subcenter,
                      const int local_table_version);

    const std::string& get_master_table_name() const;
    const std::string& get_local_table_name() const;

    void populate_code_flags(std::map<uint64_t, std::string>& code_meaning, const std::map<FXY, double>& b_descriptors);
    std::string get_code_meaning(const FXY fxy, int code);

    bool read_from_file_eccodes(sqlite3* db,
                                const bool is_master,
                                const std::string& fname) const;
    bool read_from_file_ncep(sqlite3* db,
                             const bool is_master,
                             const std::string& fname) const;

private:
    TableF(const TableF&) = delete;
    TableF& operator=(TableF const&) = delete;

    sqlite3* m_db{nullptr};

    int m_master_table_number{-1};
    int m_master_table_version{-1};
    int m_originating_center{-1};
    int m_originating_subcenter{-1};
    int m_local_table_version{-1};

    std::string f_master_table_name;
    std::string f_local_table_name;

    void open_db();
    std::string get_code_meaning_from_table(const std::string& table_name, const FXY fxy, int code);

    void populate_code_flags_from_table(const std::string& table_name,
                                        std::map<uint64_t, std::string>& code_meaning,
                                        const std::map<FXY, double>& b_descriptors);

    static void create_table(sqlite3* db, const std::string& table_name);
    static void insert_row(sqlite3* db,
                           const bool ignore,
                           const std::string& table_name,
                           const std::string& fxy_string,
                           const std::string& mnemonic,
                           const std::string& name,
                           const std::string& list_of_dep_fxy,
                           const std::string& list_of_dep_val,
                           const int code,
                           const std::string& code_meaning,
                           const std::string& origin);
};
