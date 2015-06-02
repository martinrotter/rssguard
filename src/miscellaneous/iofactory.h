// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#ifndef IOFACTORY_H
#define IOFACTORY_H

#include <QCoreApplication>

#if QT_VERSION >= 0x050000
#include <QStandardPaths>
#define SYSTEM_FOLDER_ENUM QStandardPaths
#else
#include <QDesktopServices>
#define SYSTEM_FOLDER_ENUM QDesktopServices
#endif


class IOFactory {
    Q_DECLARE_TR_FUNCTIONS(IOFactory)

  private:
    IOFactory();

  public:   
    // Returns system-wide folder according to type.
    static QString getSystemFolder(SYSTEM_FOLDER_ENUM::StandardLocation location);

    // Returns contents of a file.
    // Throws exception when no such file exists.
    static QByteArray readTextFile(const QString &file_path);

    // Copies file, overwrites destination.
    static bool copyFile(const QString &source, const QString &destination);
};

#endif // IOFACTORY_H
