// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GREADERSERVICEROOT_H
#define GREADERSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

#include <QMap>

class GreaderNetwork;

class GreaderServiceRoot : public ServiceRoot, public CacheForServiceRoot {
  Q_OBJECT

  public:
    enum class Service {
      FreshRss = 1,
      TheOldReader = 2,
      Bazqux = 4
    };

    explicit GreaderServiceRoot(RootItem* parent = nullptr);
    virtual ~GreaderServiceRoot();

    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual bool canBeDeleted() const;
    virtual bool editViaGui();
    virtual bool deleteViaGui();
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual void saveAllCachedData(bool ignore_errors);

    void setNetwork(GreaderNetwork* network);
    GreaderNetwork* network() const;

    void updateTitleIcon();
    void saveAccountDataToDatabase(bool creating_new);

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void loadFromDatabase();

  private:
    GreaderNetwork* m_network;
};

Q_DECLARE_METATYPE(GreaderServiceRoot::Service)

inline void GreaderServiceRoot::setNetwork(GreaderNetwork* network) {
  m_network = network;
}

inline GreaderNetwork* GreaderServiceRoot::network() const {
  return m_network;
}

#endif // GREADERSERVICEROOT_H
