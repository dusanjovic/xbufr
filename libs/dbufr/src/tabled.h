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

#include "descriptortabled.h"
#include "sqlite3.h"

#include <string>
#include <vector>

class TableA;
class TableB;

class TableD
{
public:
    TableD();

    void set_versions(const int master_table_number,
                      const int master_table_version,
                      const int originating_center,
                      const int originating_subcenter,
                      const int local_table_version);

    void add_descriptor(const DescriptorTableD& desc);
    const DescriptorTableD& get_decriptor(const FXY fxy) const;
    bool search_descriptor(const FXY fxy, DescriptorTableD& desc) const;

    void dump1(const TableA& ta, std::ostream& ostr) const;
    void dump2(const TableB& tb, std::ostream& ostr) const;

    const std::string& get_master_table_name() const;
    const std::string& get_local_table_name() const;

    bool read_from_db();

    bool read_from_file_eccodes(sqlite3* db,
                                const bool is_master,
                                const std::string& fname) const;
    bool read_from_file_ncep(sqlite3* db,
                             const bool is_master,
                             const std::string& fname) const;

    bool load_table(sqlite3* db, const bool is_master);

private:
    TableD(const TableD&) = delete;
    TableD& operator=(TableD const&) = delete;

    std::vector<DescriptorTableD> m_tabled;
    std::vector<FXY> m_tdskip;

    int m_master_table_number{-1};
    int m_master_table_version{-1};
    int m_originating_center{-1};
    int m_originating_subcenter{-1};
    int m_local_table_version{-1};

    std::string d_master_table_name;
    std::string d_local_table_name;
};
