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

#include "tableb.h"

#include "fxy.h"
#include "string_utils.h"

#include "fmt/format.h"

#include <fstream>
#include <iomanip>
#include <iostream>

TableB::TableB()
{
#ifdef USE_VECTOR
    m_tableb.resize(65535);
#endif
}

void TableB::set_versions(const int master_table_number,
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
    ostr << "b_master_table";
    ostr << "_" << std::setfill('0') << std::setw(3) << m_master_table_number;
    // For master table 0, version 13 is a superset of all earlier versions.
    ostr << "_v" << std::setfill('0') << std::setw(3) << (m_master_table_number == 0 ? std::max(13, m_master_table_version) : m_master_table_version);
    b_master_table_name = ostr.str();

    ostr.str("");
    ostr << "b_local_table";
    ostr << "_c" << std::setfill('0') << std::setw(5) << m_originating_center;
    ostr << "_s" << std::setfill('0') << std::setw(5) << 0; // m_originating_subcenter;
    ostr << "_v" << std::setfill('0') << std::setw(3) << m_local_table_version;
    b_local_table_name = ostr.str();
}

void TableB::add_descriptor(const DescriptorTableB& desc)
{
    const FXY fxy = desc.fxy();
    if (!exists_decriptor(fxy)) {
        m_insertion_order.push_back(fxy);
    }
#ifdef USE_VECTOR
    m_tableb[fxy.as_int()] = desc;
#else
    m_tableb[fxy] = desc;
#endif
}

const DescriptorTableB& TableB::get_decriptor(const FXY fxy) const
{
#ifdef USE_VECTOR
    return m_tableb[fxy.as_int()];
#else
    auto it = m_tableb.find(fxy);
    if (it != m_tableb.end()) {
        return it->second;
    }
    throw std::runtime_error(fmt::format("TableB::get_decriptor cannot find descriptor {} in tableb", fxy.as_str()));
#endif
}

bool TableB::search_decriptor(const FXY fxy, DescriptorTableB& desc) const
{
#ifdef USE_VECTOR
    desc = m_tableb[fxy.as_int()];
    return desc.fxy().as_int() != 0;
#else
    auto it = m_tableb.find(fxy);
    if (it != m_tableb.end()) {
        desc = it->second;
        return true;
    }
    return false;
#endif
}

bool TableB::exists_decriptor(const FXY fxy) const
{
#ifdef USE_VECTOR
    const DescriptorTableB& desc = m_tableb[fxy.as_int()];
    return desc.fxy().as_int() != 0;
#else
    auto it = m_tableb.find(fxy);
    return it != m_tableb.end();
#endif
}

void TableB::dump1(std::ostream& ostr)
{
    ostr << "|          |        |                                                          |" << '\n';
    for (const auto& i : m_insertion_order) {
#ifdef USE_VECTOR
        const DescriptorTableB& desc = m_tableb[i.as_int()];
#else
        const DescriptorTableB& desc = m_tableb[i];
#endif
        const int x = desc.fxy().x();
        if (!(x == 0 || x == 63 || x == 31)) {
            ostr << "| " << std::setw(8) << desc.mnemonic()
                 << " | " << desc.fxy().as_str()
                 << " | " << std::setw(55) << desc.description()
                 << "  |"
                 << '\n';
        }
    }
    ostr << "|          |        |                                                          |" << '\n';
}

void TableB::dump2(std::ostream& ostr)
{
    ostr << "|------------------------------------------------------------------------------|" << '\n';
    ostr << "| MNEMONIC | SCAL | REFERENCE   | BIT | UNITS                    |-------------|" << '\n';
    ostr << "|----------|------|-------------|-----|--------------------------|-------------|" << '\n';
    ostr << "|          |      |             |     |                          |-------------|" << '\n';

    for (const auto& i : m_insertion_order) {
#ifdef USE_VECTOR
        const DescriptorTableB& desc = m_tableb[i.as_int()];
#else
        const DescriptorTableB& desc = m_tableb[i];
#endif
        const int x = desc.fxy().x();
        if (!(x == 0 || x == 63 || x == 31)) {
            ostr << std::right << std::setfill(' ') << "| " << std::setw(8) << desc.mnemonic()
                 << " | " << std::setw(4) << desc.scale()
                 << " | " << std::setw(11) << desc.reference()
                 << " | " << std::setw(3) << desc.bit_width()
                 << " | " << std::left << std::setw(24) << desc.unit()
                 << " |-------------|"
                 << '\n';
        }
    }
    ostr << "|          |      |             |     |                          |-------------|" << '\n';
    ostr << "`------------------------------------------------------------------------------'" << '\n';
}

const std::string& TableB::get_master_table_name() const
{
    return b_master_table_name;
}

const std::string& TableB::get_local_table_name() const
{
    return b_local_table_name;
}

bool TableB::read_from_db()
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

