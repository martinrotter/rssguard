// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/wallabag/wallabagserviceentrypoint.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "gui/messagebox.h"
#include "gui/dialogs/formmain.h"


WallabagServiceEntryPoint::WallabagServiceEntryPoint() {
}

WallabagServiceEntryPoint::~WallabagServiceEntryPoint() {
}

ServiceRoot *WallabagServiceEntryPoint::createNewRoot() const {
  MessageBox::show(qApp->mainForm(), QMessageBox::Warning, QObject::tr("Not yet supported"),
                   QObject::tr("This plugin is not yet ready for usage. It will be added in future versions."));
  return NULL;
}

QList<ServiceRoot*> WallabagServiceEntryPoint::initializeSubtree() const {
  return QList<ServiceRoot*>();
}

bool WallabagServiceEntryPoint::isSingleInstanceService() const {
  return false;
}

QString WallabagServiceEntryPoint::name() const {
  return QSL("wallabag");
}

QString WallabagServiceEntryPoint::code() const {
  return SERVICE_CODE_WALLABAG;
}

QString WallabagServiceEntryPoint::description() const {
  return QObject::tr("This plugin allows you to view and manager your wallabag articles.");
}

QString WallabagServiceEntryPoint::version() const {
  return APP_VERSION;
}

QString WallabagServiceEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon WallabagServiceEntryPoint::icon() const {
  return qApp->icons()->fromTheme(QSL("application-wallabag"));
}
