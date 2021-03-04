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

#include <cassert>
#include <vector>

template <class T>
class Node
{
public:
    Node() = default;

    ~Node()
    {
        for (unsigned int i = 0; i < m_children.size(); i++) {
            delete m_children[i];
        }
    }

    T& data()
    {
        return m_data;
    }

    const T& data() const
    {
        return m_data;
    }

    int index_of(const Node* const node) const
    {
        for (unsigned int i = 0; i < m_children.size(); i++) {
            if (m_children[i] == node) {
                return i;
            }
        }
        return -1;
    }

    unsigned int num_children() const
    {
        return (unsigned int)m_children.size();
    }

    Node* child(unsigned int i) const
    {
        assert(i < num_children());
        return m_children[i];
    }

    Node* parent() const
    {
        return (m_parent);
    }

    void set_parent(Node* const parent)
    {
        m_parent = parent;
    }

    bool has_children() const
    {
        return (num_children() > 0);
    }

    std::vector<Node*>& children() const
    {
        return m_children;
    }

    int row() const
    {
        if (m_parent) {
            return m_parent->index_of(this);
        }
        return 0;
    }

    Node* add_child()
    {
        Node* child_node = new Node();
        child_node->set_depth(m_depth + 1);
        child_node->set_parent(this);
        m_children.push_back(child_node);
        return child_node;
    }

    void set_depth(const int depth)
    {
        m_depth = depth;
    }

    int depth() const
    {
        return m_depth;
    }

private:
    Node(const Node&) = delete;
    Node& operator=(Node const&) = delete;

    std::vector<Node*> m_children{};
    Node* m_parent{nullptr};
    T m_data{};
    int m_depth{0};
};
