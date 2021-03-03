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

    virtual LabelOperation supportedLabelOperations() const;
    virtual void start(bool freshly_activated);
    virtual void stop();
    virtual QString code() const;
    virtual bool isSyncable() const;
    virtual bool canBeEdited() const;
    virtual bool editViaGui();
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual void addNewFeed(RootItem* selected_item, const QString& url = QString());
    virtual QString additionalTooltip() const;
    virtual void saveAllCachedData(bool ignore_errors);
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QList<Message> obtainNewMessages(const QList<Feed*>& feeds, bool* error_during_obtaining);

    // Access to network.
    TtRssNetworkFactory* network() const;

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void updateTitle();

  private:
    TtRssNetworkFactory* m_network;
};

#endif // TTRSSSERVICEROOT_H
