// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/cacheforserviceroot.h"

#include "3rd-party/boolinq/boolinq.h"
#include "miscellaneous/application.h"
#include "miscellaneous/mutex.h"
#include "services/abstract/label.h"

#include <QDir>
#include <QSet>

CacheForServiceRoot::CacheForServiceRoot() : m_uniqueId(NO_PARENT_CATEGORY), m_cacheSaveMutex(new QMutex()) {}

void CacheForServiceRoot::addLabelsAssignmentsToCache(const QStringList& ids_of_messages,
                                                      const QString& lbl_custom_id,
                                                      bool assign) {
  if (assign) {
    for (const QString& custom_id : ids_of_messages) {
      if (m_cachedLabelDeassignments[lbl_custom_id].contains(custom_id)) {
        // We want to assign this ID but it was marked for deassignment, remove from deassignment.
        m_cachedLabelDeassignments[lbl_custom_id].removeAll(custom_id);
      }
      else {
        m_cachedLabelAssignments[lbl_custom_id].append(custom_id);
        m_cachedLabelAssignments[lbl_custom_id].removeDuplicates();
      }
    }
  }
  else {
    for (const QString& custom_id : ids_of_messages) {
      if (m_cachedLabelAssignments[lbl_custom_id].contains(custom_id)) {
        // We want to deassign this ID but it was marked for assignment, remove from assignment.
        m_cachedLabelAssignments[lbl_custom_id].removeAll(custom_id);
      }
      else {
        m_cachedLabelDeassignments[lbl_custom_id].append(custom_id);
        m_cachedLabelDeassignments[lbl_custom_id].removeDuplicates();
      }
    }
  }

  saveCacheToFile();
}

void CacheForServiceRoot::addLabelsAssignmentsToCache(const QList<Message>& ids_of_messages, Label* lbl, bool assign) {
  auto custom_ids = lbl->getParentServiceRoot()->customIDsOfMessages(ids_of_messages);

  addLabelsAssignmentsToCache(custom_ids, lbl->customId(), assign);
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

  saveCacheToFile();
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

  saveCacheToFile();
}

void CacheForServiceRoot::saveCacheToFile() {
  // Save to file.
  const QString file_cache = qApp->userDataFolder() + QDir::separator() + QString::number(m_uniqueId) + "-cached-msgs.dat";

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
  }
}

void CacheForServiceRoot::clearCache() {
  m_cachedStatesRead.clear();
  m_cachedStatesImportant.clear();
  m_cachedLabelAssignments.clear();
  m_cachedLabelDeassignments.clear();
}

void CacheForServiceRoot::loadCacheFromFile() {
  QMutexLocker lck(m_cacheSaveMutex.data());

  clearCache();

  // Load from file.
  const QString file_cache = qApp->userDataFolder() + QDir::separator() + QString::number(m_uniqueId) + "-cached-msgs.dat";
  QFile file(file_cache);

  if (file.exists()) {
    if (file.open(QIODevice::OpenModeFlag::ReadOnly)) {
      QDataStream stream(&file);

      stream >> m_cachedStatesImportant >> m_cachedStatesRead >> m_cachedLabelAssignments >> m_cachedLabelDeassignments;
      file.close();
    }
  }
}

void CacheForServiceRoot::setUniqueId(int unique_id) {
  m_uniqueId = unique_id;
}

CacheSnapshot CacheForServiceRoot::takeMessageCache() {
  QMutexLocker lck(m_cacheSaveMutex.data());

  if (isEmpty()) {
    return CacheSnapshot();
  }

  // Make copy of changes.
  auto cached_data_read = m_cachedStatesRead;
  auto cached_data_imp = m_cachedStatesImportant;
  auto cached_ass_lbl = m_cachedLabelAssignments;
  auto cached_deass_lbl = m_cachedLabelDeassignments;

  cached_data_read.detach();
  cached_data_imp.detach();
  cached_ass_lbl.detach();
  cached_deass_lbl.detach();

  clearCache();
  saveCacheToFile();

  CacheSnapshot c;

  c.m_cachedLabelAssignments = cached_ass_lbl;
  c.m_cachedLabelDeassignments = cached_deass_lbl;
  c.m_cachedStatesImportant = cached_data_imp;
  c.m_cachedStatesRead = cached_data_read;

  return c;
}

bool CacheForServiceRoot::isEmpty() const {
  return m_cachedStatesRead.isEmpty() && m_cachedStatesImportant.isEmpty() &&
         m_cachedLabelAssignments.isEmpty() && m_cachedLabelDeassignments.isEmpty();
}
