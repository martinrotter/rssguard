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
#include "qtsingleapplication/qtsingleapplication.h"
#include "updater/formupdater.h"

#include <QTranslator>
#include <QDebug>
#include <QThread>
#include <QProcess>
#include <QFileInfo>
#include <QDir>

#include <iostream>
#include <limits>


// Main entry point to "rssguard_updater.exe".
// It expects 4 ARGUMENTS:
//  0) - the actual path of this process,
//  1) - string with current version,
//  2) - string with future version,
//  3) - path to RSS Guard ("rssguard.exe") file,
//  4) - path to update file (stored in TEMP folder).
int main(int argc, char *argv[]) {
  // Instantiate base application object.
  QtSingleApplication application(APP_LOW_NAME, argc, argv);
  application.setQuitOnLastWindowClosed(true);

  FormUpdater main_form;

  // Setup message handler after main_form is created.
  qInstallMessageHandler(FormUpdater::debugHandler);

  main_form.show();
  main_form.startUpgrade();

  return application.exec();
}

