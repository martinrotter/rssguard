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

    // Returns pointer to global network access manager
    // used by ALL feed downloaders.
    static SilentNetworkAccessManager *instance();

  protected slots:
    void onAuthenticationRequired(QNetworkReply * reply, QAuthenticator *authenticator);

  private:
    static QPointer<SilentNetworkAccessManager> s_instance;
};

#endif // SILENTNETWORKACCESSMANAGER_H
