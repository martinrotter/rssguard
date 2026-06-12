// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FORMDISCOVERFEEDS_H
#define FORMDISCOVERFEEDS_H

#include "src/parsers/feedparser.h"

#include "ui_formdiscoverfeeds.h"

#include <librssguard/services/abstract/accountcheckmodel.h>

#include <QDialog>
#include <QFutureWatcher>
#include <QHash>
#include <QUrl>

class ServiceRoot;
class RootItem;
class Category;

class DiscoveredFeedsModel : public AccountCheckModel {
    Q_OBJECT

  public:
    explicit DiscoveredFeedsModel(QObject* parent = {});

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    virtual int columnCount(const QModelIndex& parent) const;
    virtual QVariant data(const QModelIndex& index, int role) const;

    RootItem* removeItem(RootItem* it);
    RootItem* removeItem(const QModelIndex& idx);
};

class FormDiscoverFeeds : public QDialog {
    Q_OBJECT

  public:
    struct DiscoverTask {
        const FeedParser* m_parser;
        QUrl m_url;
        QList<DocumentWithUrl> m_documents;
    };

    struct DiscoverDocumentsTask {
        QUrl m_url;
        bool m_deepDiscovery;
    };

    struct DiscoverDocumentsResult {
        QUrl m_url;
        QList<DocumentWithUrl> m_documents;
    };

    explicit FormDiscoverFeeds(ServiceRoot* service_root,
                               RootItem* parent_to_select = {},
                               const QString& url = {},
                               QWidget* parent = {});
    virtual ~FormDiscoverFeeds();

  protected:
    virtual void closeEvent(QCloseEvent* event);

  private slots:
    void discoverFeeds();
    void onUrlChanged();
    void addSingleFeed();
    void importSelectedFeeds();

    void onFeedSelectionChanged();
    void onDiscoveryProgress(int progress);
    void onDocumentsProgressMaximumIncreased(int increase);
    void onDocumentsProgressStepFinished();
    void onDocumentsFinished();
    void onDiscoveryFinished();

  private:
    StandardFeed* selectedFeed() const;
    RootItem* targetParent() const;

    QList<StandardFeed*> discoverFeedsWithParser(const FeedParser* parser,
                                                 const QUrl& url,
                                                 bool deep_discovery,
                                                 const QList<DocumentWithUrl>& documents);
    DiscoverDocumentsResult fetchDocumentsForUrl(const DiscoverDocumentsTask& task);
    void startDiscoveringFeeds(const QHash<QUrl, QList<DocumentWithUrl>>& documents_by_url);

    void userWantsAdvanced();
    void loadDiscoveredFeeds(const QList<StandardFeed*>& feeds);
    void loadCategories(const QList<Category*>& categories, RootItem* root_item);

  private:
    Ui::FormDiscoverFeeds m_ui;
    QPushButton* m_btnGoAdvanced;
    ServiceRoot* m_serviceRoot;
    QList<FeedParser*> m_parsers;
    QFutureWatcher<DiscoverDocumentsResult> m_watcherDocuments;
    QFutureWatcher<QList<StandardFeed*>> m_watcherLookup;
    DiscoveredFeedsModel* m_discoveredModel;
    bool m_deepDiscovery;
};

#endif // FORMDISCOVERFEEDS_H