bool TableB::load_table(sqlite3* db, bool is_master)
{
    int rc;
    std::string table_name;

    if (is_master) {
        table_name = b_master_table_name;
    } else {
        table_name = b_local_table_name;
    }

    bool try_ncep_local_table = false;

    sqlite3_stmt* statement;

    std::ostringstream ostr;
    ostr << "SELECT fxy, mnemonic, name, unit, scale, refval, bits FROM " << table_name << ";";

    rc = sqlite3_prepare(db, ostr.str().c_str(), -1, &statement, nullptr);
    if (rc != SQLITE_OK) {
        if (is_master) {
            std::ostringstream estr;
            estr << __FILE__ << " " << __LINE__ << '\n';
            estr << "SQL error: sqlite3_prepare rc=" << rc << " " << sqlite3_errmsg(db) << '\n';
            estr << ostr.str();
            throw std::runtime_error(estr.str());
        }
        if (m_originating_center == 7) {
            try_ncep_local_table = true;
        } else {
            return false;
        }
    }

    if (try_ncep_local_table) {
        table_name = "b_local_table_c00007_s00000_v001";
        ostr << "SELECT fxy, mnemonic, name, unit, scale, refval, bits FROM " << table_name << ";";

        rc = sqlite3_prepare(db, ostr.str().c_str(), -1, &statement, nullptr);
        if (rc != SQLITE_OK) {
            std::ostringstream estr;
            estr << __FILE__ << " " << __LINE__ << '\n';
            estr << "SQL error: sqlite3_prepare rc=" << rc << " " << sqlite3_errmsg(db) << '\n';
            throw std::runtime_error(estr.str());
        }
    }

    int ctotal = sqlite3_column_count(statement);
    if (ctotal != 7) {
        std::ostringstream estr;
        estr << __FILE__ << " " << __LINE__ << " ctotal != 7";
        throw std::runtime_error(estr.str());
    }

    while (true) {
        rc = sqlite3_step(statement);

        if (rc == SQLITE_ROW) {

            const std::string fxy = (const char*)sqlite3_column_text(statement, 0);
            const std::string mnemonic = (const char*)sqlite3_column_text(statement, 1);
            const std::string name = (const char*)sqlite3_column_text(statement, 2);
            const std::string unit = (const char*)sqlite3_column_text(statement, 3);
            const int scale = sqlite3_column_int(statement, 4);
            const int reference = sqlite3_column_int(statement, 5);
            const int bits = sqlite3_column_int(statement, 6);

            const int f = string_to_int(fxy.substr(0, 1));
            const int x = string_to_int(fxy.substr(1, 2));
            const int y = string_to_int(fxy.substr(3, 3));

            add_descriptor(DescriptorTableB(f, x, y, mnemonic, name, unit, scale, reference, bits));
        }

        if (rc == SQLITE_DONE) {
            break;
        }

        if (rc == SQLITE_ERROR) {
            std::ostringstream estr;
            estr << "Error sqlite3_step: " << sqlite3_errmsg(db);
            throw std::runtime_error(estr.str());
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

void TableB::create_table(sqlite3* db, const std::string& table_name)
{
    int rc;

    std::ostringstream ostr;

    ostr << "CREATE TABLE IF NOT EXISTS " << table_name << " (\n"
         << "    fxy      TEXT NOT NULL,\n"
         << "    mnemonic TEXT,\n"
         << "    name     TEXT NOT NULL,\n"
         << "    unit     TEXT NOT NULL,\n"
         << "    scale    INTEGER NOT NULL,\n"
         << "    refval   INTEGER NOT NULL,\n"
         << "    bits     INTEGER NOT NULL,\n"
         << "    origin   TEXT,\n"
         << "    PRIMARY KEY (fxy,scale,refval,bits) );";

    rc = sqlite3_exec(db, ostr.str().c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << std::endl;
        std::cerr << ostr.str() << std::endl;
        exit(1);
    }
}

std::string TableB::build_insert_string(const std::string& origin,
                                        const std::string& refval,
                                        const std::string& name,
                                        const std::string& bits,
                                        const std::string& fxy_string,
                                        const std::string& table_name,
                                        const std::string& scale,
                                        const std::string& mnemonic,
                                        const std::string& unit)
{
    std::ostringstream sqls;
    sqls << "INSERT INTO " << table_name << " (fxy, mnemonic, name, unit, scale, refval, bits, origin) VALUES(";
    sqls << "'" << fxy_string << "'";
    sqls << ", '" << mnemonic << "'";
    sqls << ", '" << to_escaped(trim(name)) << "'";
    sqls << ", '" << trim(unit) << "'";
    sqls << ", " << trim(scale);
    sqls << ", " << trim(refval);
    sqls << ", " << trim(bits);
    sqls << ", '" << trim(origin) << "')";
    return sqls.str();
}

void TableB::insert_row(sqlite3* db,
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
                        const std::string& origin)
{
    std::string row_string;

    const std::string fxy_string = FXY(f, x, y).as_str();
    const bool local_desc = ((x >= 48 && x <= 63) || (y >= 192 && y <= 255));

    if (is_master && local_desc) {
        std::cerr << "Found local descriptor " << fxy_string << " in master table" << '\n';
        return;
    }

    DescriptorTableB existing_desc;
    bool found = search_decriptor(FXY(f, x, y), existing_desc);

    if (found) {
        // or if it's found but the descriptor is different insert new the one
        int iscale = string_to_int(scale);
        int irefval = string_to_int(refval);
        int ibits = string_to_int(bits);
        if (!(existing_desc.scale() == iscale && existing_desc.reference() == irefval && existing_desc.bit_width() == ibits)) {
            std::cout << "Warning: replace b descriptor " << fxy_string << " " << trim(name) << " ";
            std::cout << trim(scale) << " " << trim(refval) << " " << trim(bits) << " ";
            std::cout << existing_desc.scale() << " " << existing_desc.reference() << " " << existing_desc.bit_width() << '\n';
        } else {
            skiped_entries++;
            return;
        }
    }

    new_entries++;
    row_string = build_insert_string(origin, refval, name, bits, fxy_string, table_name, scale, mnemonic, unit);
    int rc = sqlite3_exec(db, row_string.c_str(), nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::ostringstream estr;
        estr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
        estr << row_string;
        throw std::runtime_error(estr.str());
    }
}

bool TableB::read_from_file_eccodes(sqlite3* db,
                                    const bool is_master,
                                    const std::string& fname)
{
    std::cout << "TableB::read_from_file_eccodes " << fname << '\n';

    int rc;

    std::string table_name;

    if (is_master) {
        table_name = b_master_table_name;
    } else {
        table_name = b_local_table_name;
    }

    create_table(db, table_name);

    load_table(db, true); // load master table in memory
    if (!is_master) {
        load_table(db, false);
    }

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

    new_entries = 0;
    skiped_entries = 0;

    // skip first line
    std::getline(ifile, line);

    while (std::getline(ifile, line)) {

        std::vector<std::string> columns;
        split(line, '|', columns);
        const std::string fxy_string = columns[0];

        int f = string_to_int(fxy_string.substr(0, 1));

        if (f < 0 || f > 3) {
            std::ostringstream estr;
            estr << " incorrect f " << f;
            throw std::runtime_error(estr.str());
        }

        int x = string_to_int(fxy_string.substr(1, 2));
        int y = string_to_int(fxy_string.substr(3, 3));

        std::string mnemonic;
        std::string name = columns[3];   // line.substr(8, 64);
        std::string unit = columns[4];   // line.substr(73, 24);
        std::string scale = columns[5];  // line.substr(98, 3);
        std::string refval = columns[6]; // line.substr(102, 12);
        std::string bits = columns[7];   // line.substr(115, 3);

        insert_row(db, table_name, is_master, f, x, y, unit, bits, mnemonic, name, refval, scale, "eccodes");
    }

    std::cout << "eccodes: " << table_name << " new_entries " << new_entries << " skiped_entries " << skiped_entries << '\n';

    ifile.close();

    rc = sqlite3_exec(db, "END TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::ostringstream estr;
        estr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
        throw std::runtime_error(estr.str());
    }

    return true;
}

bool TableB::read_from_file_ncep(sqlite3* db,
                                 const bool is_master,
                                 const std::string& fname)
{
    std::cout << "TableB::read_from_file_ncep " << fname << '\n';

    int rc;

    std::string table_name;

    if (is_master) {
        table_name = b_master_table_name;
    } else {
        table_name = b_local_table_name;
    }

    create_table(db, table_name);

    load_table(db, true); // load master table in memory
    if (!is_master) {
        load_table(db, false);
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

    new_entries = 0;
    skiped_entries = 0;

    std::string line;
    std::getline(ifile, line); // header for ex. 'Table B STD |  0 | 13'

    while (std::getline(ifile, line)) {
        if (line.substr(0, 1) == "#") {
            continue;
        }
        if (line.substr(0, 3) == "END") {
            break;
        }

        std::vector<std::string> columns;
        split(line, '|', columns);

        int f = string_to_int(columns[0].substr(2, 1));
        int x = string_to_int(columns[0].substr(4, 2));
        int y = string_to_int(columns[0].substr(7, 3));

        std::string scale = trim(columns[1]);
        std::string refval = trim(columns[2]);
        std::string bits = trim(columns[3]);
        std::string unit = trim(columns[4]);

        std::vector<std::string> parts;
        split(columns[5], ';', parts);

        std::string mnemonic = trim(parts[0]);
        std::string name = trim(parts[2]);

        insert_row(db, table_name, is_master, f, x, y, unit, bits, mnemonic, name, refval, scale, "ncep");
    }

    std::cout << "ncep:  " << table_name << " new_entries " << new_entries << " skiped_entries " << skiped_entries << '\n';

    ifile.close();

    rc = sqlite3_exec(db, "END TRANSACTION;", nullptr, nullptr, nullptr);
    if (rc != SQLITE_OK) {
        std::ostringstream estr;
        estr << "SQL error: sqlite3_exec " << rc << " " << sqlite3_errmsg(db) << '\n';
        throw std::runtime_error(estr.str());
    }

    return true;
}
