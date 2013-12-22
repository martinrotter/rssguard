#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QTreeView>


class FeedsModel;
class FeedsProxyModel;
class FeedsModelFeed;

class FeedsView : public QTreeView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsView(QWidget *parent = 0);
    virtual ~FeedsView();

    void setSortingEnabled(bool enable);
    
    // Returns list of selected feeds.
    QList<FeedsModelFeed*> selectedFeeds() const;

    QList<FeedsModelFeed*> allFeeds() const;

  public slots:
    // Reloads count for selected feeds.
    void updateCountsOfSelectedFeeds();

  protected:
    // Sets up appearance of this widget.
    void setupAppearance();


    void selectionChanged(const QItemSelection &selected,
                          const QItemSelection &deselected);

  signals:
    void feedsSelected(const QList<int> feed_ids);

  private:
    FeedsModel *m_sourceModel;
    FeedsProxyModel *m_proxyModel;
};

#endif // FEEDSVIEW_H
