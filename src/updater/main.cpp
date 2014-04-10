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

#include "qtsingleapplication/qtsinglecoreapplication.h"
#include "updater/detector.h"

#include <QTranslator>
#include <QDebug>
#include <QThread>
#include <QProcess>
#include <QFileInfo>
#include <QDir>


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

int main(int argc, char *argv[]) { 
  // Instantiate base application object.
  QtSingleCoreApplication application("rssguard", argc, argv);

  if (argc != 4) {
    qDebug("Insufficient arguments passed. Quitting RSS Guard updater...");
    return EXIT_FAILURE;
  }

  QString temp_directory = QDir::toNativeSeparators(argv[1]);
  QString rssguard_executable = QDir::toNativeSeparators(argv[2]);
  QString rssguard_path = QDir::toNativeSeparators(QFileInfo(rssguard_executable).absolutePath());
  QString update_archive = QDir::toNativeSeparators(argv[3]);

  // Print input data.
  qDebug("\n===== directories & files =====");
  qDebug("TEMP folder:\n\t %s", qPrintable(temp_directory));
  qDebug("RSS Guard application executable:\n\t %s", qPrintable(rssguard_executable));
  qDebug("RSS Guard application path:\n\t %s", qPrintable(rssguard_path));
  qDebug("File with update to be installed:\n\t %s", qPrintable(update_archive));
  qDebug("===== directories & files =====\n");

  // Check if main RSS Guard instance is running.
  if (application.sendMessage("app_quit")) {
    qDebug("RSS Guard application is running. Quitting it...");
  }

  Detector detector;

  qDebug().nospace() << "Running updater in thread: \'" <<
                        QThread::currentThreadId() << "\'.";

  // Setup single-instance behavior.
  QObject::connect(&application, SIGNAL(messageReceived(QString)),
                   &detector, SLOT(handleMessage(QString)));

  QString extractor_program("7za.exe");
  QStringList arguments;
  QString output_temp_directory = temp_directory + QDir::separator() + "rssguard";

  // Remove old folders.
  if (QDir(output_temp_directory).exists()) {
    removeDir(output_temp_directory);
  }

  arguments << "x" << update_archive << "-r" <<
               "-y" << QString("-o%1").arg(output_temp_directory);

  switch (QProcess::execute(extractor_program, arguments)) {
    case -1:
      qDebug("\nDecompressor crashed. Upgrading process failed.");
      return EXIT_FAILURE;

    case -2:
      qDebug("\nDecompressor was not started successfully. Upgrading process failed.");
      return EXIT_FAILURE;

    case 0:
      qDebug("\nDecompression is done.");
      break;

    default:
      qDebug("\nUnspecified error occured.");
      return EXIT_FAILURE;
  }

  // All needed files are now decompressed in temporary directory.
  // Copy all possible files to RSS Guard application path and
  // do final cleanup.

  // Enter global event loop.
  return QtSingleCoreApplication::exec();
}

