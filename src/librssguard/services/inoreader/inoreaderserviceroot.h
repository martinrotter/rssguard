// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef INOREADERSERVICEROOT_H
#define INOREADERSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

class InoreaderNetworkFactory;

class InoreaderServiceRoot : public ServiceRoot, public CacheForServiceRoot {
  Q_OBJECT

  public:
    explicit InoreaderServiceRoot(InoreaderNetworkFactory* network, RootItem* parent = nullptr);
    virtual ~InoreaderServiceRoot();

    void saveAccountDataToDatabase();

    void setNetwork(InoreaderNetworkFactory* network);
    InoreaderNetworkFactory* network() const;

    bool isSyncable() const;
    bool canBeEdited() const;
    bool editViaGui();
    bool canBeDeleted() const;
    bool deleteViaGui();
    bool supportsFeedAdding() const;
    bool supportsCategoryAdding() const;
    void start(bool freshly_activated);
    void stop();
    QString code() const;

    QString additionalTooltip() const;

    RootItem* obtainNewTreeForSyncIn() const;

    void saveAllCachedData(bool async = true);

  public slots:
    void addNewFeed(const QString& url);
    void addNewCategory();
    void updateTitle();

  private:
    void loadFromDatabase();

  private:
    InoreaderNetworkFactory* m_network;
};

inline void InoreaderServiceRoot::setNetwork(InoreaderNetworkFactory* network) {
  m_network = network;
}

inline InoreaderNetworkFactory* InoreaderServiceRoot::network() const {
  return m_network;
}

#endif // INOREADERSERVICEROOT_H
