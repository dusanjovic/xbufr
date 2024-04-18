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

#include "versiondialog.h"

#include "version.h"

#include <QDate>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>

#include <sstream>

#if defined __GLIBC__
#include <gnu/libc-version.h>
#endif

#define STRINGIFY_INTERNAL(x) #x
#define STRINGIFY(x) STRINGIFY_INTERNAL(x)

const char* const VERSION_STR = STRINGIFY(VERSION_MAJOR.VERSION_MINOR.VERSION_PATCH);
const char* const AUTHOR = "Dusan Jovic";
const char* const YEAR = "2021";

#ifdef GIT_REVISION
const char* const GIT_REVISION_STR = STRINGIFY(GIT_REVISION);
#else
const char* const GIT_REVISION_STR = "";
#endif

#undef STRINGIFY
#undef STRINGIFY_INTERNAL

template <typename T>
std::string toString(const T& value)
{
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

VersionDialog::VersionDialog(QWidget* parent)
    : QDialog(parent)
{
    // We need to set the window icon explicitly here since for some reason the
    // application icon isn't used when the size of the dialog is fixed (at least not on X11/GNOME)
    setWindowIcon(QIcon(":/images/bufr.png"));

    setWindowTitle(tr("About xbufr"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    auto* layout = new QGridLayout(this);
    layout->setSizeConstraint(QLayout::SetFixedSize);

    const QString version = QLatin1String(VERSION_STR);

    QString appRev;
#ifdef GIT_REVISION
    const QString git_revision = QString::fromLatin1(GIT_REVISION_STR);
    if (!git_revision.isEmpty()) {
        appRev = tr("From revision %1<br/>").arg(git_revision);
    }
#endif

    std::string platform;
#ifdef _WIN32
    platform = "win32";
#elif defined __linux__
    platform = "linux";
#elif defined __APPLE__
    platform = "apple";
#elif defined __unix__
    platform = "unix";
#else
    platform = "unknown";
#endif

    std::string compiler;
#if defined(__clang__)
    compiler = "Clang " + toString(__clang_version__);
#elif defined(__GNUC__) || defined(__GNUG__)
    compiler = "GCC " + toString(__GNUC__) + "." + toString(__GNUC_MINOR__) + "." + toString(__GNUC_PATCHLEVEL__);
#else
    compiler = "unknown";
#endif

    std::string cplusplus;
#ifdef __cplusplus
    cplusplus = toString(__cplusplus);
#else
    cplusplus = "unknown";
#endif

    std::string libc;
#ifdef __GLIBC__
    libc = "GNU glibc: compiled on: " + toString(__GLIBC__) + "." + toString(__GLIBC_MINOR__) + " running on: " + toString(gnu_get_libc_version());
#endif

    const QString description = tr(
                                    "<h3>xbufr - BUFR Viewer %1</h3>"
                                    "Copyright 2015-%2 %3. All rights reserved.<br/>"
                                    "<br/>"
                                    "The program is provided AS IS with NO WARRANTY OF ANY KIND, "
                                    "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "
                                    "PARTICULAR PURPOSE.<br/><br/>"
                                    "%4<br/>")
                                    .arg(version,
                                         QLatin1String(YEAR),
                                         QLatin1String(AUTHOR),
                                         appRev)
                                + tr("Qt: %1<br/><br").arg(QLatin1String(QT_VERSION_STR))
                                + tr("Tool chain: %1<br/>").arg(QLatin1String(__VERSION__))
                                + tr("Platform: %1<br/>").arg(QString(platform.c_str()))
                                + tr("Compiler: %1<br/>").arg(QString(compiler.c_str()))
                                + tr("C++ version: %1<br/>").arg(QString(cplusplus.c_str()))
                                + tr("%1<br/>").arg(QString(libc.c_str()));

    auto* copyrightLabel = new QLabel(description);
    copyrightLabel->setWordWrap(true);
    copyrightLabel->setOpenExternalLinks(true);
    copyrightLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    QPushButton* closeButton = buttonBox->button(QDialogButtonBox::Close);
    buttonBox->addButton(closeButton, QDialogButtonBox::ButtonRole(QDialogButtonBox::RejectRole | QDialogButtonBox::AcceptRole));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    auto* logoLabel = new QLabel;
    logoLabel->setPixmap(QPixmap(":/images/bufr.png"));
    layout->addWidget(logoLabel, 0, 0, 1, 1);
    layout->addWidget(copyrightLabel, 0, 1, 4, 4);
    layout->addWidget(buttonBox, 4, 0, 1, 5);
}
