#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QTreeView>


class FeedsModel;
class FeedsProxyModel;

class FeedsView : public QTreeView {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedsView(QWidget *parent = 0);
    virtual ~FeedsView();
    
  signals:
    
  public slots:

  private:
    FeedsModel *m_sourceModel;
    FeedsProxyModel *m_proxyModel;
    
};

#endif // FEEDSVIEW_H
