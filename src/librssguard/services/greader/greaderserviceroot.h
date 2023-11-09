// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GREADERSERVICEROOT_H
#define GREADERSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

class GreaderNetwork;

class GreaderServiceRoot : public ServiceRoot, public CacheForServiceRoot {
    Q_OBJECT

  public:
    enum class Service {
      FreshRss = 1,
      TheOldReader = 2,
      Bazqux = 4,
      Reedah = 8,
      Inoreader = 16,
      Miniflux = 32,
      Other = 1024
    };

    Q_ENUM(Service)

    explicit GreaderServiceRoot(RootItem* parent = nullptr);

    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual void editItems(const QList<RootItem*>& items);
    virtual FormAccountDetails* accountSetupDialog() const;
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual QList<QAction*> serviceMenu();
    virtual void saveAllCachedData(bool ignore_errors);
    virtual LabelOperation supportedLabelOperations() const;
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual void aboutToBeginFeedFetching(const QList<Feed*>& feeds,
                                          const QHash<QString, QHash<ServiceRoot::BagOfMessages, QStringList>>&
                                            stated_messages,
                                          const QHash<QString, QStringList>& tagged_messages);
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);
    virtual bool wantsBaggedIdsOfExistingMessages() const;

    GreaderNetwork* network() const;

    static QString serviceToString(Service service);

  private slots:
    void importFeeds();
    void exportFeeds();

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void updateTitleIcon();

  private:
    GreaderNetwork* m_network;
};

Q_DECLARE_METATYPE(GreaderServiceRoot::Service)

inline GreaderNetwork* GreaderServiceRoot::network() const {
  return m_network;
}

#endif // GREADERSERVICEROOT_H
