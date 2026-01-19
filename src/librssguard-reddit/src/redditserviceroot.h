// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef REDDITSERVICEROOT_H
#define REDDITSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"
#include "src/gui/threadpreviewer.h"

class RedditNetworkFactory;

class RedditServiceRoot : public ServiceRoot, public CacheForServiceRoot {
    Q_OBJECT

  public:
    explicit RedditServiceRoot(RootItem* parent = nullptr);

    RedditNetworkFactory* network() const;

    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual void editItems(const QList<RootItem*>& items);
    virtual FormAccountDetails* accountSetupDialog() const;
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual QString additionalTooltip() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);
    virtual CustomMessagePreviewer* customMessagePreviewer();

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void updateTitle();

  private:
    QPointer<ThreadPreviewer> m_threadPreview;
    RedditNetworkFactory* m_network;
};

#endif // REDDITSERVICEROOT_H
