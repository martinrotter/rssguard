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
    virtual bool editViaGui();
    virtual void start(bool freshly_activated);
    virtual QString code() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual LabelOperation supportedLabelOperations() const;

    FeedlyNetwork* network() const;

    void updateTitle();

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    FeedlyNetwork* m_network;
};

inline FeedlyNetwork* FeedlyServiceRoot::network() const {
  return m_network;
}

#endif // FEEDLYSERVICEROOT_H
