// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CACHEFORSERVICEROOT_H
#define CACHEFORSERVICEROOT_H

#include "services/abstract/serviceroot.h"

#include <QMap>
#include <QPair>
#include <QStringList>

class QMutex;

struct CacheSnapshot {
  QMap<QString, QStringList> m_cachedLabelAssignments;
  QMap<QString, QStringList> m_cachedLabelDeassignments;
  QMap<RootItem::ReadStatus, QStringList> m_cachedStatesRead;
  QMap<RootItem::Importance, QList<Message>> m_cachedStatesImportant;
};

class CacheForServiceRoot {
  public:
    explicit CacheForServiceRoot();

    void addLabelsAssignmentsToCache(const QList<Message>& ids_of_messages, Label* lbl, bool assign);
    void addMessageStatesToCache(const QList<Message>& ids_of_messages, RootItem::Importance importance);
    void addMessageStatesToCache(const QStringList& ids_of_messages, RootItem::ReadStatus read);

    // Persistently saves/loads cached changes to/from file.
    // NOTE: The whole cache is cleared after save is done and before load is done.
    void saveCacheToFile(int acc_id);
    void loadCacheFromFile(int acc_id);

    virtual void saveAllCachedData(bool async = true) = 0;

  protected:
    CacheSnapshot takeMessageCache();

    QScopedPointer<QMutex> m_cacheSaveMutex;

    // Map where key is label's custom ID and value is list of message custom IDs
    // which we want to assign to the label.
    QMap<QString, QStringList> m_cachedLabelAssignments;

    // Map where key is label's custom ID and value is list of message custom IDs
    // which we want to remove from the label assignment.
    QMap<QString, QStringList> m_cachedLabelDeassignments;

    // Map of cached read/unread changes.
    QMap<RootItem::ReadStatus, QStringList> m_cachedStatesRead;

    // Map of cached important/unimportant changes.
    QMap<RootItem::Importance, QList<Message>> m_cachedStatesImportant;

  private:
    bool isEmpty() const;
    void clearCache();
};

#endif // CACHEFORSERVICEROOT_H
