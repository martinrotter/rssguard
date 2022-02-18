// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/reddit/redditnetworkfactory.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/networkexception.h"
#include "gui/dialogs/formmain.h"
#include "gui/tabwidget.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/reddit/definitions.h"
#include "services/reddit/redditserviceroot.h"

#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QThread>
#include <QUrl>

RedditNetworkFactory::RedditNetworkFactory(QObject* parent) : QObject(parent),
  m_service(nullptr), m_username(QString()), m_batchSize(REDDIT_DEFAULT_BATCH_SIZE),
  m_downloadOnlyUnreadMessages(false),
  m_oauth2(new OAuth2Service(QSL(REDDIT_OAUTH_AUTH_URL), QSL(REDDIT_OAUTH_TOKEN_URL),
                             {}, {}, QSL(REDDIT_OAUTH_SCOPE), this)) {
  initializeOauth();
}

void RedditNetworkFactory::setService(RedditServiceRoot* service) {
  m_service = service;
}

OAuth2Service* RedditNetworkFactory::oauth() const {
  return m_oauth2;
}

QString RedditNetworkFactory::username() const {
  return m_username;
}

int RedditNetworkFactory::batchSize() const {
  return m_batchSize;
}

void RedditNetworkFactory::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

void RedditNetworkFactory::initializeOauth() {
  m_oauth2->setUseHttpBasicAuthWithClientData(true);
  m_oauth2->setRedirectUrl(QSL(OAUTH_REDIRECT_URI) +
                           QL1C(':') +
                           QString::number(REDDIT_OAUTH_REDIRECT_URI_PORT),
                           true);

  connect(m_oauth2, &OAuth2Service::tokensRetrieveError, this, &RedditNetworkFactory::onTokensError);
  connect(m_oauth2, &OAuth2Service::authFailed, this, &RedditNetworkFactory::onAuthFailed);
  connect(m_oauth2, &OAuth2Service::tokensRetrieved, this, [this](QString access_token, QString refresh_token, int expires_in) {
    Q_UNUSED(expires_in)
    Q_UNUSED(access_token)

    if (m_service != nullptr && !refresh_token.isEmpty()) {
      QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

      DatabaseQueries::storeNewOauthTokens(database, refresh_token, m_service->accountId());
    }
  });
}

bool RedditNetworkFactory::downloadOnlyUnreadMessages() const {
  return m_downloadOnlyUnreadMessages;
}

void RedditNetworkFactory::setDownloadOnlyUnreadMessages(bool download_only_unread_messages) {
  m_downloadOnlyUnreadMessages = download_only_unread_messages;
}

void RedditNetworkFactory::setOauth(OAuth2Service* oauth) {
  m_oauth2 = oauth;
}

void RedditNetworkFactory::setUsername(const QString& username) {
  m_username = username;
}

QVariantHash RedditNetworkFactory::me(const QNetworkProxy& custom_proxy) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    throw ApplicationException(tr("you are not logged in"));
  }

  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  auto result = NetworkFactory::performNetworkOperation(QSL(REDDIT_API_GET_PROFILE),
                                                        timeout,
                                                        {},
                                                        output,
                                                        QNetworkAccessManager::Operation::GetOperation,
                                                        headers,
                                                        false,
                                                        {},
                                                        {},
                                                        custom_proxy).m_networkError;

  if (result != QNetworkReply::NetworkError::NoError) {
    throw NetworkException(result, output);
  }
  else {
    QJsonDocument doc = QJsonDocument::fromJson(output);

    return doc.object().toVariantHash();
  }
}

void RedditNetworkFactory::onTokensError(const QString& error, const QString& error_description) {
  Q_UNUSED(error)

  qApp->showGuiMessage(Notification::Event::LoginFailure, {
    tr("Reddit: authentication error"),
    tr("Click this to login again. Error is: '%1'").arg(error_description),
    QSystemTrayIcon::MessageIcon::Critical },
                       {}, {
    tr("Login"),
    [this]() {
      m_oauth2->setAccessToken(QString());
      m_oauth2->setRefreshToken(QString());
      m_oauth2->login();
    } });
}

void RedditNetworkFactory::onAuthFailed() {
  qApp->showGuiMessage(Notification::Event::LoginFailure, {
    tr("Reddit: authorization denied"),
    tr("Click this to login again."),
    QSystemTrayIcon::MessageIcon::Critical },
                       {}, {
    tr("Login"),
    [this]() {
      m_oauth2->login();
    } });
}
