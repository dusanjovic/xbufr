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

#include "nodemodel.h"

#include <QColor>
#include <QFont>
#include <QPalette>

NodeModel::NodeModel(QObject* parent)
    : QAbstractItemModel(parent)
{
    adjust_colors();
}

NodeModel::~NodeModel()
{
    delete root_node;
}

void NodeModel::set_root_node(NodeItem* node)
{
    beginResetModel();
    if (root_node != nullptr) {
        delete root_node;
        root_node = nullptr;
    }
    root_node = node;

    endResetModel();

    build_item_map(this->index(0, 0, {}));
}

QModelIndex NodeModel::index(int row, int column, const QModelIndex& parent) const
{
    if (root_node == nullptr) {
        return {};
    }
    const NodeItem* parentNode = node_from_index(parent);
    if (parentNode->num_children() > 0) {
        return createIndex(row, column, parentNode->child(row));
    }
    return {};
}

QModelIndex NodeModel::parent(const QModelIndex& index) const
{
    if (!index.isValid()) {
        return {};
    }

    auto* childItem = static_cast<NodeItem*>(index.internalPointer());
    NodeItem* parentItem = childItem->parent();

    if (parentItem == root_node) {
        return {};
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int NodeModel::rowCount(const QModelIndex& parent) const
{
    const NodeItem* parentNode = node_from_index(parent);
    if (parentNode == nullptr) {
        return 0;
    }
    return parentNode->num_children();
}

int NodeModel::columnCount(const QModelIndex& /* parent */) const
{
    return Column::__Count;
}

QVariant NodeModel::data(const QModelIndex& index, int role) const
{
    const NodeItem* node = node_from_index(index);
    if (node == nullptr) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
        case Column::Descriptor: {
            return QString(node->data().name.c_str());
        } break;
        case Column::Value: {
            const Item& item = node->data();
            if (item.missing) {
                return QString("MISSING");
            }
            if (!item.values.empty()) {
                QString str;
                const Item::Value value = item.values[0];
                if (value.type == Item::ValueType::Double) {
                    QString unit(item.unit.c_str());
                    if (unit.startsWith("FLAG", Qt::CaseInsensitive)) {
                        str += QString::number((int)value.d);
                    } else {
                        str += QString::number(value.d);
                    }
                } else {
                    str += QString(value.s.c_str()).trimmed();
                }
                return str;
            }
            return QString();
        } break;
        case Column::Unit: {
            return QString(node->data().unit.c_str());
        } break;
        case Column::Description: {
            QString description = node->data().description.c_str();
            if (description.startsWith("TABLE B ENTRY - ")) {
                return description.mid(16);
            }
            return description;
        } break;
        case Column::Scale: {
            const int scale = node->data().scale;
            if (scale != Item::undef_int_value) {
                return QString::number(scale);
            }
            return QString();
        } break;
        case Column::Reference: {
            const int ref_value = node->data().ref_value;
            if (ref_value != Item::undef_int_value) {
                return QString::number(ref_value);
            }
            return QString();
        } break;
        case Column::Bits: {
            const int bits = node->data().bits;
            if (bits != Item::undef_int_value) {
                return QString::number(bits);
            }
            return QString();
        } break;
        case Column::Range: {
            const size_t s = node->data().bits_range_start;
            const size_t e = node->data().bits_range_end;
            if (s == 0 && e == 0) {
                return QString();
            }
            return QString("[%1-%2]").arg(s).arg(e);
        } break;
        default: {
            assert(false);
            break;
        }
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (index.column() == 1) {
            return Qt::AlignRight;
        }
        return Qt::AlignLeft;
    }

    if (role == Qt::FontRole) {
        if (node->data().warning && (index.column() == 1 || index.column() == 2)) {
            QFont font;
            font.setBold(true);
            return font;
        }
    }

    if (role == Qt::BackgroundRole) {
        switch (node->data().type) {
        case Item::Type::Element:
            return element_bg_color;
            break;
        case Item::Type::Replicator:
            return replicator_bg_color;
            break;
        case Item::Type::Operator:
            return operator_bg_color;
            break;
        case Item::Type::Sequence:
            return sequence_bg_color;
            break;
        default:
            break;
        }
    }

    if (role == Qt::ForegroundRole) {

        switch (index.column()) {
        case Column::Value: {
            if (node->data().value_tooltip == "NOT FOUND") {
                return QColor(Qt::red);
            }
        } break;
        case Column::Scale: {
            if (node->data().new_scale) {
                return QColor(Qt::red);
            }
        } break;
        case Column::Reference: {
            if (node->data().new_ref_value) {
                return QColor(Qt::red);
            }
        } break;
        case Column::Bits: {
            if (node->data().new_bits) {
                return QColor(Qt::red);
            }
        } break;
        default:
            break;
        }

        switch (node->data().type) {
        case Item::Type::Element:
            return element_fg_color;
            break;
        case Item::Type::Replicator:
            return replicator_fg_color;
            break;
        case Item::Type::Operator:
            return operator_fg_color;
            break;
        case Item::Type::Sequence:
            return sequence_fg_color;
            break;
        default:
            break;
        }
    }

    if (role == Qt::ToolTipRole) {
        switch (index.column()) {
        case Column::Value:
            return QString(node->data().value_tooltip.c_str());
            break;
        default:
            break;
        }
    }

    return QVariant();
}

QVariant NodeModel::headerData(int column,
                               Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (column) {
        case Column::Descriptor:
            return tr("Descriptor");
            break;
        case Column::Value:
            return tr("Value");
            break;
        case Column::Unit:
            return tr("Unit");
            break;
        case Column::Description:
            return tr("Description");
            break;
        case Column::Scale:
            return tr("Scale");
            break;
        case Column::Reference:
            return tr("Reference");
            break;
        case Column::Bits:
            return tr("Bits");
            break;
        case Column::Range:
            return tr("Range (bits)");
            break;
        default: {
            assert(false);
            break;
        }
        }
    }

    if (role == Qt::TextAlignmentRole) {
        if (column == 1) {
            return Qt::AlignRight;
        }
        return Qt::AlignLeft;
    }

    return QVariant();
}

