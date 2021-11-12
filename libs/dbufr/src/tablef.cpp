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

#include "tablef.h"

#include "fxy.h"
#include "string_utils.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

#ifndef _MSC_VER
#include <dirent.h>
#endif

TableF::~TableF()
{
    int rc = sqlite3_close_v2(m_db);
    if (rc != SQLITE_OK) {
        std::ostringstream estr;
        estr << "Error closing database: " << sqlite3_errmsg(m_db);
        std::cerr << estr.str() << '\n';
    }
}

void TableF::set_versions(const int master_table_number,
                          const int master_table_version,
                          const int originating_center,
                          const int originating_subcenter,
                          const int local_table_version)
{
    m_master_table_number = master_table_number;
    m_master_table_version = master_table_version;
    m_originating_center = originating_center;
    m_originating_subcenter = originating_subcenter;
    m_local_table_version = local_table_version;

    std::ostringstream ostr;
    ostr << "f_master_table";
    ostr << "_" << std::setfill('0') << std::setw(3) << m_master_table_number;
    // For master table 0, version 13 is a superset of all earlier versions.
    ostr << "_v" << std::setfill('0') << std::setw(3) << (m_master_table_number == 0 ? std::max(13, m_master_table_version) : m_master_table_version);
    f_master_table_name = ostr.str();

    ostr.str("");
    ostr << "f_local_table";
    ostr << "_c" << std::setfill('0') << std::setw(5) << m_originating_center;
    ostr << "_s" << std::setfill('0') << std::setw(5) << 0; // m_originating_subcenter;
    ostr << "_v" << std::setfill('0') << std::setw(3) << m_local_table_version;
    f_local_table_name = ostr.str();
}

const std::string& TableF::get_master_table_name() const
{
    return f_master_table_name;
}

const std::string& TableF::get_local_table_name() const
{
    return f_local_table_name;
}

void TableF::open_db()
{
    std::string dbfile;
    if (const char* db_env = std::getenv("DBUFR_DB_DIR")) {
        dbfile = std::string(db_env) + "/bufr_tables.db";
    } else {
        dbfile = "bufr_tables.db";
    }

    int rc = sqlite3_open_v2(dbfile.c_str(), &m_db, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK) {
        std::ostringstream ostr;
        ostr << "Can't open database: " << dbfile << "\n";
        ostr << sqlite3_errmsg(m_db);
        throw std::runtime_error(ostr.str());
    }
}

void TableF::populate_code_flags(std::map<uint64_t, std::string>& code_meaning, const std::map<FXY, double>& b_descriptors)
{
    if (m_db == nullptr) {
        open_db();
    }

    if (m_local_table_version != 0) {
        populate_code_flags_from_table(f_local_table_name, code_meaning, b_descriptors);
    }
    populate_code_flags_from_table(f_master_table_name, code_meaning, b_descriptors);
}

std::string TableF::get_code_meaning(const FXY fxy, int code)
{
    if (m_db == nullptr) {
        open_db();
    }

    //if (m_local_table_version != 0) {
    //    get_code_meaning_from_table(f_local_table_name, fxy, code);
    //}
    return get_code_meaning_from_table(f_master_table_name, fxy, code);
}

