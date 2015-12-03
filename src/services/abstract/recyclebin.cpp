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

#include "services/abstract/recyclebin.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"


RecycleBin::RecycleBin(RootItem *parent_item) : RootItem(parent_item) {
  setKind(RootItemKind::Bin);
  setIcon(qApp->icons()->fromTheme(QSL("folder-recycle-bin")));
  setTitle(tr("Recycle bin"));
  setDescription(tr("Recycle bin contains all deleted messages from all feeds."));
  setCreationDate(QDateTime::currentDateTime());
}

RecycleBin::~RecycleBin() {
}

QVariant RecycleBin::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      return tr("Recycle bin\n\n%1").arg(tr("%n deleted message(s).", 0, countOfAllMessages()));

    default:
      return RootItem::data(column, role);
  }
}
