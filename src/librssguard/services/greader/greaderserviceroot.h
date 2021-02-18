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
      Other = 1024
    };

    explicit GreaderServiceRoot(RootItem* parent = nullptr);

    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual bool canBeDeleted() const;
    virtual bool editViaGui();
    virtual bool deleteViaGui();
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual LabelOperation supportedLabelOperations() const;

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

inline GreaderNetwork* GreaderServiceRoot::network() const {
  return m_network;
}

#endif // GREADERSERVICEROOT_H
