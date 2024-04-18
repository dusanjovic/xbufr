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

#include "tabled.h"

#include "bitutils.h"
#include "fxy.h"
#include "string_utils.h"
#include "tablea.h"
#include "tableb.h"

#include "fmt/format.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

TableD::TableD()
{
    m_tdskip.emplace_back(3, 60, 1); // DRP16BIT
    m_tdskip.emplace_back(3, 60, 2); // DRP8BIT
    m_tdskip.emplace_back(3, 60, 3); // DRPSTAK
    m_tdskip.emplace_back(3, 60, 4); // DRP1BIT
}

void TableD::set_versions(const int master_table_number,
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
    ostr << "d_master_table";
    ostr << "_" << std::setfill('0') << std::setw(3) << m_master_table_number;
    // For master table 0, version 13 is a superset of all earlier versions.
    ostr << "_v" << std::setfill('0') << std::setw(3) << (m_master_table_number == 0 ? std::max(13, m_master_table_version) : m_master_table_version);
    d_master_table_name = ostr.str();

    ostr.str("");
    ostr << "d_local_table";
    ostr << "_c" << std::setfill('0') << std::setw(5) << m_originating_center;
    ostr << "_s" << std::setfill('0') << std::setw(5) << 0; // m_originating_subcenter;
    ostr << "_v" << std::setfill('0') << std::setw(3) << m_local_table_version;
    d_local_table_name = ostr.str();
}

void TableD::add_descriptor(const DescriptorTableD& desc)
{
    m_tabled.push_back(desc);
}

const DescriptorTableD& TableD::get_decriptor(const FXY fxy) const
{
    for (const auto& d_desc : m_tabled) {
        if (fxy == d_desc.fxy()) {
            return d_desc;
        }
    }
    throw std::runtime_error(fmt::format("TableD::get_decriptor cannot find descriptor {} in tabled", fxy.as_str()));
}

bool TableD::search_descriptor(const FXY fxy, DescriptorTableD& desc) const
{
    for (const auto& d_desc : m_tabled) {
        if (fxy == d_desc.fxy()) {
            desc = d_desc;
            return true;
        }
    }
    return false;
}

void TableD::dump1(const TableA& ta, std::ostream& ostr) const
{
    std::vector<FXY>::const_iterator it;

    ostr << "|          |        |                                                          |" << '\n';
    for (const auto& d_desc : m_tabled) {
        const FXY fxy = d_desc.fxy();

        it = find(m_tdskip.begin(), m_tdskip.end(), fxy);
        if (it == m_tdskip.end()) { // didn't find this fxy in list of tdskip

            std::string fxy_string = fxy.as_str();
            bool last_a_entry = false;

            for (size_t ia = 0; ia < ta.m_tablea.size(); ia++) {
                if (ta.m_tablea[ia].mnemonic() == d_desc.mnemonic()) {
                    fxy_string[0] = 'A';
                    if (ia == ta.m_tablea.size() - 1) {
                        last_a_entry = true;
                    }
                    break;
                }
            }

            ostr << std::setfill(' ') << "| " << std::left << std::setw(8) << d_desc.mnemonic()
                 << " | " << fxy_string << " | " << std::setw(55) << d_desc.description() << "  |"
                 << '\n';

            if (last_a_entry) {
                ostr << "|          |        |                                                          |" << '\n';
            }
        }
    }
}