void TableF::populate_code_flags_from_table(const std::string& table_name,
                                            std::map<uint64_t, std::string>& code_meaning,
                                            const std::map<FXY, double>& b_descriptors)
{
    int rc;

    for (const auto& entry : code_meaning) {

        const uint64_t fxy_and_code = entry.first;
        const std::string& current_meaning = entry.second;
        if (!current_meaning.empty() && current_meaning != "NOT FOUND") {
            continue;
        }

        const FXY desc = FXY((uint16_t)(fxy_and_code >> 32U));
        const int code = fxy_and_code & 0x00000000FFFFFFFF;

        sqlite3_stmt* statement;

        std::ostringstream ostr;
        ostr << "SELECT fxy, mnemonic, code_flag, dep_fxy, dep_val, val, meaning FROM " << table_name;
        ostr << " WHERE fxy = \"" << desc.as_str() << "\" AND val = " << code;

        rc = sqlite3_prepare(m_db, ostr.str().c_str(), -1, &statement, nullptr);
        if (rc != SQLITE_OK) {
            std::ostringstream estr;
            estr << __FILE__ << " " << __LINE__ << '\n';
            estr << "SQL error: sqlite3_prepare rc=" << rc << " " << sqlite3_errmsg(m_db) << '\n';
            estr << ostr.str();
            throw std::runtime_error(estr.str());
        }

        const int ctotal = sqlite3_column_count(statement);
        if (ctotal != 7) {
            std::ostringstream estr;
            estr << __FILE__ << " " << __LINE__ << " ctotal != 7";
            throw std::runtime_error(estr.str());
        }

        int row = 0;
        while (true) {
            rc = sqlite3_step(statement);

            if (rc == SQLITE_ROW) {
                row++;

                // const std::string fxy_string = (char*)sqlite3_column_text(statement, 0);
                // const std::string mnemonic = (char*)sqlite3_column_text(statement, 1);
                // const std::string code_flag = (char*)sqlite3_column_text(statement, 2);
                const std::string dep_fxy = (char*)sqlite3_column_text(statement, 3);
                const std::string dep_val = (char*)sqlite3_column_text(statement, 4);
                const int val = sqlite3_column_int(statement, 5);
                const std::string meaning = (char*)sqlite3_column_text(statement, 6);

                (void)val;
                assert(code == val);

                if (dep_fxy.empty() && dep_val.empty()) {
                    // no dependencies
                    assert(row == 1); // there must be only one row
                    code_meaning[fxy_and_code] = meaning;
                    break; // break while (true)
                }

                // there are dependencies, must check the value of b_descriptors
                const int dep_val_int = string_to_int(dep_val);
                const FXY dep_fxt_int(dep_fxy);

                if (b_descriptors.find(dep_fxt_int) != b_descriptors.end()) {
                    if ((int)b_descriptors.at(dep_fxt_int) == dep_val_int) {
                        code_meaning[fxy_and_code] = meaning;
                        break; // break while (true)
                    }
                }

                // loop to the next row
            }

            if (rc == SQLITE_DONE) {
                code_meaning[fxy_and_code] = "NOT FOUND";
                break; // break while (true)
            }

            if (rc == SQLITE_ERROR) {
                std::cerr << "SQL error:  sqlite3_step " << std::endl;
            }
        } // while (true)

        rc = sqlite3_finalize(statement);
        if (rc != SQLITE_OK) {
            std::ostringstream estr;
            estr << "Error sqlite3_finalize: " << sqlite3_errmsg(m_db);
            throw std::runtime_error(estr.str());
        }
    }
}

std::string TableF::get_code_meaning_from_table(const std::string& table_name, const FXY fxy, int code)
{
    std::string code_meaning;
    sqlite3_stmt* statement;

    std::ostringstream ostr;
    ostr << "SELECT fxy, mnemonic, code_flag, dep_fxy, dep_val, val, meaning FROM " << table_name;
    ostr << " WHERE fxy = \"" << fxy.as_str() << "\" AND val = " << code;

    int rc = sqlite3_prepare(m_db, ostr.str().c_str(), -1, &statement, nullptr);
    if (rc != SQLITE_OK) {
        std::ostringstream estr;
        estr << __FILE__ << " " << __LINE__ << '\n';
        estr << "SQL error: sqlite3_prepare rc=" << rc << " " << sqlite3_errmsg(m_db) << '\n';
        estr << ostr.str();
        throw std::runtime_error(estr.str());
    }

    const int ctotal = sqlite3_column_count(statement);
    if (ctotal != 7) {
        std::ostringstream estr;
        estr << __FILE__ << " " << __LINE__ << " ctotal != 7";
        throw std::runtime_error(estr.str());
    }
    int row = 0;
    while (true) {
        rc = sqlite3_step(statement);

        if (rc == SQLITE_ROW) {
            row++;

            // const std::string fxy_string = (char*)sqlite3_column_text(statement, 0);
            // const std::string mnemonic = (char*)sqlite3_column_text(statement, 1);
            // const std::string code_flag = (char*)sqlite3_column_text(statement, 2);
            const std::string dep_fxy = (char*)sqlite3_column_text(statement, 3);
            const std::string dep_val = (char*)sqlite3_column_text(statement, 4);
            const int val = sqlite3_column_int(statement, 5);
            const std::string meaning = (char*)sqlite3_column_text(statement, 6);

            (void)val;
            assert(code == val);

            if (dep_fxy.empty() && dep_val.empty()) {
                // no dependencies
                assert(row == 1); // there must be only one row
                code_meaning = meaning;
                break; // break while (true)
            }
            assert(false);
            // loop to the next row
        }

        if (rc == SQLITE_DONE) {
            code_meaning = "NOT FOUND";
            break; // break while (true)
        }

        if (rc == SQLITE_ERROR) {
            std::cerr << "SQL error:  sqlite3_step " << std::endl;
        }
    } // while (true)

    rc = sqlite3_finalize(statement);
    if (rc != SQLITE_OK) {
        std::ostringstream estr;
        estr << "Error sqlite3_finalize: " << sqlite3_errmsg(m_db);
        throw std::runtime_error(estr.str());
    }
    return code_meaning;
}

