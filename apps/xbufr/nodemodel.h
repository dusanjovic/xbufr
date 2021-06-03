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

#include <QAbstractItemModel>
#include <QColor>
#include <QModelIndex>
#include <QVariant>

class NodeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    NodeModel(QObject* parent = nullptr);
    ~NodeModel();

    void set_root_node(NodeItem* node);

    QModelIndex index(int row, int column, const QModelIndex& parent) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    int rowCount(const QModelIndex& parent) const override;
    int columnCount(const QModelIndex& parent) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int column, Qt::Orientation orientation, int role) const override;

    QModelIndex index_from_node(const NodeItem* ni) const;
    const NodeItem* node_from_index(const QModelIndex& index) const;

    void adjust_colors();

private:
    const NodeItem* root_node{nullptr};

    QColor element_bg_color = QColor(255, 255, 229, 50);
    QColor replicator_bg_color = QColor(230, 255, 204, 50);
    QColor operator_bg_color = QColor(204, 229, 255, 50);
    QColor sequence_bg_color = QColor(255, 229, 229, 50);

    QColor element_fg_color = QColor(0, 0, 0);
    QColor replicator_fg_color = QColor(0, 0, 0);
    QColor operator_fg_color = QColor(0, 0, 0);
    QColor sequence_fg_color = QColor(0, 0, 0);

    QMap<const NodeItem*, QModelIndex> item_map;
    void build_item_map(const QModelIndex& index);

    enum Column : int {
        Descriptor = 0,
        Value,
        Unit,
        Description,
        Scale,
        Reference,
        Bits,
        Range,
        __Count
    };
};
