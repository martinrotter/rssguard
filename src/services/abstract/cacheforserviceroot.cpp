// This file is part of RSS Guard.
//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/abstract/cacheforserviceroot.h"

#include "miscellaneous/mutex.h"

#include <QSet>


CacheForServiceRoot::CacheForServiceRoot() : m_cacheSaveMutex(new Mutex(QMutex::NonRecursive, nullptr)),
  m_cachedStatesRead(QMap<RootItem::ReadStatus, QStringList>()),
  m_cachedStatesImportant(QMap<RootItem::Importance, QStringList>()) {
}

CacheForServiceRoot::~CacheForServiceRoot() {
  m_cacheSaveMutex->deleteLater();
}

void CacheForServiceRoot::addMessageStatesToCache(const QStringList &ids_of_messages, RootItem::ReadStatus read) {
  m_cacheSaveMutex->lock();

  QStringList &list_act = m_cachedStatesRead[read];
  QStringList &list_other = m_cachedStatesRead[read == RootItem::Read ? RootItem::Unread : RootItem::Read];

  // Store changes, they will be sent to server later.
  list_act.append(ids_of_messages);

  QSet<QString> set_act = list_act.toSet();
  QSet<QString> set_other = list_other.toSet();

  // Now, we want to remove all IDS from list_other, which are contained in list.
  set_other -= set_act;

  list_act.clear(); list_act.append(set_act.toList());
  list_other.clear(); list_other.append(set_other.toList());

  m_cacheSaveMutex->unlock();
}

QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QStringList> > CacheForServiceRoot::takeMessageCache() {
  m_cacheSaveMutex->lock();

  if (m_cachedStatesRead.isEmpty() && m_cachedStatesImportant.isEmpty()) {
    // No cached changes.
    m_cacheSaveMutex->unlock();
    return QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QStringList> >();
  }

  // Make copy of changes.
  QMap<RootItem::ReadStatus, QStringList> cached_data_read = m_cachedStatesRead;
  cached_data_read.detach();

  QMap<RootItem::Importance, QStringList> cached_data_imp = m_cachedStatesImportant;
  cached_data_imp.detach();

  m_cachedStatesRead.clear();
  m_cachedStatesImportant.clear();

  m_cacheSaveMutex->unlock();

  return QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QStringList> >(cached_data_read, cached_data_imp);
}
