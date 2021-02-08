// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/feedlynetwork.h"

#include "3rd-party/boolinq/boolinq.h"
#include "miscellaneous/application.h"
#include "network-web/networkfactory.h"

#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/label.h"
#include "services/abstract/labelsnode.h"
#include "services/feedly/definitions.h"
#include "services/feedly/feedlyfeed.h"
#include "services/feedly/feedlyserviceroot.h"

#if defined (FEEDLY_OFFICIAL_SUPPORT)
#include "network-web/oauth2service.h"
#endif

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

FeedlyNetwork::FeedlyNetwork(QObject* parent)
  : QObject(parent), m_service(nullptr),
#if defined (FEEDLY_OFFICIAL_SUPPORT)
  m_oauth(new OAuth2Service(QSL(FEEDLY_API_URL_BASE) + FEEDLY_API_URL_AUTH,
                            QSL(FEEDLY_API_URL_BASE) + FEEDLY_API_URL_TOKEN,
                            "dontknow",
                            "dontknow",
                            FEEDLY_API_SCOPE, this)),
#endif
  m_username(QString()),
  m_developerAccessToken(QString()), m_batchSize(FEEDLY_UNLIMITED_BATCH_SIZE) {

#if defined (FEEDLY_OFFICIAL_SUPPORT)
  m_oauth->setRedirectUrl(QString(OAUTH_REDIRECT_URI) + QL1C(':') + QString::number(FEEDLY_API_REDIRECT_URI_PORT));

  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &FeedlyNetwork::onTokensError);
  connect(m_oauth, &OAuth2Service::authFailed, this, &FeedlyNetwork::onAuthFailed);
  connect(m_oauth, &OAuth2Service::tokensReceived, this, &FeedlyNetwork::onTokensReceived);
#endif
}

QString FeedlyNetwork::username() const {
  return m_username;
}

void FeedlyNetwork::setUsername(const QString& username) {
  m_username = username;
}

QString FeedlyNetwork::developerAccessToken() const {
  return m_developerAccessToken;
}

void FeedlyNetwork::setDeveloperAccessToken(const QString& dev_acc_token) {
  m_developerAccessToken = dev_acc_token;
}

int FeedlyNetwork::batchSize() const {
  return m_batchSize;
}

void FeedlyNetwork::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

#if defined (FEEDLY_OFFICIAL_SUPPORT)

void FeedlyNetwork::onTokensError(const QString& error, const QString& error_description) {
  Q_UNUSED(error)

  qApp->showGuiMessage(tr("Feedly: authentication error"),
                       tr("Click this to login again. Error is: '%1'").arg(error_description),
                       QSystemTrayIcon::MessageIcon::Critical,
                       nullptr, false,
                       [this]() {
    m_oauth->setAccessToken({});
    m_oauth->setRefreshToken({});
    m_oauth->login();
  });
}

void FeedlyNetwork::onAuthFailed() {
  qApp->showGuiMessage(tr("Feedly: authorization denied"),
                       tr("Click this to login again."),
                       QSystemTrayIcon::MessageIcon::Critical,
                       nullptr, false,
                       [this]() {
    m_oauth->login();
  });
}

void FeedlyNetwork::onTokensReceived(const QString& access_token, const QString& refresh_token, int expires_in) {
  Q_UNUSED(expires_in)
  Q_UNUSED(access_token)

  if (m_service != nullptr && !refresh_token.isEmpty()) {
    QSqlDatabase database = qApp->database()->connection(metaObject()->className());

    //DatabaseQueries::storeNewInoreaderTokens(database, refresh_token, m_service->accountId());

    qApp->showGuiMessage(tr("Logged in successfully"),
                         tr("Your login to Feedly was authorized."),
                         QSystemTrayIcon::MessageIcon::Information);
  }
}

OAuth2Service* FeedlyNetwork::oauth() const {
  return m_oauth;
}

void FeedlyNetwork::setOauth(OAuth2Service* oauth) {
  m_oauth = oauth;
}

#endif

void FeedlyNetwork::setService(FeedlyServiceRoot* service) {
  m_service = service;
}
