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

#include "bufrmessage.h"

#include <memory>
#include <string>
#include <vector>

class BUFRFile
{
public:
    explicit BUFRFile(const std::string& filename);
    virtual ~BUFRFile();

    BUFRMessage get_message_num(const unsigned int message_num) const;

    unsigned int num_messages() const;
    unsigned int num_table_messages() const;

    std::string get_tableb_name() const;
    std::string get_tabled_name() const;
    std::string get_tablef_name() const;

    bool has_builtin_tables() const;
    void dump_tables(std::ostream& ostr) const;

private:
    class PrivateData;
    std::unique_ptr<PrivateData> d;

    BUFRFile(const BUFRFile&) = delete;
    BUFRFile& operator=(BUFRFile const&) = delete;
};
