// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/cacheforserviceroot.h"

#include "3rd-party/boolinq/boolinq.h"
#include "miscellaneous/application.h"
#include "miscellaneous/mutex.h"
#include "services/abstract/label.h"

#include <QDir>
#include <QSet>

CacheForServiceRoot::CacheForServiceRoot() : m_cacheSaveMutex(new QMutex(QMutex::NonRecursive)) {}

void CacheForServiceRoot::addMessageStatesToCache(const QList<Message>& ids_of_messages, Label* lbl, bool assign) {
  auto custom_ids = lbl->getParentServiceRoot()->customIDsOfMessages(ids_of_messages);

  if (assign) {
    m_cachedLabelAssignments[lbl->customId()].append(custom_ids);
    m_cachedLabelAssignments[lbl->customId()].removeDuplicates();

    // Remove the same messages from "deassign" list.
    auto deassign = m_cachedLabelDeassignments[lbl->customId()];
    auto list = boolinq::from(deassign.begin(), deassign.end()).where([custom_ids](const QString& id) {
      return !custom_ids.contains(id);
    }).toStdList();

    m_cachedLabelDeassignments[lbl->customId()] = FROM_STD_LIST(QStringList, list);
  }
}

void CacheForServiceRoot::addMessageStatesToCache(const QList<Message>& ids_of_messages, RootItem::Importance importance) {
  QMutexLocker lck(m_cacheSaveMutex.data());
  QList<Message>& list_act = m_cachedStatesImportant[importance];
  QList<Message>& list_other = m_cachedStatesImportant[importance == RootItem::Importance::Important
                               ? RootItem::Importance::NotImportant
                               : RootItem::Importance::Important];

  // Store changes, they will be sent to server later.
  list_act.append(ids_of_messages);

#if QT_VERSION >= 0x050E00 // Qt >= 5.14.0
  QSet<Message> set_act(list_act.begin(), list_act.end());
  QSet<Message> set_other(list_other.begin(), list_other.end());
#else
  QSet<Message> set_act = list_act.toSet();
  QSet<Message> set_other = list_other.toSet();
#endif

  // Now, we want to remove all IDS from list_other, which are contained in list.
  set_other -= set_act;
  list_act.clear();
  list_act.append(set_act.values());
  list_other.clear();
  list_other.append(set_other.values());
}

void CacheForServiceRoot::addMessageStatesToCache(const QStringList& ids_of_messages, RootItem::ReadStatus read) {
  QMutexLocker lck(m_cacheSaveMutex.data());
  QStringList& list_act = m_cachedStatesRead[read];
  QStringList& list_other = m_cachedStatesRead[read == RootItem::ReadStatus::Read
                            ? RootItem::ReadStatus::Unread
                            : RootItem::ReadStatus::Read];

  // Store changes, they will be sent to server later.
  list_act.append(ids_of_messages);

#if QT_VERSION >= 0x050E00 // Qt >= 5.14.0
  QSet<QString> set_act(list_act.begin(), list_act.end());
  QSet<QString> set_other(list_other.begin(), list_other.end());
#else
  QSet<QString> set_act = list_act.toSet();
  QSet<QString> set_other = list_other.toSet();
#endif

  // Now, we want to remove all IDS from list_other, which are contained in list.
  set_other -= set_act;
  list_act.clear();
  list_act.append(set_act.values());
  list_other.clear();
  list_other.append(set_other.values());
}

void CacheForServiceRoot::saveCacheToFile(int acc_id) {
  QMutexLocker lck(m_cacheSaveMutex.data());

  // Save to file.
  const QString file_cache = qApp->userDataFolder() + QDir::separator() + QString::number(acc_id) + "-cached-msgs.dat";

  if (isEmpty()) {
    QFile::remove(file_cache);
  }
  else {
    QFile file(file_cache);

    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
      QDataStream stream(&file);

      stream << m_cachedStatesImportant << m_cachedStatesRead << m_cachedLabelAssignments << m_cachedLabelDeassignments;
      file.flush();
      file.close();
    }

    clearCache();
  }
}

void CacheForServiceRoot::clearCache() {
  m_cachedStatesRead.clear();
  m_cachedStatesImportant.clear();
}

void CacheForServiceRoot::loadCacheFromFile(int acc_id) {
  QMutexLocker lck(m_cacheSaveMutex.data());

  clearCache();

  // Load from file.
  const QString file_cache = qApp->userDataFolder() + QDir::separator() + QString::number(acc_id) + "-cached-msgs.dat";
  QFile file(file_cache);

  if (file.exists()) {
    if (file.open(QIODevice::ReadOnly)) {
      QDataStream stream(&file);

      stream >> m_cachedStatesImportant >> m_cachedStatesRead >> m_cachedLabelAssignments >> m_cachedLabelDeassignments;
      file.flush();
      file.close();
    }

    file.remove();
  }
}

QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>> CacheForServiceRoot::takeMessageCache() {
  QMutexLocker lck(m_cacheSaveMutex.data());

  if (isEmpty()) {
    return QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>>();
  }

  // Make copy of changes.
  QMap<RootItem::ReadStatus, QStringList> cached_data_read = m_cachedStatesRead;

  cached_data_read.detach();

  QMap<RootItem::Importance, QList<Message>> cached_data_imp = m_cachedStatesImportant;

  cached_data_imp.detach();

  clearCache();

  return QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>>(cached_data_read, cached_data_imp);
}

bool CacheForServiceRoot::isEmpty() const {
  return m_cachedStatesRead.isEmpty() && m_cachedStatesImportant.isEmpty() &&
         m_cachedLabelAssignments.isEmpty() && m_cachedLabelDeassignments.isEmpty();
}