void TableF::create_table(sqlite3* db, const std::string& table_name)
{
    std::ostringstream ostr;

    ostr << "CREATE TABLE IF NOT EXISTS " << table_name << " (\n"
         << "    fxy       TEXT NOT NULL,\n"
         << "    mnemonic  TEXT,\n"
         << "    code_flag TEXT,\n"
         << "    dep_fxy   TEXT NOT NULL,\n"
         << "    dep_val   TEXT NOT NULL,\n"
         << "    val       INTEGER NOT NULL,\n"
         << "    meaning   TEXT,\n"
         << "    origin    TEXT,\n"
         << "    PRIMARY KEY (fxy,dep_fxy,dep_val,val) );";

    int rc = sqlite3_exec(db, ostr.str().c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << std::endl;
        std::cerr << ostr.str() << std::endl;
        exit(1);
    }
}

void TableF::insert_row(sqlite3* db,
                        const bool ignore,
                        const std::string& table_name,
                        const std::string& fxy_string,
                        const std::string& mnemonic,
                        const std::string& name,
                        const std::string& list_of_dep_fxy,
                        const std::string& list_of_dep_val,
                        const int code,
                        const std::string& code_meaning,
                        const std::string& origin)
{
    std::ostringstream sqls;
    sqls << "INSERT ";
    if (ignore) {
        sqls << "OR IGNORE ";
    }
    sqls << "INTO " << table_name << " (fxy, mnemonic, code_flag, dep_fxy, dep_val, val, meaning, origin) VALUES(";
    sqls << "'" << fxy_string << "'";
    sqls << ", '" << mnemonic << "'";
    sqls << ", '" << to_escaped(trim(name)) << "'";
    sqls << ", '" << to_escaped(trim(list_of_dep_fxy)) << "'";
    sqls << ", '" << to_escaped(trim(list_of_dep_val)) << "'";
    sqls << ", '" << code << "'";
    sqls << ", '" << to_escaped(trim(code_meaning)) << "'";
    sqls << ", '" << origin << "'";
    sqls << ");";

    int rc = sqlite3_exec(db, sqls.str().c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::ostringstream estr;
        estr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << std::endl;
        estr << sqls.str() << std::endl;
        throw std::runtime_error(estr.str());
    }
}