QModelIndex NodeModel::index_from_node(const NodeItem* ni) const
{
    return item_map[ni];
}

const NodeItem* NodeModel::node_from_index(const QModelIndex& index) const
{
    if (index.isValid()) {
        return static_cast<NodeItem*>(index.internalPointer());
    }
    return root_node;
}

void NodeModel::build_item_map(const QModelIndex& index)
{
    if (!index.isValid()) {
        return;
    }

    auto* item = static_cast<NodeItem*>(index.internalPointer());
    item_map.insert(item, index);

    for (int i = 0; i < rowCount(index); ++i) {
        build_item_map(this->index(i, 0, index));
    }
}

void NodeModel::adjust_colors()
{
    const QColor c = QPalette().base().color();
    const int r = c.red();
    const int g = c.green();
    const int b = c.blue();
    if (r > 128 && g > 128 && b > 128) {
        // light
        element_bg_color = QPalette().base().color();
        replicator_bg_color = QColor(230, 255, 204, 150);
        operator_bg_color = QColor(204, 229, 255, 150);
        sequence_bg_color = QColor(255, 229, 229, 150);

        element_fg_color = QPalette().windowText().color();
        replicator_fg_color = QPalette().windowText().color();
        operator_fg_color = QPalette().windowText().color();
        sequence_fg_color = QPalette().windowText().color();
    } else {
        // dark
        element_bg_color = QPalette().base().color();
        replicator_bg_color = QPalette().base().color();
        operator_bg_color = QPalette().base().color();
        sequence_bg_color = QPalette().base().color();

        element_fg_color = QPalette().windowText().color();
        replicator_fg_color = QColor(200, 255, 180);
        operator_fg_color = QColor(180, 200, 255);
        sequence_fg_color = QColor(255, 180, 180);
    }
}
