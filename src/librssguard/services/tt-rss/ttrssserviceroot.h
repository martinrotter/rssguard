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

    void start(bool freshly_activated);
    void stop();
    QString code() const;
    bool canBeEdited() const;
    bool canBeDeleted() const;
    bool editViaGui();
    bool deleteViaGui();
    bool supportsFeedAdding() const;
    bool supportsCategoryAdding() const;
    QList<QAction*> serviceMenu();

    QString additionalTooltip() const;

    void saveAllCachedData(bool async = true);

    // Access to network.
    TtRssNetworkFactory* network() const;

    void saveAccountDataToDatabase();
    void updateTitle();

  public slots:
    void addNewFeed(const QString& url = QString());
    void addNewCategory();

  private:
    RootItem* obtainNewTreeForSyncIn() const;

    void loadFromDatabase();

    QAction* m_actionSyncIn;

    QList<QAction*> m_serviceMenu;
    TtRssNetworkFactory* m_network;
};

#endif // TTRSSSERVICEROOT_H
