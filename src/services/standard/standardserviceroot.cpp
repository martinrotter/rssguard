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

#include "services/standard/standardserviceroot.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "services/standard/standardserviceentrypoint.h"


StandardServiceRoot::StandardServiceRoot(RootItem *parent) : ServiceRoot(parent) {
  m_title = qApp->system()->getUsername() + "@" + APP_LOW_NAME;
  m_icon = StandardServiceEntryPoint().icon();
}

StandardServiceRoot::~StandardServiceRoot() {
}

bool StandardServiceRoot::canBeEdited() {
  return false;
}

bool StandardServiceRoot::canBeDeleted() {
  return false;
}

QVariant StandardServiceRoot::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::CountFormat)).toString()
            .replace(PLACEHOLDER_UNREAD_COUNTS, QString::number(countOfUnreadMessages()))
            .replace(PLACEHOLDER_ALL_COUNTS, QString::number(countOfAllMessages()));
      }
      else {
        return QVariant();
      }

    case Qt::EditRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return countOfUnreadMessages();
      }
      else {
        return QVariant();
      }

    case Qt::DecorationRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_icon;
      }
      else {
        return QVariant();
      }

    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return
            m_title + "\n" +
            tr("This is service account for standard RSS/RDF/ATOM feeds.");
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        //: Tooltip for "unread" column of feed list.
        return tr("%n unread message(s).", 0, countOfUnreadMessages());
      }
      else {
        return QVariant();
      }

    case Qt::TextAlignmentRole:
      if (column == FDS_MODEL_COUNTS_INDEX) {
        return Qt::AlignCenter;
      }
      else {
        return QVariant();
      }

    case Qt::FontRole:
      return countOfUnreadMessages() > 0 ? m_boldFont : m_normalFont;

    default:
      return QVariant();
  }
}
