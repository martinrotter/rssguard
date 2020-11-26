// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef OWNCLOUDSERVICEROOT_H
#define OWNCLOUDSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

#include <QMap>

class OwnCloudNetworkFactory;
class Mutex;

class OwnCloudServiceRoot : public ServiceRoot, public CacheForServiceRoot {
  Q_OBJECT

  public:
    explicit OwnCloudServiceRoot(RootItem* parent = nullptr);
    virtual ~OwnCloudServiceRoot();

    bool isSyncable() const;
    bool canBeEdited() const;
    bool canBeDeleted() const;
    bool editViaGui();
    bool deleteViaGui();
    bool supportsFeedAdding() const;
    bool supportsCategoryAdding() const;

    void start(bool freshly_activated);
    void stop();
    QString code() const;
    OwnCloudNetworkFactory* network() const;

    void updateTitle();
    void saveAccountDataToDatabase();

    void saveAllCachedData(bool async = true);

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void loadFromDatabase();

    OwnCloudNetworkFactory* m_network;
};

#endif // OWNCLOUDSERVICEROOT_H
