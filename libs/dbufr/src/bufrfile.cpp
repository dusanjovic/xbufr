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

#include "bufrfile.h"

#include "bufrdecoder.h"
#include "bufrmessage.h"
#include "bufrutil.h"
#include "descriptortableb.h"
#include "tablea.h"
#include "tableb.h"
#include "tabled.h"
#include "tablef.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>

class BUFRFile::PrivateData
{
public:
    std::ifstream ifile;
    std::vector<std::ios::pos_type> offset;

    int curr_master_table_number{0};
    int curr_master_table_version{0};
    int curr_originating_center{0};
    int curr_originating_subcenter{0};
    int curr_local_table_version{0};

    TableA tablea;
    TableB tableb;
    TableD tabled;
    TableF tablef;

    unsigned int total_num_messages{0};
    unsigned int num_table_messages{0};
    bool has_builtin_tables{false};
};

BUFRFile::BUFRFile(const std::string& filename)
    : d(new PrivateData)
{
    d->curr_master_table_number = -1;
    d->curr_master_table_version = -1;
    d->curr_originating_center = -1;
    d->curr_originating_subcenter = -1;
    d->curr_local_table_version = -1;
    d->num_table_messages = 0;
    d->has_builtin_tables = false;

    d->ifile.open(filename.c_str(), std::ios::in | std::ios::binary);
    if (!d->ifile) {
        std::ostringstream ostr;
        ostr << "Error openning file: " << filename;
        throw std::runtime_error(ostr.str());
    }

    d->ifile.seekg(0, std::ios::end);
    const std::ios::pos_type filesize = d->ifile.tellg();
    std::ios::pos_type pos = 0;

    while (!d->ifile.eof() && pos < filesize) {
        size_t len_bufr;
        pos = seek_bufr(d->ifile, pos, len_bufr);
        if (pos < 0) {
            break;
        }
        d->offset.push_back(pos);
        pos = pos + (std::ios::pos_type)len_bufr;
    }

    if (d->offset.empty()) {
        std::ostringstream ostr;
        ostr << "There are no BUFR messages in this file: " << filename;
        throw std::runtime_error(ostr.str());
    }

    d->total_num_messages = (unsigned int)d->offset.size();

    int current_message = 0;

    // loop until we load all data_cat==11 messages
    for (;;) {

        BUFRMessage bm;
        bm.parse(d->ifile, d->offset[current_message]);

        current_message++;

        if (current_message == 1) {
            if (bm.data_cat() == 11) {
                // add to TableB set of standard tableb descriptors used to build BUFR table entries
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 1, "TABLAE  ", "Table A: entry", "CCITT_IA5", 0, 0, 24));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 2, "TABLAD1 ", "Table A: data category description, line 1", "CCITT_IA5", 0, 0, 256));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 3, "TABLAD2 ", "Table A: data category description, line 2", "CCITT_IA5", 0, 0, 256));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 4, "MTABL   ", "BUFR/CREX Master table (see Note 1)", "CCITT_IA5", 0, 0, 16));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 5, "BUFREDN ", "BUFR/CREX edition number", "CCITT_IA5", 0, 0, 24));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 6, "BMTVN   ", "BUFR Master table Version number (see Note 2)", "CCITT_IA5", 0, 0, 16));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 7, "CMTVN   ", "CREX Master table version number (see Note 3)", "CCITT_IA5", 0, 0, 16));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 8, "BLTVN   ", "BUFR Local table version number (see Note 4)", "CCITT_IA5", 0, 0, 16));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 10, "FDESC   ", "F descriptor to be added or defined", "CCITT_IA5", 0, 0, 8));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 11, "XDESC   ", "X descriptor to be added or defined", "CCITT_IA5", 0, 0, 16));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 12, "YDESC   ", "Y descriptor to be added or defined", "CCITT_IA5", 0, 0, 24));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 13, "ELEMNA1 ", "Element name, line 1", "CCITT_IA5", 0, 0, 256));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 14, "ELEMNA2 ", "Element name, line 2", "CCITT_IA5", 0, 0, 256));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 15, "UNITSNA ", "Units name", "CCITT_IA5", 0, 0, 192));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 16, "SCALESG ", "Units scale sign", "CCITT_IA5", 0, 0, 8));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 17, "SCALEU  ", "Units scale", "CCITT_IA5", 0, 0, 24));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 18, "REFERSG ", "Units reference sign", "CCITT_IA5", 0, 0, 8));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 19, "REFERVA ", "Units reference value", "CCITT_IA5", 0, 0, 80));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 20, "ELEMDWD ", "Element data width", "CCITT_IA5", 0, 0, 24));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 24, "CODFIG  ", "Code figure", "CCITT_IA5", 0, 0, 64));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 25, "CODFIGM ", "Code figure meaning", "CCITT_IA5", 0, 0, 496));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 26, "BITNUM  ", "Bit number", "CCITT_IA5", 0, 0, 48));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 27, "BITNUMM ", "Bit number meaning", "CCITT_IA5", 0, 0, 496));
                d->tableb.add_descriptor(DescriptorTableB(0, 0, 30, "DDSEQ   ", "Descriptor defining sequence", "CCITT_IA5", 0, 0, 48));

                d->tablef.set_versions(bm.master_table_number(),
                                       bm.master_table_version(),
                                       bm.originating_center(),
                                       bm.originating_subcenter(),
                                       bm.local_table_version());

            } else {

                d->tableb.set_versions(bm.master_table_number(),
                                       bm.master_table_version(),
                                       bm.originating_center(),
                                       bm.originating_subcenter(),
                                       bm.local_table_version());
                d->tableb.read_from_db();

                d->tabled.set_versions(bm.master_table_number(),
                                       bm.master_table_version(),
                                       bm.originating_center(),
                                       bm.originating_subcenter(),
                                       bm.local_table_version());
                d->tabled.read_from_db();

                d->tablef.set_versions(bm.master_table_number(),
                                       bm.master_table_version(),
                                       bm.originating_center(),
                                       bm.originating_subcenter(),
                                       bm.local_table_version());
                // d->tablef.read_from_db();

                d->curr_master_table_number = bm.master_table_number();
                d->curr_master_table_version = bm.master_table_version();
                d->curr_originating_center = bm.originating_center();
                d->curr_originating_subcenter = bm.originating_subcenter();
                d->curr_local_table_version = bm.local_table_version();
            }
        }

        bm.set_tables(&d->tablea, &d->tableb, &d->tabled, &d->tablef);

        if (bm.data_cat() == 11) {
            if (bm.load_tables() == 0) {
                break;
            }
            d->num_table_messages++;
            d->has_builtin_tables = true;
        } else {
            // done loading all data_cat == 11 messages
            // assuming they are all at the beginning of the file before any other message
            break;
        }
    }
}

