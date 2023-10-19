// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMDISCOVERFEEDS_H
#define FORMDISCOVERFEEDS_H

#include <QDialog>

#include "ui_formdiscoverfeeds.h"

#include "services/standard/parsers/feedparser.h"

#include <QFutureWatcher>

class ServiceRoot;
class RootItem;
class Category;

class DiscoveredFeedsModel : public QAbstractListModel {
    Q_OBJECT

  public:
    struct FeedItem {
        bool m_isChecked;
        StandardFeed* m_feed;
    };

    explicit DiscoveredFeedsModel(QObject* parent = {});

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual int rowCount(const QModelIndex& parent) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual bool setData(const QModelIndex& index, const QVariant& value, int role);
    virtual Qt::ItemFlags flags(const QModelIndex& index) const;

    QList<FeedItem> discoveredFeeds() const;
    void setDiscoveredFeeds(const QList<StandardFeed*>& feeds);

  private:
    QList<FeedItem> m_discoveredFeeds;
};

class FormDiscoverFeeds : public QDialog {
    Q_OBJECT

  public:
    explicit FormDiscoverFeeds(ServiceRoot* service_root,
                               RootItem* parent_to_select = {},
                               const QString& url = {},
                               QWidget* parent = {});
    virtual ~FormDiscoverFeeds();

  private slots:
    void discoverFeeds();
    void onUrlChanged(const QString& new_url);
    void addSingleFeed(StandardFeed* feed);
    void importSelectedFeeds();

  private:
    QList<StandardFeed*> discoverFeedsWithParser(const FeedParser* parser, const QString& url);

    void userWantsAdvanced();
    void loadDiscoveredFeeds(const QList<StandardFeed*>& feeds);
    void loadCategories(const QList<Category*>& categories, RootItem* root_item);

  private:
    Ui::FormDiscoverFeeds m_ui;
    QPushButton* m_btnImportSelectedFeeds;
    QPushButton* m_btnGoAdvanced;
    ServiceRoot* m_serviceRoot;
    QList<FeedParser*> m_parsers;
    QFutureWatcher<QList<StandardFeed*>> m_watcherLookup;
    DiscoveredFeedsModel* m_discoveredModel;
};

#endif // FORMDISCOVERFEEDS_H
