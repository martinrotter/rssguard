#ifndef BASEWEBPAGE_H
#define BASEWEBPAGE_H

#include <QWebPage>


class BaseWebPage : public QWebPage {
    Q_OBJECT
  public:
    explicit BaseWebPage(QObject *parent = 0);
    
  signals:
    
  public slots:
    
};

#endif // BASEWEBPAGE_H
