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

#include "mainwindow.h"

#include "ui_mainwindow.h"
#include "versiondialog.h"

#include <QDebug>
#include <QFileDialog>
#include <QFontDatabase>
#include <QMessageBox>
#include <QScreen>
#include <QScrollBar>
#include <QShortcut>

#include <sstream>
#include <stdexcept>

#if (QT_VERSION < QT_VERSION_CHECK(5, 13, 0))
static void expandChildren(const QModelIndex& index, QTreeView* view)
{
    if (!index.isValid()) {
        return;
    }

    int childCount = index.model()->rowCount(index);
    for (int i = 0; i < childCount; i++) {
        const QModelIndex& child = index.model()->index(i, 0, index);
        expandChildren(child, view);
    }

    if (!view->isExpanded(index)) {
        view->expand(index);
    }
}
#endif

static void collapseChildren(const QModelIndex& index, QTreeView* view)
{
    if (!index.isValid()) {
        return;
    }

    if (view->isExpanded(index)) {
        view->collapse(index);
    }

    int childCount = index.model()->rowCount(index);
    for (int i = 0; i < childCount; i++) {
        const QModelIndex& child = index.model()->index(i, 0, index);
        collapseChildren(child, view);
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->treeView->setModel(&node_model);
    ui->treeView->header()->setStretchLastSection(false);
    // ui->treeView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->treeView->header()->setSectionResizeMode(3, QHeaderView::Stretch);
    ui->treeView->setColumnWidth(0, 250);
    ui->treeView->setColumnWidth(1, 100);
    ui->treeView->setColumnWidth(4, 50);
    ui->treeView->setColumnWidth(5, 80);
    ui->treeView->setColumnWidth(6, 50);
    ui->treeView->setColumnWidth(7, 100);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeView->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView->header(), &QHeaderView::customContextMenuRequested, this, &MainWindow::treeview_header_context_menu);

    ui->tableView->setModel(&values_model);

#if defined(Q_OS_LINUX) && defined(QT_NO_FONTCONFIG)
    if (QFontDatabase::addApplicationFont(":/fonts/MyFont-subset.ttf") == -1) {
        std::cerr << "Error loading Font" << std::endl;
    } else {
        QFont f("DejaVu Sans", 10);
        // force full hinting (when we compile (staticaly) without full fontconfig support)
        f.setHintingPreference(QFont::PreferFullHinting);
        QApplication::setFont(f);
    }
    if (QFontDatabase::addApplicationFont(":/fonts/MyFontMono-subset.ttf") == -1) {
        std::cerr << "Error loading FontMono" << std::endl;
    }
    ui->treeView->setFont(QFont("DejaVu Sans Mono", 10));
    ui->tableView->setFont(QFont("DejaVu Sans Mono", 10));
    ui->textEdit->setFont(QFont("DejaVu Sans Mono", 10));
#else
#ifdef Q_OS_WIN32
    QApplication::setFont(QFontDatabase::systemFont(QFontDatabase::GeneralFont));
    QFont fixed_font = QFont("Consolas");
    fixed_font.setPointSize(10);
#else
    int pt = QFontDatabase::systemFont(QFontDatabase::GeneralFont).pointSize();
    QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixed_font.setPointSize(pt);
#endif
    ui->treeView->setFont(fixed_font);
    ui->tableView->setFont(fixed_font);
    ui->textEdit->setFont(fixed_font);
#endif

    resize(QGuiApplication::primaryScreen()->availableSize() * 0.8);
    move(QGuiApplication::primaryScreen()->availableGeometry().center() - frameGeometry().center());

    QList<int> sizes;
    sizes << 330 << 400;
    ui->splitter->setSizes(sizes);
    ui->splitter->setStretchFactor(0, 0);
    ui->splitter->setStretchFactor(1, 1);

    ui->firstMessageButton->setEnabled(false);
    ui->prevMessageButton->setEnabled(false);
    ui->messageSpinBox->setEnabled(false);
    ui->nextMessageButton->setEnabled(false);
    ui->lastMessageButton->setEnabled(false);

    ui->firstSubsetButton->setEnabled(false);
    ui->prevSubsetButton->setEnabled(false);
    ui->subsetSpinBox->setEnabled(false);
    ui->nextSubsetButton->setEnabled(false);
    ui->lastSubsetButton->setEnabled(false);

    ui->hasBuiltinTablesLabel->hide();
    ui->dumpTablesButton->hide();

    auto* shortcut_prev_message = new QShortcut(QKeySequence(Qt::Key_J), this, SLOT(on_prevMessageButton_clicked()));
    Q_UNUSED(shortcut_prev_message)
    auto* shortcut_next_message = new QShortcut(QKeySequence(Qt::Key_L), this, SLOT(on_nextMessageButton_clicked()));
    Q_UNUSED(shortcut_next_message)

    auto* shortcut_prev_subset = new QShortcut(QKeySequence(Qt::Key_K), this, SLOT(on_prevSubsetButton_clicked()));
    Q_UNUSED(shortcut_prev_subset)
    auto* shortcut_next_subset = new QShortcut(QKeySequence(Qt::Key_I), this, SLOT(on_nextSubsetButton_clicked()));
    Q_UNUSED(shortcut_next_subset)

    connect(ui->tableView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(table_selection_changed(QItemSelection, QItemSelection)));
    connect(ui->treeView->selectionModel(), SIGNAL(selectionChanged(QItemSelection, QItemSelection)), this, SLOT(tree_selection_changed(QItemSelection, QItemSelection)));
}

