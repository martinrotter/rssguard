// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDLYSERVICEROOT_H
#define FEEDLYSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

class FeedlyNetwork;

class FeedlyServiceRoot : public ServiceRoot, public CacheForServiceRoot {
    Q_OBJECT

  public:
    explicit FeedlyServiceRoot(RootItem* parent = nullptr);

    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual void editItems(const QList<RootItem*>& items);
    virtual FormAccountDetails* accountSetupDialog() const;
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual LabelOperation supportedLabelOperations() const;
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual bool wantsBaggedIdsOfExistingMessages() const;
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);

    FeedlyNetwork* network() const;

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void updateTitle();

  private:
    FeedlyNetwork* m_network;
};

inline FeedlyNetwork* FeedlyServiceRoot::network() const {
  return m_network;
}

#endif // FEEDLYSERVICEROOT_H
