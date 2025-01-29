// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef BASENETWORKACCESSMANAGER_H
#define BASENETWORKACCESSMANAGER_H

#include "network-web/networkfactory.h"

#include <QNetworkAccessManager>

// This is base class for all network access managers.
class BaseNetworkAccessManager : public QNetworkAccessManager {
    Q_OBJECT

  public:
    explicit BaseNetworkAccessManager(QObject* parent = nullptr);

    void setSpecificHtpp2Status(NetworkFactory::Http2Status status);

  public slots:
    void loadSettings();

  protected slots:
    void onSslErrors(QNetworkReply* reply, const QList<QSslError>& error);

  protected:
    QNetworkReply* createRequest(Operation op, const QNetworkRequest& request, QIODevice* outgoingData);

  private:
    bool m_enableHttp2;
};

#endif // BASENETWORKACCESSMANAGER_H
