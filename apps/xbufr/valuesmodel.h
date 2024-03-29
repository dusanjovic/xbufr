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

#include "item.h"

#include <QAbstractTableModel>

class ValuesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit ValuesModel(QObject* parent = nullptr);

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    std::vector<std::vector<const NodeItem*>>& data_nodes();

    void begin_reset();
    void end_reset();

    QModelIndex index_from_node(const NodeItem* ni) const;

private:
    std::vector<std::vector<const NodeItem*>> m_data_nodes;

    QMap<const NodeItem*, QModelIndex> item_map;
};

Q_DECLARE_METATYPE(NodeItem*)
