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

#include "myapplication.h"

#include <QFileOpenEvent>
#include <QMessageBox>

#include <exception>
#include <iostream>

MyApplication::MyApplication(int& argc, char** argv)
    : QApplication(argc, argv)
{
}

bool MyApplication::event(QEvent* event)
{
    if (event->type() == QEvent::FileOpen) {
        auto* file_open_event = static_cast<QFileOpenEvent*>(event);
        Q_EMIT fileOpenRequest(file_open_event->file());
        return true;
    }
    return QApplication::event(event);
}

bool MyApplication::notify(QObject* receiver, QEvent* event)
{
    try {
        return QApplication::notify(receiver, event);
    } catch (std::exception& ex) {
        QMessageBox::critical(nullptr, "Critical error", ex.what());
    } catch (int i) {
        QMessageBox::critical(nullptr, "Critical error", QString::number(i));
    }
    return false;
}