void TableD::dump2(const TableB& tb, std::ostream& ostr) const
{

    ostr << "|------------------------------------------------------------------------------|" << '\n';
    ostr << "| MNEMONIC | SEQUENCE                                                          |" << '\n';
    ostr << "|----------|-------------------------------------------------------------------|" << '\n';
    ostr << "|          |                                                                   |" << '\n';

    for (const auto& d_desc : m_tabled) {
        const FXY fxy = d_desc.fxy();

        std::vector<FXY>::const_iterator it;
        it = find(m_tdskip.begin(), m_tdskip.end(), fxy);
        if (it == m_tdskip.end()) { // didn't find this fxy in list of tdskip

            std::ostringstream parent_str;
            parent_str << std::setfill(' ') << "| " << std::setw(8) << d_desc.mnemonic() << " |";
            const std::string parent = parent_str.str();

            std::vector<Descriptor> seq = d_desc.sequence();

            std::string children_line;

            for (size_t child = 0; child < seq.size(); child++) {

                FXY const child_fxy = seq[child].fxy();
                int cf;
                int cx;
                int cy;
                child_fxy.fxy(cf, cx, cy);

                std::string prefix;
                std::string suffix;

                // define replication tags
                if (cf == 3 && cx == 60 && cy == 1) { // DRP16BIT
                    prefix = "(";
                    suffix = ")";
                } else if (cf == 3 && cx == 60 && cy == 2) { // DRP8BIT
                    prefix = "{";
                    suffix = "}";
                } else if (cf == 3 && cx == 60 && cy == 3) { // DRPSTAK
                    prefix = "[";
                    suffix = "]";
                } else if (cf == 3 && cx == 60 && cy == 4) { // DRP1BIT
                    prefix = "<";
                    suffix = ">";
                } else if (cf == 1 && cx == 1) { // "........"cy
                    prefix = "\"";
                    std::ostringstream sstr;
                    sstr << "\"" << cy;
                    suffix = sstr.str();
                } else {

                    std::ostringstream this_child_str;
                    //  first search in tabled (this table) to find if this child is also tabled entry ie. sequence itself
                    DescriptorTableD desc_d;
                    if (search_descriptor(child_fxy, desc_d)) {
                        this_child_str << " " << prefix << trim(desc_d.mnemonic()) << suffix << " ";
                    } else {
                        // search table B to find this child mnemonic etc.
                        DescriptorTableB desc_b;
                        if (!tb.search_decriptor(child_fxy, desc_b)) {
                            this_child_str << " " << prefix << trim(child_fxy.as_str()) << suffix << " ";
                        } else {
                            std::string nemo = trim(desc_b.mnemonic());
                            // is this a "following value" mnemonic
                            if (nemo[0] == '.') {
                                const DescriptorTableB& next_desc_b = tb.get_decriptor(seq[child + 1].fxy());
                                const std::string& next_nemo = next_desc_b.mnemonic();
                                size_t j = 0;
                                for (size_t n = 1; n < nemo.length(); n++) {
                                    if (nemo[n] == '.') {
                                        nemo[n] = next_nemo[j];
                                        j++;
                                    }
                                }
                            }
                            this_child_str << " " << prefix << nemo << suffix << " ";
                        }
                    }
                    const std::string child_str = this_child_str.str();

                    if (children_line.size() + child_str.size() <= 67) {
                        children_line += child_str;
                    } else {
                        ostr << parent << std::left << std::setw(67) << std::setfill(' ') << children_line << "|" << '\n';
                        children_line = child_str;
                    }

                    prefix = "";
                    suffix = "";
                }
            }
            if (!children_line.empty()) {
                ostr << parent << std::left << std::setw(67) << std::setfill(' ') << children_line << "|" << '\n';
            }

            ostr << "|          |                                                                   |" << '\n';
        }
    }
}

const std::string& TableD::get_master_table_name() const
{
    return d_master_table_name;
}

const std::string& TableD::get_local_table_name() const
{
    return d_local_table_name;
}

