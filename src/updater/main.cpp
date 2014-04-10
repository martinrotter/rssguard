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


int main(int argc, char *argv[]) { 
  // Instantiate base application object.
  QtSingleCoreApplication application("rssguard", argc, argv);
  qDebug("Instantiated QtSingleApplication class.");

  if (argc != 3) {
    qDebug("Insufficient arguments passed. Quitting updater...");
  }
  else {
    // Print input data.
    qDebug("RSS Guard application executable: %s", argv[1]);
    qDebug("File with update to be installed: %s", argv[2]);
  }

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

  // Everything is set up, file is downloaded, RSS Guard is not running.
  // TODO: nahradit spravne "/" > "\\" v argumentech pro 7za
  // https://code.google.com/p/quite-rss/source/browse/src/mainwindow.cpp?repo=updater#393
  QProcess::startDetached("7za", QStringList() << "e" << argv[2]);

  // Enter global event loop.
  return QtSingleCoreApplication::exec();
}

