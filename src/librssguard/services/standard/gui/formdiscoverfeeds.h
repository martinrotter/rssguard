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
    explicit DiscoveredFeedsModel(QObject* parent = {});

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual int rowCount(const QModelIndex& parent) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual QVariant data(const QModelIndex& index, int role) const;

    QList<StandardFeed*> discoveredFeeds() const;
    void setDiscoveredFeeds(const QList<StandardFeed*>& newDiscoveredFeeds);

  private:
    QList<StandardFeed*> m_discoveredFeeds;
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
    void loadDiscoveredFeeds(const QList<StandardFeed*>& feeds);
    void loadCategories(const QList<Category*>& categories, RootItem* root_item);

  private:
    Ui::FormDiscoverFeeds m_ui;
    QPushButton* m_btnImportSelectedFeeds;
    ServiceRoot* m_serviceRoot;
    QList<FeedParser*> m_parsers;
    QFutureWatcher<QList<StandardFeed*>> m_watcherLookup;
    DiscoveredFeedsModel* m_discoveredModel;
};

#endif // FORMDISCOVERFEEDS_H
