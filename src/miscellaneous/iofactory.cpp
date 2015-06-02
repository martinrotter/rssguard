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

#include "miscellaneous/iofactory.h"

#include <exceptions/ioexception.h>

#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QObject>


IOFactory::IOFactory() {
}

QString IOFactory::getSystemFolder(SYSTEM_FOLDER_ENUM::StandardLocation location) {
#if QT_VERSION >= 0x050000
  return SYSTEM_FOLDER_ENUM::writableLocation(location);
#else
  return SYSTEM_FOLDER_ENUM::storageLocation(location);
#endif
}

QByteArray IOFactory::readTextFile(const QString &file_path) {
  QFile input_file(file_path);
  QByteArray input_data;

  if (input_file.open(QIODevice::Text | QIODevice::Unbuffered | QIODevice::ReadOnly)) {
    input_data = input_file.readAll();
    input_file.close();
    return input_data;
  }
  else {
    throw IOException(tr("Cannot open file '%1' for reading.").arg(QDir::toNativeSeparators(file_path)));
  }
}

bool IOFactory::copyFile(const QString &source, const QString &destination) {
  if (QFile::exists(destination)) {
    if (!QFile::remove(destination)) {
      return false;
    }
  }

  return QFile::copy(source, destination);
}
