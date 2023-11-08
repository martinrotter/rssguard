// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TTRSSSERVICEROOT_H
#define TTRSSSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

#include <QCoreApplication>

class TtRssCategory;
class TtRssFeed;
class TtRssNetworkFactory;

class TtRssServiceRoot : public ServiceRoot, public CacheForServiceRoot {
    Q_OBJECT

  public:
    explicit TtRssServiceRoot(RootItem* parent = nullptr);
    virtual ~TtRssServiceRoot();

    virtual bool wantsBaggedIdsOfExistingMessages() const;
    virtual LabelOperation supportedLabelOperations() const;
    virtual void start(bool freshly_activated);
    virtual void stop();
    virtual QString code() const;
    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual void editItems(const QList<RootItem*>& items);
    virtual FormAccountDetails* accountSetupDialog() const;
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual void addNewFeed(RootItem* selected_item, const QString& url = QString());
    virtual QString additionalTooltip() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);

    // Access to network.
    TtRssNetworkFactory* network() const;

  public slots:
    void shareToPublished();

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void updateTitle();
    QList<Message> obtainMessagesIntelligently(Feed* feed,
                                               const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages);
    QList<Message> obtainMessagesViaHeadlines(Feed* feed);

  private:
    TtRssNetworkFactory* m_network;
};

#endif // TTRSSSERVICEROOT_H
