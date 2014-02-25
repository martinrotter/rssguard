#ifndef WEBBROWSERNETWORKACCESSMANAGER_H
#define WEBBROWSERNETWORKACCESSMANAGER_H

#include "core/basenetworkaccessmanager.h"

#include <QPointer>


// This is network access manager for web browsers.
class WebBrowserNetworkAccessManager : public BaseNetworkAccessManager {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit WebBrowserNetworkAccessManager(QObject *parent = 0);
    virtual ~WebBrowserNetworkAccessManager();

    // Returns pointer to global network access manager
    // used by ALL web browsers.
    // NOTE: All web browsers use shared network access manager,
    // which makes setting of custom network settings easy.
    static WebBrowserNetworkAccessManager *instance();

  protected slots:
    void onAuthenticationRequired(QNetworkReply * reply, QAuthenticator *authenticator);

  private:
    static QPointer<WebBrowserNetworkAccessManager> s_instance;
};

#endif // WEBBROWSERNETWORKACCESSMANAGER_H
