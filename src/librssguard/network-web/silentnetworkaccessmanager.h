// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SILENTNETWORKACCESSMANAGER_H
#define SILENTNETWORKACCESSMANAGER_H

#include "network-web/basenetworkaccessmanager.h"

#include <QPointer>

// Network manager used for more communication for feeds.
// This network manager does not provide any GUI interaction options.
class SilentNetworkAccessManager : public BaseNetworkAccessManager {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit SilentNetworkAccessManager(QObject* parent = 0);
    virtual ~SilentNetworkAccessManager();

    // Returns pointer to global silent network manager
    static SilentNetworkAccessManager* instance();

  public slots:

    // This cannot do any GUI stuff.
    void onAuthenticationRequired(QNetworkReply* reply, QAuthenticator* authenticator);
};

#endif // SILENTNETWORKACCESSMANAGER_H
