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

#include <QDir>
#include <QFileInfo>
#include <QFile>


IOFactory::IOFactory() {
}

bool IOFactory::copyFile(const QString &source, const QString &destination) {
  if (QFile::exists(destination)) {
    if (!QFile::remove(destination)) {
      return false;
    }
  }

  return QFile::copy(source, destination);
}

bool IOFactory::removeFolder(const QString& directory_name,
                                const QStringList& exception_file_list,
                                const QStringList& exception_folder_list) {
  bool result = true;
  QDir dir(directory_name);

  if (dir.exists(directory_name)) {
    foreach (QFileInfo info,
             dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System |
                               QDir::Hidden | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
      if (info.isDir()) {
        if (!exception_folder_list.contains(info.fileName())) {
          result &= removeFolder(info.absoluteFilePath(), exception_file_list, exception_folder_list);
        }
      }
      else if (!exception_file_list.contains(info.fileName())) {
        if (!QFile::remove(info.absoluteFilePath())) {
          result &= false;
          qDebug("Failed to remove file \'%s\'.", qPrintable(QDir::toNativeSeparators(info.absoluteFilePath())));
        }
        else {
          result &= true;
        }
      }
    }

    if (dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden | QDir::AllDirs | QDir::Files).isEmpty()) {
      result &= dir.rmdir(directory_name);
    }
  }

  return result;
}

bool IOFactory::copyFolder(const QString &source, const QString &destination) {
  QDir dir_source(source);

  if (!dir_source.exists()) {
    return false;
  }

  QDir dir_destination(destination);

  if (!dir_destination.exists()) {
    dir_destination.mkpath(destination);
  }

  foreach (QString d, dir_source.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    QString dst_path = destination + QDir::separator() + d;
    dir_source.mkpath(dst_path);
    copyFolder(source + QDir::separator() + d, dst_path);
  }

  foreach (QString f, dir_source.entryList(QDir::Files)) {
    QString original_file = source + QDir::separator() + f;
    QString destination_file = destination + QDir::separator() + f;

    if (!QFile::exists(destination_file) || QFile::remove(destination_file)) {
      if (QFile::copy(original_file, destination_file)) {
        qDebug("Copied file \'%s\'.", qPrintable(f));
      }
      else {
        qDebug("Failed to copy file \'%s\'.", qPrintable(QDir::toNativeSeparators(original_file)));
      }
    }
    else {
      qDebug("Failed to remove file \'%s\'.", qPrintable(QDir::toNativeSeparators(original_file)));
    }
  }

  return true;
}
