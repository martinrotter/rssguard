#ifndef FEEDSVIEW_H
#define FEEDSVIEW_H

#include <QTreeView>


class FeedsView : public QTreeView {
    Q_OBJECT

  public:
    explicit FeedsView(QWidget *parent = 0);
    virtual ~FeedsView();
    
  signals:
    
  public slots:
    
};

#endif // FEEDSVIEW_H
