// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEXTCLOUDSERVICEROOT_H
#define NEXTCLOUDSERVICEROOT_H

#include <librssguard/services/abstract/cacheforserviceroot.h>
#include <librssguard/services/abstract/serviceroot.h>

#include <QMap>

class NextcloudNetworkFactory;
class Mutex;

class NextcloudServiceRoot : public ServiceRoot, public CacheForServiceRoot {
    Q_OBJECT

  public:
    explicit NextcloudServiceRoot(RootItem* parent = nullptr);
    virtual ~NextcloudServiceRoot();

    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual void editItems(const QList<RootItem*>& items);
    virtual FormAccountDetails* accountSetupDialog() const;
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);

    NextcloudNetworkFactory* network() const;

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void updateTitle();

  private:
    NextcloudNetworkFactory* m_network;
};

#endif // NEXTCLOUDSERVICEROOT_H
