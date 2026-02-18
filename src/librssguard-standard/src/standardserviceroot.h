// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef STANDARDSERVICEROOT_H
#define STANDARDSERVICEROOT_H

#include "src/standardfeed.h"

#include <librssguard/services/abstract/serviceroot.h>

#include <QCoreApplication>
#include <QMutex>
#include <QPair>

class StandardCategory;
class FeedsImportExportModel;
class QMenu;

class StandardServiceRoot : public ServiceRoot {
    Q_OBJECT

    friend class FormStandardFeedDetails;
    friend class FormStandardImportExport;

  public:
    explicit StandardServiceRoot(RootItem* parent = nullptr);
    virtual ~StandardServiceRoot();

    virtual QNetworkProxy networkProxyForItem(RootItem* item) const;
    virtual FormAccountDetails* accountSetupDialog() const;
    virtual void updateItemTitle(RootItem* item, const QString& new_title);
    virtual void onDatabaseCleanup();
    virtual void onAfterFeedsPurged(const QList<Feed*>& feeds);
    virtual void start(bool freshly_activated);
    virtual void stop();
    virtual QString code() const;
    virtual bool canBeEdited() const;
    virtual void editItems(const QList<RootItem*>& items);
    virtual bool supportsFeedAdding() const;
    virtual bool supportsCategoryAdding() const;
    virtual Qt::ItemFlags additionalFlags(int column) const;
    virtual QVariantHash customDatabaseData() const;
    virtual void setCustomDatabaseData(const QVariantHash& data);
    virtual QList<Message> obtainNewMessages(Feed* feed,
                                             const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                             const QHash<QString, QStringList>& tagged_messages);

    virtual QList<QAction*> serviceMenu();
    virtual QList<QAction*> contextMenuFeedsList(const QList<RootItem*>& selected_items);

    void spaceHost(const QString& host, const QString& url);
    void resetHostSpacing(const QString& host, const QDateTime& next_dt);

    // If set to number > 0, then requests to fetch feeds
    // will be spaced by the given number (in seconds).
    // This is used to avoid too many concurrent network
    // requests from to the same server and getting HTTP/429
    // errors or other DDoS-attack-related bans.
    int spacingSameHostsRequests() const;
    void setSpacingSameHostsRequests(int spacing);

    static QString defaultTitle();

  public slots:
    void addNewFeed(RootItem* selected_item, const QString& url = QString());
    void addNewCategory(RootItem* selected_item);

  private slots:
    void loadDefaultFeeds();
    void importFeeds();
    void importFromQuiteRss();
    void importFromRssGuard4();
    void exportFeeds();

  private:
    void fetchMetadataForAllFeeds(const QList<Feed*>& feeds);

    // Takes structure residing under given root item and adds feeds/categories from
    // it to active structure.
    // NOTE: This is used for import/export of the model.
    bool mergeImportExportModel(FeedsImportExportModel* model, RootItem* target_root_node, QString& output_message);

    int m_spacingSameHostsRequests;
    QHash<QString, QDateTime> m_spacingHosts;
    QMutex m_spacingMutex;
    QAction* m_actionFetchMetadata;
};

#endif // STANDARDSERVICEROOT_H
