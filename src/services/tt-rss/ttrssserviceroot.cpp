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

#include "services/tt-rss/ttrssserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"
#include "core/feedsmodel.h"


TtRssServiceRoot::TtRssServiceRoot(FeedsModel *feeds_model, RootItem *parent) : ServiceRoot(feeds_model, parent) {
  // TODO: nadpis se bude měnit podle nastavení uživatelského
  // jména a serveru tohoto ttrss učtu
  setTitle(qApp->system()->getUsername() + "@ttrss");
  setIcon(TtRssServiceEntryPoint().icon());
}

TtRssServiceRoot::~TtRssServiceRoot() {
}

bool TtRssServiceRoot::editViaGui() {
  return false;
}

bool TtRssServiceRoot::canBeEdited() {
  return true;
}

bool TtRssServiceRoot::canBeDeleted() {
  return true;
}

QVariant TtRssServiceRoot::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      // TODO: zobrazovat pokročile informace a statistiky.
      if (column == FDS_MODEL_TITLE_INDEX) {
        return tr("This is service account TT-RSS (TinyTiny RSS) server.");
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        //: Tooltip for "unread" column of feed list.
        return tr("%n unread message(s).", 0, countOfUnreadMessages());
      }
      else {
        return QVariant();
      }

    default:
      return ServiceRoot::data(column, role);
  }
}