MainWindow::~MainWindow()
{
    delete bufrfile;
    delete ui;
}

void MainWindow::load_file(const QString& filename)
{
    QFileInfo info(filename);
    if (!info.isFile()) {
        QMessageBox msgBox;
        msgBox.setText("File " + filename + " is not a regular file");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    if (!info.exists()) {
        QMessageBox msgBox;
        msgBox.setText("File " + filename + " does not exist");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    if (!info.isReadable()) {
        QMessageBox msgBox;
        msgBox.setText("File " + filename + " is not readable");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }

    int message_num_to_load = 1;
    try {

        bufrfile = new BUFRFile(filename.toStdString());
        if (bufrfile->num_messages() > 0) {
            if (bufrfile->has_builtin_tables() && !ui->actionShow_BUFR_table_messages->isChecked()) {
                start_message = bufrfile->num_table_messages() + 1;
            } else {
                start_message = 1;
            }
            message_num_to_load = start_message;
            end_message = bufrfile->num_messages();
            load_message(message_num_to_load);
        } else {
            throw std::runtime_error("There are 0 messages in a file");
        }

    } catch (const std::exception& e) {
        QMessageBox msgBox;
        msgBox.setText(e.what());
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.exec();
        return;
    }

    number_of_messages = bufrfile->num_messages();
    QString totalMessages = " (" + QString::number(number_of_messages) + ")";
    ui->totalMessagesLabel->setText(totalMessages);
    ui->messageSpinBox->blockSignals(true);
    ui->messageSpinBox->setMinimum(start_message);
    ui->messageSpinBox->setMaximum(end_message);
    if (number_of_messages > 1) {
        ui->firstMessageButton->setEnabled(true);
        ui->prevMessageButton->setEnabled(true);
        ui->messageSpinBox->setEnabled(true);
        ui->nextMessageButton->setEnabled(true);
        ui->lastMessageButton->setEnabled(true);
    }
    ui->messageSpinBox->blockSignals(false);

    const QFileInfo file_info(filename);
    setWindowTitle("BUFR Viewer - " + file_info.fileName());
    ui->filenameLabel->setText("File: " + file_info.fileName());

    if (bufrfile->has_builtin_tables()) {
        ui->hasBuiltinTablesLabel->show();
        ui->dumpTablesButton->show();
    } else {
        ui->hasBuiltinTablesLabel->hide();
        ui->dumpTablesButton->hide();
    }
}

void MainWindow::load_message(const int message_num)
{
    if (bufrfile == nullptr) {
        return;
    }

    try {

        std::stringstream ostr;
        ostr << "Message: " << message_num;

        auto* root_nodeitem = new NodeItem;

        auto* message_nodeitem = root_nodeitem->add_child();
        Item& item = message_nodeitem->data();
        item.name = ostr.str();
        item.description = "";

        loaded_message = bufrfile->get_message_num(message_num);

        loaded_message.decode_data(message_nodeitem);

        values_model.begin_reset();
        loaded_message.get_values_for_subset(values_model.data_nodes(), 1);
        values_model.end_reset();
        ui->tableView->reset();

        std::stringstream bufr_sections_ostream;

        loaded_message.dump_section_0(bufr_sections_ostream);
        loaded_message.dump_section_1(bufr_sections_ostream);
        loaded_message.dump_section_2(bufr_sections_ostream);
        loaded_message.dump_section_3(bufr_sections_ostream);
        loaded_message.dump_section_4(bufr_sections_ostream);
        loaded_message.dump_section_5(bufr_sections_ostream);

        ui->textEdit->setText(bufr_sections_ostream.str().c_str());
        ui->textEdit->verticalScrollBar()->setValue(0);

        number_of_subsets = loaded_message.number_of_subsets();

        QString totalSubsets = " (" + QString::number(number_of_subsets) + ")";
        ui->totalSubsetsLabel->setText(totalSubsets);

        node_model.set_root_node(root_nodeitem);

        if (number_of_subsets > 1) {
            ui->subsetSpinBox->blockSignals(true);
            ui->subsetSpinBox->setMaximum(1);
            ui->subsetSpinBox->setMaximum(number_of_subsets);
            ui->subsetSpinBox->blockSignals(false);

            ui->firstSubsetButton->setEnabled(true);
            ui->prevSubsetButton->setEnabled(true);
            ui->subsetSpinBox->setEnabled(true);
            ui->nextSubsetButton->setEnabled(true);
            ui->lastSubsetButton->setEnabled(true);
        } else {
            ui->firstSubsetButton->setEnabled(false);
            ui->prevSubsetButton->setEnabled(false);
            ui->subsetSpinBox->setEnabled(false);
            ui->nextSubsetButton->setEnabled(false);
            ui->lastSubsetButton->setEnabled(false);
        }

    } catch (const std::exception& e) {
        QMessageBox msgBox(QMessageBox::Icon::Warning, QString("xbufr - Warning"), QString(e.what()));
        msgBox.exec();
    }

    current_message = message_num;
    current_subset = 1;

    ui->messageSpinBox->blockSignals(true);
    ui->messageSpinBox->setValue(current_message);
    if (current_message <= (int)bufrfile->num_table_messages()) {
        QPalette pal = ui->messageSpinBox->palette();
        pal.setColor(QPalette::Text, Qt::red);
        ui->messageSpinBox->setPalette(pal);
    } else {
        QPalette pal = ui->messageSpinBox->palette();
        pal.setColor(QPalette::Text, palette().color(QPalette::Text));
        ui->messageSpinBox->setPalette(pal);
    }
    ui->messageSpinBox->blockSignals(false);

    ui->subsetSpinBox->blockSignals(true);
    ui->subsetSpinBox->setValue(current_subset);
    ui->subsetSpinBox->blockSignals(false);

    const std::string loaded_tables = bufrfile->get_tableb_name() + "   " + bufrfile->get_tabled_name() + "   " + bufrfile->get_tablef_name();
    ui->statusBar->showMessage(QString(loaded_tables.c_str()));

    update_tree_view(current_subset);
}

void MainWindow::load_subset(const int subset_num)
{
    if (bufrfile == nullptr) {
        return;
    }

    if (current_subset == subset_num) {
        return;
    }
    current_subset = subset_num;

    values_model.begin_reset();
    loaded_message.get_values_for_subset(values_model.data_nodes(), subset_num);
    values_model.end_reset();

    ui->tableView->reset();

    ui->subsetSpinBox->blockSignals(true);
    ui->subsetSpinBox->setValue(current_subset);
    ui->subsetSpinBox->blockSignals(false);

    update_tree_view(current_subset);
}

void MainWindow::update_tree_view(const int subset_num)
{
    assert(subset_num > 0);

    ui->treeView->expandAll();

    // scroll to subset_num
    const QModelIndex message_index = node_model.index(0, 0, QModelIndex());
    const QModelIndex subset_index = node_model.index(subset_num - 1, 0, message_index);
    if (!subset_index.isValid()) {
        return;
    }
    ui->treeView->scrollTo(subset_index, QAbstractItemView::ScrollHint::PositionAtTop);
    ui->treeView->selectionModel()->select(subset_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void MainWindow::on_prevMessageButton_clicked()
{
    if (current_message > start_message) {
        load_message(current_message - 1);
    }
}

void MainWindow::on_firstMessageButton_clicked()
{
    load_message(start_message);
}

void MainWindow::on_nextMessageButton_clicked()
{
    if (current_message < end_message) {
        load_message(current_message + 1);
    }
}

void MainWindow::on_lastMessageButton_clicked()
{
    load_message(end_message);
}

void MainWindow::on_messageSpinBox_valueChanged(int arg1)
{
    load_message(arg1);
}

void MainWindow::on_firstSubsetButton_clicked()
{
    load_subset(1);
}

void MainWindow::on_prevSubsetButton_clicked()
{
    if (current_subset > 1) {
        load_subset(current_subset - 1);
    }
}

void MainWindow::on_nextSubsetButton_clicked()
{
    if (current_subset < number_of_subsets) {
        load_subset(current_subset + 1);
    }
}

void MainWindow::on_lastSubsetButton_clicked()
{
    load_subset(number_of_subsets);
}

void MainWindow::on_subsetSpinBox_valueChanged(int arg1)
{
    load_subset(arg1);
}

void MainWindow::open_file()
{
    QString filename = QFileDialog::getOpenFileName(this, "Open BUFR file");
    if (filename.length() == 0) {
        return;
    }
    load_file(filename);
}

void MainWindow::show_about_dialog()
{
    VersionDialog versionDialog(this);
    versionDialog.exec();
}

void MainWindow::on_dumpTablesButton_clicked()
{
    if (bufrfile == nullptr) {
        return;
    }

    QTextEdit te;
#if defined(Q_OS_LINUX) && defined(QT_NO_FONTCONFIG)
    te.setFont(QFont("DejaVu Sans Mono", 10));
#else
#ifdef Q_OS_WIN32
    QFont fixed_font = QFont("Consolas");
    fixed_font.setPointSize(10);
#else
    int pt = QFontDatabase::systemFont(QFontDatabase::GeneralFont).pointSize();
    QFont fixed_font = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    fixed_font.setPointSize(pt);
#endif
    te.setFont(fixed_font);
#endif
    te.setReadOnly(true);

    std::stringstream tables_ostream;
    bufrfile->dump_tables(tables_ostream);
    te.setText(tables_ostream.str().c_str());

    auto* table_dialog = new QDialog(this);
    QLayout* layoutWidget = new QVBoxLayout(table_dialog);
    layoutWidget->addWidget(&te);
    table_dialog->resize(800, 800);
    table_dialog->setWindowTitle("BUFR tables");

    table_dialog->exec();
}

void MainWindow::table_selection_changed(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)

    if (selected.isEmpty()) {
        return;
    }
    if (selected.indexes().isEmpty()) {
        return;
    }

    const QModelIndex index = selected.indexes().at(0);
    auto* ni = ui->tableView->model()->data(index, Qt::UserRole).value<NodeItem*>();
    const QModelIndex tree_index = ((NodeModel*)ui->treeView->model())->index_from_node(ni);

    ui->treeView->scrollTo(tree_index);
    ui->treeView->selectionModel()->select(tree_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void MainWindow::tree_selection_changed(const QItemSelection& selected, const QItemSelection& deselected)
{
    Q_UNUSED(deselected)

    if (selected.isEmpty()) {
        return;
    }
    if (selected.indexes().isEmpty()) {
        return;
    }
    const QModelIndex index = selected.indexes().at(0);

    // search for 'Subset' node
    QModelIndex parent_index = index.parent();
    while (parent_index.isValid()) {
        const QString name = ((NodeModel*)ui->treeView->model())->node_from_index(parent_index)->data().name.c_str();
        if (name.startsWith("Subset:")) {
            QStringList parts = name.split(' ');
            assert(parts.size() == 2);
            int found_subset = parts[1].toInt();
            if (found_subset != current_subset) {
                load_subset(found_subset);
            }
            break;
        }
        parent_index = parent_index.parent();
    }

    const NodeItem* ni = ((const NodeModel*)ui->treeView->model())->node_from_index(index);
    if (ni->data().type == Item::Type::Element) {
        const QModelIndex table_index = ((ValuesModel*)ui->tableView->model())->index_from_node(ni);
        ui->tableView->scrollTo(table_index);
        ui->tableView->selectionModel()->select(table_index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    } else {
        ui->tableView->selectionModel()->clear();
    }
}

void MainWindow::on_treeView_customContextMenuRequested(const QPoint& pos)
{
    QModelIndex index = ui->treeView->indexAt(pos);
    if (index.isValid() && index.model()->rowCount(index) > 0) {
        QMenu menu(this);
        auto* expandAllAction = new QAction("Expand all", this);
        connect(expandAllAction, SIGNAL(triggered()), this, SLOT(treeview_expand_all()));
        menu.addAction(expandAllAction);
        auto* collapseAllAction = new QAction("Collapse all", this);
        connect(collapseAllAction, SIGNAL(triggered()), this, SLOT(treeview_collapse_all()));
        menu.addAction(collapseAllAction);
        menu.exec(ui->treeView->viewport()->mapToGlobal(pos));
    }
}

void MainWindow::treeview_header_context_menu(const QPoint& pos)
{
    const auto& header = ui->treeView->header();
    QMenu menu(header);

    for (int i = 0; i < header->count(); ++i) {
        auto* action = menu.addAction(ui->treeView->model()->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString(), [i, this]() {
            ui->treeView->setColumnHidden(i, !ui->treeView->isColumnHidden(i));
        });
        action->setCheckable(true);
        action->setChecked(!ui->treeView->isColumnHidden(i));
    }

    menu.exec(ui->treeView->mapToGlobal(pos));
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::PaletteChange) {
        node_model.adjust_colors();
    }
}

void MainWindow::treeview_expand_all()
{
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }
#if (QT_VERSION >= QT_VERSION_CHECK(5, 13, 0))
    ui->treeView->expandRecursively(index);
#else
    expandChildren(index, ui->treeView);
#endif
}

void MainWindow::treeview_collapse_all()
{
    QModelIndex index = ui->treeView->selectionModel()->currentIndex();
    if (!index.isValid()) {
        return;
    }
    collapseChildren(index, ui->treeView);
}

void MainWindow::on_actionShow_BUFR_table_messages_toggled(bool arg1)
{
    if (arg1) { // show all messages, including tables
        start_message = 1;
    } else { // show only data messages
        start_message = bufrfile->num_table_messages() + 1;
    }

    ui->messageSpinBox->blockSignals(true);
    ui->messageSpinBox->setMinimum(start_message);
    ui->messageSpinBox->blockSignals(false);

    if (start_message > current_message) {
        load_message(start_message);
    }
}
