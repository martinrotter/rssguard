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
  Q_PROPERTY(QString username READ username WRITE setUsername)
  Q_PROPERTY(QString password READ password WRITE setPassword)
  Q_PROPERTY(bool auth_protected READ authProtected WRITE setAuthProtected)
  Q_PROPERTY(QString auth_username READ authUsername WRITE setAuthUsername)
  Q_PROPERTY(QString auth_password READ authPassword WRITE setAuthPassword)
  Q_PROPERTY(QString url READ url WRITE setUrl)
  Q_PROPERTY(bool force_update READ forceUpdate WRITE setForceUpdate)
  Q_PROPERTY(bool download_only_unread READ downloadOnlyUnread WRITE setDownloadOnlyUnread)

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
    virtual QList<CustomDatabaseEntry> customDatabaseAttributes() const;

    // Access to network.
    TtRssNetworkFactory* network() const;

    void updateTitle();

    // Support for dynamic DB attributes.
    QString username() const;
    void setUsername(const QString& username);

    QString password() const;
    void setPassword(const QString& password);

    bool authProtected() const;
    void setAuthProtected(bool auth_protected);

    QString authUsername() const;
    void setAuthUsername(const QString& auth_username);

    QString authPassword() const;
    void setAuthPassword(const QString& auth_password);

    QString url() const;
    void setUrl(const QString& url);

    bool forceUpdate() const;
    void setForceUpdate(bool force_update);

    bool downloadOnlyUnread() const;
    void setDownloadOnlyUnread(bool download_only_unread);

  protected:
    virtual RootItem* obtainNewTreeForSyncIn() const;

  private:
    void loadFromDatabase();

    TtRssNetworkFactory* m_network;
};

#endif // TTRSSSERVICEROOT_H
