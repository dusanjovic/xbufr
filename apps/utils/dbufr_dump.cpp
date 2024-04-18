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

#include <iomanip>
#include <iostream>
#include <sstream>

static void dump_data(const NodeItem* ni,
                      std::ostream& output)
{
    const Item& item = ni->data();
    const int indent = std::max(ni->depth() - 1, 0);
    const std::string ind(indent * 2, ' ');

    const std::string name = (ind + item.name);
    output << std::setw(35) << std::left << name;

    output << " " << std::setw(15) << std::right;

    if (item.missing) {
        output << "MISSING";
    } else {
        if (!item.values.empty()) {
            const Item::Value value = item.values[0];
            if (value.type == Item::ValueType::Double) {
                output << value.d;
            } else {
                output << ("'" + value.s + "'");
            }
        } else {
            output << " ";
        }
    }

    output << " " << std::setw(20) << std::left << item.unit;
    output << " " << std::left << item.description;
    output << '\n';
    output << std::right;

    if (ni->has_children()) {
        for (unsigned int i = 0; i < ni->num_children(); i++) {
            dump_data(ni->child(i), output);
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc != 2) {
        std::cerr << "Usage: dbufr_dump <bufr_file>" << '\n';
        return 1;
    }

    try {
        BUFRFile bufr_file(argv[1]);

        // std::cout.setstate(std::ios_base::badbit);

        for (unsigned int i = 1; i <= bufr_file.num_messages(); i++) {
            std::ostringstream ostr;
            ostr << "Message: " << i;

            NodeItem message_nodeitem;
            Item& item = message_nodeitem.data();
            item.name = ostr.str();
            item.description = "...";

            std::vector<std::vector<const NodeItem*>> values_data_nodes;
            BUFRMessage m = bufr_file.get_message_num(i);
            m.decode_data(&message_nodeitem);

            m.dump_section_0(std::cout);
            m.dump_section_1(std::cout);
            m.dump_section_2(std::cout);
            m.dump_section_3(std::cout);
            m.dump_section_4(std::cout);
            m.dump_section_5(std::cout);

            for (int j = 1; j <= m.number_of_subsets(); j++) {
                m.get_values_for_subset(values_data_nodes, j);
            }
            dump_data(&message_nodeitem, std::cout);
            std::cout << "+++++++++" << '\n';
        }

    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return -1;
    } catch (...) {
        std::cerr << "exception" << '\n';
        return -1;
    }
    return 0;
}
