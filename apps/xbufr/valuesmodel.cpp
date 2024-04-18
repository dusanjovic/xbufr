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

#include "valuesmodel.h"

#include <iomanip>
#include <iostream>
#include <sstream>

ValuesModel::ValuesModel(QObject* parent)
    : QAbstractTableModel(parent)
{
}

QVariant ValuesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Orientation::Vertical) {
        const Item& item = m_data_nodes[section][0]->data();
        if (role == Qt::DisplayRole) {
            const int f = (item.fxy >> 14) & 0x3;
            const int x = (item.fxy >> 8) & 0x3f;
            const int y = item.fxy & 0xff;
            std::ostringstream strstrm;
            strstrm << std::setfill('0') << std::setw(1) << f << std::setw(2) << x << std::setw(3) << y;
            if (!item.mnemonic.empty()) {
                strstrm << " " << item.mnemonic;
            }
            return QString(strstrm.str().c_str());
        }
        if (role == Qt::ToolTipRole) {
            return QString(item.description.c_str());
        }
        return {};
    }

    if (role == Qt::DisplayRole) {
        return QString::number(section + 1);
    }
    return {};
}

int ValuesModel::rowCount(const QModelIndex& /*parent*/) const
{
    return m_data_nodes.size();
}

int ValuesModel::columnCount(const QModelIndex& /*parent*/) const
{
    if (!m_data_nodes.empty()) {
        return m_data_nodes[0].size();
    }
    return 0;
}

QVariant ValuesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return {};
    }

    if (role == Qt::DisplayRole) {
        const Item& item = m_data_nodes[index.row()][index.column()]->data();
        if (item.missing) {
            return QString("MISSING");
        }
        assert(!item.values.empty());
        const Item::Value value = item.values[index.column()];
        if (value.type == Item::ValueType::Double) {
            const QString unit(item.unit.c_str());
            if (unit.startsWith("FLAG", Qt::CaseInsensitive)) {
                return QString::number((int)value.d);
            }
            return QString::number(value.d);
        }
        return QString(value.s.c_str()).trimmed();
    }

    if (role == Qt::UserRole) {
        auto* ni = const_cast<NodeItem*>(m_data_nodes[index.row()][index.column()]);
        return QVariant::fromValue(ni);
    }

    return {};
}

std::vector<std::vector<const NodeItem*>>& ValuesModel::data_nodes()
{
    return m_data_nodes;
}

void ValuesModel::begin_reset()
{
    for (auto& v_row : m_data_nodes) {
        v_row.clear();
    }
    m_data_nodes.clear();
    beginResetModel();
}

void ValuesModel::end_reset()
{
    endResetModel();

    for (int i = 0; i < rowCount({}); ++i) {
        item_map.insert(m_data_nodes[i][0], createIndex(i, 0));
    }
}

QModelIndex ValuesModel::index_from_node(const NodeItem* ni) const
{
    return item_map[ni];
}
