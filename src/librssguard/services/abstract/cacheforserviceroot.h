// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CACHEFORSERVICEROOT_H
#define CACHEFORSERVICEROOT_H

#include "services/abstract/serviceroot.h"

#include <QMap>
#include <QMutex>
#include <QPair>
#include <QStringList>

struct CacheSnapshot {
  QMap<QString, QStringList> m_cachedLabelAssignments;
  QMap<QString, QStringList> m_cachedLabelDeassignments;
  QMap<RootItem::ReadStatus, QStringList> m_cachedStatesRead;
  QMap<RootItem::Importance, QList<Message>> m_cachedStatesImportant;
};

class CacheForServiceRoot {
  public:
    explicit CacheForServiceRoot();

    virtual void saveAllCachedData(bool ignore_errors) = 0;

    void addLabelsAssignmentsToCache(const QStringList& ids_of_messages, const QString& lbl_custom_id, bool assign);
    void addLabelsAssignmentsToCache(const QList<Message>& ids_of_messages, Label* lbl, bool assign);
    void addMessageStatesToCache(const QList<Message>& ids_of_messages, RootItem::Importance importance);
    void addMessageStatesToCache(const QStringList& ids_of_messages, RootItem::ReadStatus read);

    void loadCacheFromFile();
    void setUniqueId(int unique_id);
    bool isEmpty() const;

  protected:

    // Returns all cached data and clears the cache.
    // NOTE: If returned data are not successfuly passed back to
    // server then caller needs to re-add the data back to cache.
    CacheSnapshot takeMessageCache();

  private:
    void clearCache();
    void saveCacheToFile();

    int m_uniqueId;
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
};

#endif // CACHEFORSERVICEROOT_H
