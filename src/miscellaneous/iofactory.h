// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "definitions/definitions.h"

#include <QStandardPaths>

class IOFactory {
  Q_DECLARE_TR_FUNCTIONS(IOFactory)

  private:
    IOFactory();

  public:
    static bool isFolderWritable(const QString& folder);

    // Returns system-wide folder according to type.
    static QString getSystemFolder(QStandardPaths::StandardLocation location);

    // Checks given file if it exists and if it does, then generates non-existing new file
    // according to format.
    static QString ensureUniqueFilename(const QString& name, const QString& append_format = QSL("(%1)"));

    // Filters out shit characters from filename.
    static QString filterBadCharsFromFilename(const QString& name);

    // Returns contents of a file.
    // Throws exception when no such file exists.
    static QByteArray readTextFile(const QString& file_path);
    static void writeTextFile(const QString& file_path, const QByteArray& data, const QString& encoding = QSL("UTF-8"));

    // Copies file, overwrites destination.
    static bool copyFile(const QString& source, const QString& destination);
};

#endif // IOFACTORY_H
