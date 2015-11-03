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


#include "services/standard/standardserviceentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "services/standard/standardserviceroot.h"


StandardServiceEntryPoint::StandardServiceEntryPoint() {
}

StandardServiceEntryPoint::~StandardServiceEntryPoint() {
}

bool StandardServiceEntryPoint::isDefaultService() {
  return true;
}

bool StandardServiceEntryPoint::isSingleInstanceService() {
  return true;
}

bool StandardServiceEntryPoint::canBeAdded() {
  return false;
}

bool StandardServiceEntryPoint::canBeDeleted() {
  return false;
}

bool StandardServiceEntryPoint::canBeEdited() {
  return false;
}

QString StandardServiceEntryPoint::name() {
  return QSL("Standard (RSS/RDF/ATOM)");
}

QString StandardServiceEntryPoint::description() {
  return QSL("This service offers integration with standard online RSS/RDF/ATOM feeds and podcasts.");
}

QString StandardServiceEntryPoint::version() {
  return APP_VERSION;
}

QString StandardServiceEntryPoint::author() {
  return APP_AUTHOR;
}

QIcon StandardServiceEntryPoint::icon() {
  return QIcon(APP_ICON_PATH);
}

QList<ServiceRoot*> StandardServiceEntryPoint::initializeSubtree(FeedsModel *main_model) {
  StandardServiceRoot *root = new StandardServiceRoot(main_model);
  QList<ServiceRoot*> roots;

  roots.append(root);
  return roots;
}
