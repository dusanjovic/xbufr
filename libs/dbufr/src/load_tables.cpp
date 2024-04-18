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

#include "sqlite3.h"
#include "string_utils.h"
#include "tableb.h"
#include "tabled.h"
#include "tablef.h"

int main(int argc, char* argv[])
{
    if (argc != 8) {
        std::cerr << " argc != 8 " << '\n';
        return 1;
    }

    const std::string table_type = argv[1];
    const int master_table_number = string_to_int(argv[2]);
    const int master_table_version = string_to_int(argv[3]);
    const int originating_center = string_to_int(argv[4]);
    const int originating_subcenter = string_to_int(argv[5]);
    const int local_table_version = string_to_int(argv[6]);
    const int master = string_to_int(argv[7]);

    const bool is_master = (master == 1);

    const std::string dbfile("bufr_tables.db");

    TableB tb;
    tb.set_versions(master_table_number,
                    master_table_version,
                    originating_center,
                    originating_subcenter,
                    local_table_version);

    TableD td;
    td.set_versions(master_table_number,
                    master_table_version,
                    originating_center,
                    originating_subcenter,
                    local_table_version);

    TableF tf;
    tf.set_versions(master_table_number,
                    master_table_version,
                    originating_center,
                    originating_subcenter,
                    local_table_version);

    int rc;
    sqlite3* db;

    rc = sqlite3_open_v2(dbfile.c_str(), &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Can't open database: " << sqlite3_errmsg(db) << '\n';
        exit(1);
    }

    std::ostringstream btable;
    std::ostringstream dtable;
    std::ostringstream ftable;

    if (table_type == "eccodes") {
        btable << "tables/eccodes/definitions/bufr/tables/" << master_table_number;
        if (is_master) {
            btable << "/wmo/" << master_table_version << "/element.table";
        } else {
            btable << "/local/" << local_table_version << "/" << originating_center << "/" << master_table_version << "/element.table";
        }

        dtable << "tables/eccodes/definitions/bufr/tables/" << std::setfill('0') << std::setw(1) << master_table_number;
        if (is_master) {
            dtable << "/wmo/" << master_table_version << "/sequence.def";
        } else {
            dtable << "/local/" << local_table_version << "/" << originating_center << "/" << master_table_version << "/sequence.def";
        }

        ftable << "tables/eccodes/definitions/bufr/tables/" << std::setfill('0') << std::setw(1) << master_table_number;
        if (is_master) {
            ftable << "/wmo/" << master_table_version << "/codetables";
        } else {
            ftable << "/local/" << local_table_version << "/" << originating_center << "/" << master_table_version << "/codetables";
        }

        tb.read_from_file_eccodes(db, is_master, btable.str());
        td.read_from_file_eccodes(db, is_master, dtable.str());
        tf.read_from_file_eccodes(db, is_master, ftable.str());

    } else if (table_type == "ncep") {
        btable << "tables/ncep/bufrtab.TableB";
        if (is_master) {
            btable << "_STD_" << master_table_number << "_" << master_table_version;
        } else {
            btable << "_LOC_" << originating_subcenter << "_" << originating_center << "_" << local_table_version;
        }

        dtable << "tables/ncep/bufrtab.TableD";
        if (is_master) {
            dtable << "_STD_" << master_table_number << "_" << master_table_version;
        } else {
            dtable << "_LOC_" << originating_subcenter << "_" << originating_center << "_" << local_table_version;
        }

        ftable << "tables/ncep/bufrtab.CodeFlag";
        if (is_master) {
            ftable << "_STD_" << master_table_number << "_" << master_table_version;
        } else {
            ftable << "_LOC_" << originating_subcenter << "_" << originating_center << "_" << local_table_version;
        }

        tb.read_from_file_ncep(db, is_master, btable.str());
        td.read_from_file_ncep(db, is_master, dtable.str());
        tf.read_from_file_ncep(db, is_master, ftable.str());
    }

    sqlite3_close(db);

    return 0;
}
