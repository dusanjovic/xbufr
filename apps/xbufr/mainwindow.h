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

#include "bufrfile.h"
#include "nodemodel.h"
#include "valuesmodel.h"

#include <QItemSelectionModel>
#include <QMainWindow>

namespace Ui
{
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = 0);
    ~MainWindow();

    void load_file(const QString& filename);

private Q_SLOTS:
    void on_prevMessageButton_clicked();
    void on_firstMessageButton_clicked();
    void on_nextMessageButton_clicked();
    void on_lastMessageButton_clicked();
    void on_messageSpinBox_valueChanged(int arg1);

    void on_firstSubsetButton_clicked();
    void on_prevSubsetButton_clicked();
    void on_nextSubsetButton_clicked();
    void on_lastSubsetButton_clicked();
    void on_subsetSpinBox_valueChanged(int arg1);

    void open_file();
    void show_about_dialog();

    void treeview_expand_all();
    void treeview_collapse_all();

    void on_dumpTablesButton_clicked();

    void table_selection_changed(const QItemSelection& selected, const QItemSelection& deselected);
    void tree_selection_changed(const QItemSelection& selected, const QItemSelection& deselected);

    void on_treeView_customContextMenuRequested(const QPoint& pos);
    void treeview_header_context_menu(const QPoint& pos);

    void on_actionShow_BUFR_table_messages_toggled(bool arg1);

protected:
    virtual void changeEvent(QEvent* event);

private:
    void load_message(const int message_num);
    void load_subset(const int subset_num);

    void update_tree_view(const int subset_num);

    Ui::MainWindow* ui{nullptr};

    BUFRFile* bufrfile{nullptr};
    NodeModel node_model{};
    ValuesModel values_model{};

    int number_of_messages{0};
    int number_of_subsets{0};

    BUFRMessage loaded_message{};
    int current_message{0};
    int current_subset{0};
    int start_message{0};
    int end_message{0};
};
