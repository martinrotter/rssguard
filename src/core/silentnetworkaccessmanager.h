#ifndef SILENTNETWORKACCESSMANAGER_H
#define SILENTNETWORKACCESSMANAGER_H

#include "core/basenetworkaccessmanager.h"

#include <QPointer>


// Network manager used for more communication for feeds.
class SilentNetworkAccessManager : public BaseNetworkAccessManager {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit SilentNetworkAccessManager(QObject *parent = 0);
    virtual ~SilentNetworkAccessManager();

  protected slots:
    void onAuthenticationRequired(QNetworkReply * reply, QAuthenticator *authenticator);
};

#endif // SILENTNETWORKACCESSMANAGER_H