bool TableF::read_from_file_ncep(sqlite3* db,
                                 const bool is_master,
                                 const std::string& fname) const
{
    int rc;

    std::string table_name;

    if (is_master) {
        table_name = f_master_table_name;
    } else {
        table_name = f_local_table_name;
    }

    create_table(db, table_name);

    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    std::ifstream ifile;
    ifile.open(fname.c_str(), std::ios::in);
    if (!ifile) {
        std::ostringstream estr;
        estr << "can not open " << fname;
        throw std::runtime_error(estr.str());
    }

    std::string line;
    std::getline(ifile, line); // header for ex. 'Table F STD |  0 | 13'

    while (std::getline(ifile, line)) {

        if (line.substr(0, 1) == "#") {
            continue;
        }
        if (line.substr(0, 3) == "END") {
            break;
        }

        if (line.empty()) {

            // read and start parsing first line of a new sequence
            std::string sequence_line;
            std::getline(ifile, sequence_line);

            std::vector<std::string> sequence_parts, name_parts;
            split(sequence_line, '|', sequence_parts);
            split(sequence_parts[1], ';', name_parts);

            int f = string_to_int(sequence_parts[0].substr(2, 1));
            int x = string_to_int(sequence_parts[0].substr(4, 2));
            int y = string_to_int(sequence_parts[0].substr(7, 3));

            std::string fxy_string = FXY(f, x, y).as_str();
            std::string mnemonic = trim(name_parts[0]);
            std::string name = trim(name_parts[1]); // CODE or FLAG

            std::string list_of_dep_fxy;
            std::string list_of_dep_val;

            // end of parsing first line of a new sequence

            // loop until the end of this sequence is found
            while (true) {
                std::string code_flag_line;
                std::getline(ifile, code_flag_line);

                std::vector<std::string> code_flag_parts;
                split(code_flag_line, '|', code_flag_parts);

                if (code_flag_parts.size() == 2) { //  | F-XX-YYY=VAL

                    std::vector<std::string> dep_parts;
                    split(code_flag_parts[1], '=', dep_parts);

                    list_of_dep_fxy = dep_parts[0];
                    list_of_dep_val = dep_parts[1];

                } else if (code_flag_parts.size() == 3) { //   | VAL > | MEANING

                    const std::string val = code_flag_parts[1];
                    std::vector<std::string> val_parts;
                    split(val, ' ', val_parts);

                    std::vector<std::string> dep_fxy_parts;
                    split(list_of_dep_fxy, ',', dep_fxy_parts);

                    std::vector<std::string> dep_val_parts;
                    split(list_of_dep_val, ',', dep_val_parts);

                    const int code = string_to_int(val_parts[1]);
                    const std::string code_meaning = code_flag_parts[2];

                    if (dep_fxy_parts.empty() && dep_val_parts.empty()) {
                        insert_row(db, false, table_name, fxy_string, mnemonic, name, list_of_dep_fxy, list_of_dep_val, code, code_meaning, "ncep");
                    } else {
                        for (const auto& dep_fxy_part : dep_fxy_parts) {
                            for (const auto& dep_val_part : dep_val_parts) {
                                insert_row(db, false, table_name, fxy_string, mnemonic, name, dep_fxy_part, dep_val_part, code, code_meaning, "ncep");
                            }
                        }
                    }

                    if (val_parts[2] == ">") {
                    } else {
                        break;
                    }
                }
            }
        }
    }

    ifile.close();

    rc = sqlite3_exec(db, "END TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    return true;
}

bool TableF::read_from_file_eccodes(sqlite3* db, const bool is_master, const std::string& fname) const
{
#ifndef _MSC_VER
    int rc;

    std::cout << "TableF::read_from_file_eccodes " << fname << '\n';
    std::string table_name;

    if (is_master) {
        table_name = f_master_table_name;
    } else {
        table_name = f_local_table_name;
    }

    create_table(db, table_name);

    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    if (auto* dir = opendir(fname.c_str())) {
        while (auto* f = readdir(dir)) {
            if (f->d_name[0] == '.') {
                continue;
            }

            const std::string file_name = fname + "/" + f->d_name;
            if (!ends_with(file_name, ".table")) {
                continue;
            }

            const std::string base_filename = file_name.substr(file_name.find_last_of("/\\") + 1);
            const std::string codetable_num = base_filename.substr(0, base_filename.find_last_of('.'));

            std::ifstream ifile;
            ifile.open(file_name.c_str(), std::ios::in);
            if (!ifile) {
                std::ostringstream estr;
                estr << "can not open " << file_name;
                throw std::runtime_error(estr.str());
            }

            std::string line;
            while (std::getline(ifile, line)) {

                std::ostringstream ss;
                ss << std::setfill('0') << std::setw(6) << codetable_num;
                const std::string fxy_string = ss.str();
                const int code = string_to_int(line.substr(0, nth_occurrence(line, " ", 1)));
                const std::string code_meaning = erase_all_substr(line.substr(nth_occurrence(line, " ", 2) + 1), "\"    ");

                insert_row(db, true, table_name, fxy_string, "", "", "", "", code, code_meaning, "eccodes");
            }

            ifile.close();
        }
        closedir(dir);
    }

    rc = sqlite3_exec(db, "END TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
#endif
    return true;
}
