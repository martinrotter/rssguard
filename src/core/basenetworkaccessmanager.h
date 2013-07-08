#ifndef BASENETWORKACCESSMANAGER_H
#define BASENETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>


class BaseNetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
  public:
    explicit BaseNetworkAccessManager(QObject *parent = 0);
    
  signals:
    
  public slots:
    void loadSettings();
};

#endif // BASENETWORKACCESSMANAGER_H
