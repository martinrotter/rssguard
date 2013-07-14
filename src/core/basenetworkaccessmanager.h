#ifndef BASENETWORKACCESSMANAGER_H
#define BASENETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>


class BaseNetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT
  public:
    explicit BaseNetworkAccessManager(QObject *parent = 0);

  public slots:
    void loadSettings();

  protected:
    QNetworkReply *createRequest(Operation op,
                                 const QNetworkRequest &request,
                                 QIODevice *outgoingData);
};

#endif // BASENETWORKACCESSMANAGER_H