bool TableD::read_from_db()
{
    int rc;
    sqlite3* db;

    std::string dbfile;
    if (const char* db_env = std::getenv("DBUFR_DB_DIR")) {
        dbfile = std::string(db_env) + "/bufr_tables.db";
    } else {
        dbfile = "bufr_tables.db";
    }

    rc = sqlite3_open_v2(dbfile.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
    if (rc != SQLITE_OK) {
        std::ostringstream ostr;
        ostr << "Can't open database: " << dbfile << "\n";
        ostr << sqlite3_errmsg(db);
        throw std::runtime_error(ostr.str());
    }

    load_table(db, true);
    if (m_originating_center > 0 && m_local_table_version != 0) {
        load_table(db, false);
    }

    rc = sqlite3_close_v2(db);
    if (rc != SQLITE_OK) {
        std::ostringstream ostr;
        ostr << "Error closing database: " << sqlite3_errmsg(db);
        throw std::runtime_error(ostr.str());
    }
    return true;
}

bool TableD::load_table(sqlite3* db, const bool is_master)
{
    int rc;
    std::string table_name;

    if (is_master) {
        table_name = d_master_table_name;
    } else {
        table_name = d_local_table_name;
    }

    sqlite3_stmt* statement;

    std::ostringstream ostr;
    ostr << "SELECT fxy, mnemonic, name, nchild, childrens FROM " << table_name << ";";

    rc = sqlite3_prepare(db, ostr.str().c_str(), -1, &statement, nullptr);
    if (rc != SQLITE_OK) {
        if (is_master) {
            std::ostringstream estr;
            estr << __FILE__ << " " << __LINE__ << '\n';
            estr << "SQL error: sqlite3_prepare rc=" << rc << " " << sqlite3_errmsg(db) << '\n';
            estr << ostr.str();
            throw std::runtime_error(estr.str());
        }
        return false;
    }

    const int ctotal = sqlite3_column_count(statement);
    if (ctotal != 5) {
        std::ostringstream estr;
        estr << __FILE__ << " " << __LINE__ << " ctotal != 5";
        throw std::runtime_error(estr.str());
    }

    while (true) {
        rc = sqlite3_step(statement);

        if (rc == SQLITE_ROW) {

            std::string fxy = (const char*)sqlite3_column_text(statement, 0);
            const std::string mnemonic = (const char*)sqlite3_column_text(statement, 1);
            const std::string name = (const char*)sqlite3_column_text(statement, 2);
            const unsigned int nchild = sqlite3_column_int(statement, 3);
            const std::string childrens = (const char*)sqlite3_column_text(statement, 4);

            std::vector<std::string> sub_descriptors;
            split(childrens, ',', sub_descriptors);

            if (nchild != sub_descriptors.size()) {
                std::ostringstream estr;
                estr << " nchild != sub_descriptors.size() " << nchild << " " << sub_descriptors.size();
                throw std::runtime_error(estr.str());
            }

            DescriptorTableD d(trim(fxy));
            d.set_mnemonic(mnemonic);
            d.set_description(name);

            for (unsigned int n = 0; n < nchild; n++) {
                const Descriptor d1(trim(sub_descriptors[n]));
                d.add_child(d1);
            }
            add_descriptor(d);
        }

        if (rc == SQLITE_DONE) {
            break;
        }

        if (rc == SQLITE_ERROR) {
            std::cerr << "SQL error:  sqlite3_step " << '\n';
            return false;
        }
    }
    rc = sqlite3_finalize(statement);
    if (rc != SQLITE_OK) {
        std::ostringstream estr;
        estr << "Error sqlite3_finalize: " << sqlite3_errmsg(db);
        throw std::runtime_error(estr.str());
    }

    return true;
}

bool TableD::read_from_file_eccodes(sqlite3* db,
                                    const bool is_master,
                                    const std::string& fname) const
{
    std::cout << "TableD::read_from_file_eccodes " << fname << '\n';

    int rc;

    std::string table_name;

    if (is_master) {
        table_name = d_master_table_name;
    } else {
        table_name = d_local_table_name;
    }

    std::ostringstream ostr;

    ostr << "DROP TABLE IF EXISTS " << table_name << ";\n"
         << "CREATE TABLE " << table_name << " (\n"
         << "    fxy       TEXT NOT NULL,\n"
         << "    mnemonic  TEXT,\n"
         << "    name      TEXT,\n"
         << "    nchild    INTEGER NOT NULL,\n"
         << "    childrens TEXT NOT NULL,\n"
         << "    PRIMARY KEY (fxy) );";

    rc = sqlite3_exec(db, ostr.str().c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
        std::cerr << ostr.str() << '\n';
        return false;
    }

    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
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
    while (std::getline(ifile, line)) {

        const std::string mnemonic;
        const std::string name;
        const std::string fxy = trim(get_left_of_delim(line, "=")).substr(1, 6);

        std::string children = trim(get_right_of_delim(line, "="));
        while (*children.rbegin() != ']') {
            std::getline(ifile, line);
            children += trim(line);
        }
        assert(*children.begin() == '[');
        assert(*children.rbegin() == ']');

        children = children.substr(1, children.size() - 2);
        std::vector<std::string> childrens;
        split(children, ',', childrens);

        const size_t num_child = childrens.size();
        assert(num_child > 0);

        const int f = string_to_int(fxy.substr(0, 1));

        if (f != 3) {
            std::ostringstream estr;
            estr << " incorrect f " << fxy << " " << f;
            throw std::runtime_error(estr.str());
        }

        const int x = string_to_int(fxy.substr(1, 2));
        const int y = string_to_int(fxy.substr(3, 3));

        if (is_master && ((x >= 48 && x <= 63) || (y >= 192 && y <= 255))) {
            continue;
        }

        // FIXME
        // if (!is_master && ((x < 48 && x > 63) || (y < 192 && y > 255))) {
        //     continue;
        // }

        std::string children_new = trim(childrens[0]);
        for (size_t n = 1; n < num_child; n++) {
            children_new += ", " + trim(childrens[n]);
        }

        std::ostringstream sqls;
        sqls << "INSERT INTO " << table_name << " (fxy, mnemonic, name, nchild, childrens) VALUES(";
        sqls << "'" << fxy << "'";
        sqls << ", '" << mnemonic << "'";
        sqls << ", '" << name << "'";
        sqls << ", '" << num_child << "'";
        sqls << ", '" << to_escaped(trim(children_new)) << "');";

        rc = sqlite3_exec(db, sqls.str().c_str(), nullptr, nullptr, nullptr);
        if (rc != SQLITE_OK) {
            std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
            std::cerr << sqls.str() << '\n';
            return false;
        }
    }

    ifile.close();

    rc = sqlite3_exec(db, "END TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
        return false;
    }

    return true;
}

bool TableD::read_from_file_ncep(sqlite3* db,
                                 const bool is_master,
                                 const std::string& fname) const
{
    std::cout << "TableD::read_from_file_ncep " << fname << '\n';

    int rc;

    std::string table_name;

    if (is_master) {
        table_name = d_master_table_name;
    } else {
        table_name = d_local_table_name;
    }

    std::ostringstream ostr;

    ostr << "DROP TABLE IF EXISTS " << table_name << ";\n"
         << "CREATE TABLE " << table_name << " (\n"
         << "    fxy       TEXT NOT NULL,\n"
         << "    mnemonic  TEXT,\n"
         << "    name      TEXT,\n"
         << "    nchild    INTEGER NOT NULL,\n"
         << "    childrens TEXT NOT NULL,\n"
         << "    PRIMARY KEY (fxy) );";

    rc = sqlite3_exec(db, ostr.str().c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
        std::cerr << ostr.str() << '\n';
        return false;
    }

    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
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
    std::getline(ifile, line); // header for ex. 'Table D STD |  0 | 13'

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

            std::vector<std::string> sequence_parts;
            std::vector<std::string> name_parts;
            split(sequence_line, '|', sequence_parts);
            split(sequence_parts[1], ';', name_parts);

            const int f = string_to_int(sequence_parts[0].substr(2, 1));
            const int x = string_to_int(sequence_parts[0].substr(4, 2));
            const int y = string_to_int(sequence_parts[0].substr(7, 3));

            const std::string fxy_string = FXY(f, x, y).as_str();
            const std::string mnemonic = trim(name_parts[0]);
            std::string name = trim(name_parts[2]);

            // end of parsing first line of a new sequence

            // loop until the end of this sequence is found
            int num_child = 0;
            std::string children;
            while (true) {
                std::string sub_descriptor_line;
                std::getline(ifile, sub_descriptor_line);
                num_child++;

                std::vector<std::string> desc_parts;
                split(sub_descriptor_line, '|', desc_parts);

                const int f_child = string_to_int(desc_parts[1].substr(1, 1));
                const int x_child = string_to_int(desc_parts[1].substr(3, 2));
                const int y_child = string_to_int(desc_parts[1].substr(6, 3));

                children += FXY(f_child, x_child, y_child).as_str();

                if (desc_parts[1].substr(10, 1) == ">") {
                    children += ", ";
                } else {
                    break;
                }
            }

            if (is_master && ((x >= 48 && x <= 63) || (y >= 192 && y <= 255))) {
                continue;
            }

            // FIXME
            // if (!is_master && ((x < 48 && x > 63) || (y < 192 && y > 255))) {
            //     continue;
            // }

            std::ostringstream sqls;
            sqls << "INSERT INTO " << table_name << " (fxy, mnemonic, name, nchild, childrens) VALUES(";
            sqls << "'" << fxy_string << "'";
            sqls << ", '" << mnemonic << "'";
            sqls << ", '" << to_escaped(trim(name)) << "'";
            sqls << ", '" << num_child << "'";
            sqls << ", '" << to_escaped(trim(children)) << "');";

            rc = sqlite3_exec(db, sqls.str().c_str(), nullptr, nullptr, nullptr);
            if (rc != SQLITE_OK) {
                std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
                std::cerr << sqls.str() << '\n';
                return false;
            }
        }
    }

    ifile.close();

    rc = sqlite3_exec(db, "END TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
        return false;
    }

    return true;
}
