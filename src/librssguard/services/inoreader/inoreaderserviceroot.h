// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef INOREADERSERVICEROOT_H
#define INOREADERSERVICEROOT_H

#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/serviceroot.h"

class InoreaderNetworkFactory;

class InoreaderServiceRoot : public ServiceRoot, public CacheForServiceRoot {
  Q_OBJECT

  public:
    explicit InoreaderServiceRoot(RootItem* parent = nullptr);
    virtual ~InoreaderServiceRoot();

    void saveAccountDataToDatabase(bool creating_new);

    InoreaderNetworkFactory* network() const;

    virtual LabelOperation supportedLabelOperations() const;
    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual bool editViaGui();
    virtual bool canBeDeleted() const;
    virtual bool deleteViaGui();
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual QString additionalTooltip() const;
    virtual void saveAllCachedData(bool ignore_errors);

    void updateTitle();

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void loadFromDatabase();

  private:
    InoreaderNetworkFactory* m_network;
};

inline InoreaderNetworkFactory* InoreaderServiceRoot::network() const {
  return m_network;
}

#endif // INOREADERSERVICEROOT_H
