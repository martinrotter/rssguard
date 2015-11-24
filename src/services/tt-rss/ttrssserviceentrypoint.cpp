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

#include "services/tt-rss/ttrssserviceentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"


TtRssServiceEntryPoint::TtRssServiceEntryPoint(){
}


TtRssServiceEntryPoint::~TtRssServiceEntryPoint() {

}

bool TtRssServiceEntryPoint::isSingleInstanceService() {
  return false;
}

bool TtRssServiceEntryPoint::canBeEdited() {
  return true;
}

QString TtRssServiceEntryPoint::name() {
  return QSL("TT-RSS (TinyTiny RSS)");
}

QString TtRssServiceEntryPoint::description() {
  return QSL("This service offers integration with TinyTiny RSS.");
}

QString TtRssServiceEntryPoint::version() {
  return QSL("0.0.1");
}

QString TtRssServiceEntryPoint::author() {
  return APP_AUTHOR;
}

QIcon TtRssServiceEntryPoint::icon() {
  return QIcon(APP_ICON_PATH);
}

QList<ServiceRoot*> TtRssServiceEntryPoint::initializeSubtree(FeedsModel *main_model) {
  return QList<ServiceRoot*>();
}
