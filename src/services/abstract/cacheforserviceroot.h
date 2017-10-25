// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef CACHEFORSERVICEROOT_H
#define CACHEFORSERVICEROOT_H

#include "services/abstract/serviceroot.h"

#include <QMap>
#include <QPair>
#include <QStringList>

class Mutex;

class CacheForServiceRoot {
  public:
    explicit CacheForServiceRoot();
    virtual ~CacheForServiceRoot();

    void addMessageStatesToCache(const QList<Message>& ids_of_messages, RootItem::Importance importance);
    void addMessageStatesToCache(const QStringList& ids_of_messages, RootItem::ReadStatus read);

    // Persistently saves/loads cached changes to/from file.
    // NOTE: The whole cache is cleared after save is done and before load is done.
    void saveCacheToFile(int acc_id);
    void loadCacheFromFile(int acc_id);

    virtual void saveAllCachedData(bool async = true) = 0;

  protected:
    QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>> takeMessageCache();

    Mutex* m_cacheSaveMutex;

    QMap<RootItem::ReadStatus, QStringList> m_cachedStatesRead;
    QMap<RootItem::Importance, QList<Message>> m_cachedStatesImportant;

  private:
    bool isEmpty() const;
    void clearCache();
};

#endif // CACHEFORSERVICEROOT_H
