// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "definitions/definitions.h"
#include "qtsingleapplication/qtsinglecoreapplication.h"

#include <QTranslator>
#include <QDebug>
#include <QThread>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

#include <iostream>


bool removeDir(const QString & dirName) {
  bool result = true;
  QDir dir(dirName);

  if (dir.exists(dirName)) {
    foreach (QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
      if (info.isDir()) {
        result = removeDir(info.absoluteFilePath());
      }
      else {
        result = QFile::remove(info.absoluteFilePath());
      }

      if (!result) {
        return result;
      }
    }

    result = dir.rmdir(dirName);
  }

  return result;
}

bool copyPath(QString src, QString dst) {
  QDir dir(src);
  if (! dir.exists()) {
    return false;
  }

  foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
    QString dst_path = dst + QDir::separator() + d;
    dir.mkpath(dst_path);
    copyPath(src + QDir::separator() + d, dst_path);
  }

  foreach (QString f, dir.entryList(QDir::Files)) {
    QString original_file = src + QDir::separator() + f;
    QString destination_file = dst + QDir::separator() + f;

    if (!QFile::exists(destination_file) || QFile::remove(destination_file)) {
      if (QFile::copy(src + QDir::separator() + f, destination_file)) {
        qDebug("Copied  %s", qPrintable(f));
      }
      else {
        qDebug("Failed to copy file '%s'", qPrintable(original_file));
      }
    }
    else {
      qDebug("Failed to remove file '%s'", qPrintable(original_file));
    }
  }

  return true;
}

int main(int argc, char *argv[]) { 
  // Instantiate base application object.
  QtSingleCoreApplication application(APP_LOW_NAME, argc, argv);

  if (argc != 4) {
    qDebug("Insufficient arguments passed. Update process cannot proceed.");
    qDebug("Press any key to exit updater...");

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    return EXIT_FAILURE;
  }

  QString temp_directory = QDir::toNativeSeparators(argv[1]);
  QString rssguard_executable = QDir::toNativeSeparators(argv[2]);
  QString rssguard_path = QDir::toNativeSeparators(QFileInfo(rssguard_executable).absolutePath());
  QString update_archive = QDir::toNativeSeparators(argv[3]);

  // Print input data.
  qDebug("\n===== directories & files =====\n");
  qDebug("TEMP folder:\n\t %s", qPrintable(temp_directory));
  qDebug("RSS Guard application executable:\n\t %s", qPrintable(rssguard_executable));
  qDebug("RSS Guard application path:\n\t %s", qPrintable(rssguard_path));
  qDebug("File with update to be installed:\n\t %s", qPrintable(update_archive));
  qDebug("\n===== directories & files =====\n");

  // Check if main RSS Guard instance is running.
  if (application.sendMessage(APP_QUIT_INSTANCE)) {
    qDebug("RSS Guard application is running. Quitting it.");
  }

  if (!QFile::exists(update_archive)) {
    qDebug("Update file '%s' does not exist.", qPrintable(update_archive));
    qDebug("Press any key to exit updater...");

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    return EXIT_FAILURE;
  }

  qDebug().nospace() << "Running updater in thread: \'" <<
                        QThread::currentThreadId() << "\'.";

  QString extractor_program(APP_7ZA_EXECUTABLE);
  QStringList arguments;
  QString output_temp_directory = temp_directory + QDir::separator() + APP_LOW_NAME;

  // Remove old folders.
  if (QDir(output_temp_directory).exists()) {
    if (!removeDir(output_temp_directory)) {
      qDebug("Cleanup of old temporary files failed. Press any key to exit updater...");

      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

      return EXIT_FAILURE;
    }
  }

  qDebug("Update files are ready. Press any key to proceed...");

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  arguments << "x" << update_archive << "-r" <<
               "-y" << QString("-o%1").arg(output_temp_directory);

  qDebug("\n===== decompression =====\n");

  switch (QProcess::execute(extractor_program, arguments)) {
    case -1:
      qDebug("\nDecompressor crashed. Upgrading process failed.");
      return EXIT_FAILURE;

    case -2:
      qDebug("\nDecompressor was not started successfully. Upgrading process failed.");
      return EXIT_FAILURE;

    case 0:
      qDebug("\nDecompression is done. Proceeding to copying files to application directory.");
      break;

    default:
      qDebug("\nUnspecified error occured.");
      return EXIT_FAILURE;
  }

  qDebug("\n===== decompression =====\n");

  // All needed files are now decompressed in temporary directory.
  // Copy all possible files to RSS Guard application path and
  // do final cleanup.

  qDebug("\n===== copying =====\n");

  // Find "rssguard" subfolder path in
  QFileInfoList rssguard_temp_root = QDir(output_temp_directory).entryInfoList(QDir::Dirs |
                                                                               QDir::NoDotAndDotDot |
                                                                               QDir::NoSymLinks);

  if (rssguard_temp_root.size() != 1) {
    qDebug("Could not find root of downloaded application data.");

    return EXIT_FAILURE;
  }

  QString rssguard_single_temp_root = rssguard_temp_root.at(0).absoluteFilePath();

  if (!copyPath(rssguard_single_temp_root, rssguard_path)) {
    qDebug("Critical error appeared during copying of application files.");
  }

  qDebug("\n===== copying =====\n");

  qDebug("\n===== cleanup =====\n");

  removeDir(output_temp_directory);
  QFile::remove(update_archive);

  qDebug("\n===== cleanup =====\n");

  qDebug("Press any key to exit updater and start RSS Guard.");

  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

  QProcess::startDetached(rssguard_executable);

  return EXIT_SUCCESS;
}