BUFRFile::~BUFRFile() = default;

BUFRMessage BUFRFile::get_message_num(const unsigned int message_num)
{
    // skip first few messages that contain bufr tables
    // const unsigned int actual_message_num = d->num_table_messages + message_num - 1;
    const unsigned int actual_message_num = message_num - 1;

    if (actual_message_num >= d->total_num_messages) {
        std::ostringstream estr;
        estr << " Incorrect message number " << message_num << " actual_message_number " << actual_message_num;
        throw std::runtime_error(estr.str());
    }

    BUFRMessage bm;
    bm.parse(d->ifile, d->offset[actual_message_num]);

    if (!d->has_builtin_tables) {
        // reload tables in case different messages use different tables
        if (d->curr_master_table_number != bm.master_table_number()
            || d->curr_master_table_version != bm.master_table_version()
            || d->curr_originating_center != bm.originating_center()
            || d->curr_originating_subcenter != bm.originating_subcenter()
            || d->curr_local_table_version != bm.local_table_version()) {

            d->tableb.set_versions(bm.master_table_number(),
                                   bm.master_table_version(),
                                   bm.originating_center(),
                                   bm.originating_subcenter(),
                                   bm.local_table_version());
            d->tableb.read_from_db();

            d->tabled.set_versions(bm.master_table_number(),
                                   bm.master_table_version(),
                                   bm.originating_center(),
                                   bm.originating_subcenter(),
                                   bm.local_table_version());
            d->tabled.read_from_db();

            d->tablef.set_versions(bm.master_table_number(),
                                   bm.master_table_version(),
                                   bm.originating_center(),
                                   bm.originating_subcenter(),
                                   bm.local_table_version());
            // d->tablef.readFromDB();

            d->curr_master_table_number = bm.master_table_number();
            d->curr_master_table_version = bm.master_table_version();
            d->curr_originating_center = bm.originating_center();
            d->curr_originating_subcenter = bm.originating_subcenter();
            d->curr_local_table_version = bm.local_table_version();
        }
    }

    bm.set_tables(&d->tablea, &d->tableb, &d->tabled, &d->tablef);

    return bm;
}

unsigned int BUFRFile::num_messages() const
{
    return d->total_num_messages; //  - d->num_table_messages;
}

unsigned int BUFRFile::num_table_messages() const
{
    return d->num_table_messages;
}

std::string BUFRFile::get_tableb_name() const
{
    return d->tableb.get_master_table_name() + "/" + d->tableb.get_local_table_name();
}

std::string BUFRFile::get_tabled_name() const
{
    return d->tabled.get_master_table_name() + "/" + d->tabled.get_local_table_name();
}

std::string BUFRFile::get_tablef_name() const
{
    return d->tablef.get_master_table_name() + "/" + d->tablef.get_local_table_name();
}

bool BUFRFile::has_builtin_tables() const
{
    return d->has_builtin_tables;
}

void BUFRFile::dump_tables(std::ostream& ostr)
{
    d->tablea.dump(ostr);
    d->tabled.dump1(d->tablea, ostr);
    d->tableb.dump1(ostr);
    d->tabled.dump2(d->tableb, ostr);
    d->tableb.dump2(ostr);
}
