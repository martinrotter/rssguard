// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/cacheforserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/mutex.h"

#include <QDir>
#include <QSet>

CacheForServiceRoot::CacheForServiceRoot() : m_cacheSaveMutex(new Mutex(QMutex::NonRecursive, nullptr)) {}

CacheForServiceRoot::~CacheForServiceRoot() {
  m_cacheSaveMutex->deleteLater();
}

void CacheForServiceRoot::addMessageStatesToCache(const QList<Message>& ids_of_messages, RootItem::Importance importance) {
  m_cacheSaveMutex->lock();

  QList<Message>& list_act = m_cachedStatesImportant[importance];
  QList<Message>& list_other = m_cachedStatesImportant[importance == RootItem::Important ? RootItem::NotImportant : RootItem::Important];

  // Store changes, they will be sent to server later.
  list_act.append(ids_of_messages);
  QSet<Message> set_act = list_act.toSet();
  QSet<Message> set_other = list_other.toSet();

  // Now, we want to remove all IDS from list_other, which are contained in list.
  set_other -= set_act;
  list_act.clear();
  list_act.append(set_act.toList());
  list_other.clear();
  list_other.append(set_other.toList());

  m_cacheSaveMutex->unlock();
}

void CacheForServiceRoot::addMessageStatesToCache(const QStringList& ids_of_messages, RootItem::ReadStatus read) {
  m_cacheSaveMutex->lock();

  QStringList& list_act = m_cachedStatesRead[read];
  QStringList& list_other = m_cachedStatesRead[read == RootItem::Read ? RootItem::Unread : RootItem::Read];

  // Store changes, they will be sent to server later.
  list_act.append(ids_of_messages);
  QSet<QString> set_act = list_act.toSet();
  QSet<QString> set_other = list_other.toSet();

  // Now, we want to remove all IDS from list_other, which are contained in list.
  set_other -= set_act;
  list_act.clear();
  list_act.append(set_act.toList());
  list_other.clear();
  list_other.append(set_other.toList());

  m_cacheSaveMutex->unlock();
}

void CacheForServiceRoot::saveCacheToFile(int acc_id) {
  m_cacheSaveMutex->lock();

  // Save to file.
  const QString file_cache = qApp->userDataFolder() + QDir::separator() + QString::number(acc_id) + "-cached-msgs.dat";

  if (isEmpty()) {
    QFile::remove(file_cache);
  }
  else {
    QFile file(file_cache);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QDataStream stream(&file);

      stream << m_cachedStatesImportant << m_cachedStatesRead;
      file.flush();
      file.close();
    }

    clearCache();
  }

  m_cacheSaveMutex->unlock();
}

void CacheForServiceRoot::clearCache() {
  m_cachedStatesRead.clear();
  m_cachedStatesImportant.clear();
}

void CacheForServiceRoot::loadCacheFromFile(int acc_id) {
  m_cacheSaveMutex->lock();
  clearCache();

  // Load from file.
  const QString file_cache = qApp->userDataFolder() + QDir::separator() + QString::number(acc_id) + "-cached-msgs.dat";
  QFile file(file_cache);

  if (file.exists()) {
    if (file.open(QIODevice::ReadOnly)) {
      QDataStream stream(&file);

      stream >> m_cachedStatesImportant >> m_cachedStatesRead;
      file.flush();
      file.close();
    }

    file.remove();
  }

  m_cacheSaveMutex->unlock();
}

QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>> CacheForServiceRoot::takeMessageCache() {
  m_cacheSaveMutex->lock();

  if (isEmpty()) {
    // No cached changes.
    m_cacheSaveMutex->unlock();

    return QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>>();
  }

  // Make copy of changes.
  QMap<RootItem::ReadStatus, QStringList> cached_data_read = m_cachedStatesRead;
  cached_data_read.detach();

  QMap<RootItem::Importance, QList<Message>> cached_data_imp = m_cachedStatesImportant;
  cached_data_imp.detach();

  clearCache();
  m_cacheSaveMutex->unlock();

  return QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>>(cached_data_read, cached_data_imp);
}

bool CacheForServiceRoot::isEmpty() const {
  return m_cachedStatesRead.isEmpty() && m_cachedStatesImportant.isEmpty();
}
