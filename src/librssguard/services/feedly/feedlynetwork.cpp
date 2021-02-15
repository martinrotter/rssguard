// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/feedlynetwork.h"

#include "3rd-party/boolinq/boolinq.h"
#include "exceptions/networkexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
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
                            FEEDLY_CLIENT_ID,
                            FEEDLY_CLIENT_ID,
                            FEEDLY_API_SCOPE, this)),
#endif
  m_username(QString()),
  m_developerAccessToken(QString()), m_batchSize(FEEDLY_UNLIMITED_BATCH_SIZE) {

#if defined (FEEDLY_OFFICIAL_SUPPORT)
  m_oauth->setRedirectUrl(QString(OAUTH_REDIRECT_URI) + QL1C(':') + QString::number(FEEDLY_API_REDIRECT_URI_PORT));

  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, &FeedlyNetwork::onTokensError);
  connect(m_oauth, &OAuth2Service::authFailed, this, &FeedlyNetwork::onAuthFailed);
  connect(m_oauth, &OAuth2Service::tokensRetrieved, this, &FeedlyNetwork::onTokensRetrieved);
#endif
}

RootItem* FeedlyNetwork::collections(bool obtain_icons) {
  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot obtain personal collections, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  QString target_url = fullUrl(Service::Collections);
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output_msgs;
  auto result = NetworkFactory::performNetworkOperation(target_url,
                                                        timeout,
                                                        {},
                                                        output_msgs,
                                                        QNetworkAccessManager::Operation::GetOperation,
                                                        { bearerHeader(bear) },
                                                        false,
                                                        {},
                                                        {},
                                                        m_service->networkProxy());

  if (result.first != QNetworkReply::NetworkError::NoError) {
    throw NetworkException(result.first);
  }

  return decodeCollections(output_msgs, obtain_icons, m_service->networkProxy(), timeout);
}

RootItem* FeedlyNetwork::decodeCollections(const QByteArray& json, bool obtain_icons,
                                           const QNetworkProxy& proxy, int timeout) const {
  QJsonDocument doc = QJsonDocument::fromJson(json);
  auto* parent = new RootItem();
  QList<QString> used_feeds;

  for (const QJsonValue& cat : doc.array()) {
    QJsonObject cat_obj = cat.toObject();
    auto* category = new Category(parent);

    category->setTitle(cat_obj["label"].toString());
    category->setCustomId(cat_obj["id"].toString());

    for (const QJsonValue& fee : cat["feeds"].toArray()) {
      QJsonObject fee_obj = fee.toObject();

      if (used_feeds.contains(fee_obj["id"].toString())) {
        qWarningNN << LOGSEC_FEEDLY
                   << "Feed"
                   << QUOTE_W_SPACE(fee_obj["id"].toString())
                   << "is already decoded and cannot be placed under several categories.";
        continue;
      }

      auto* feed = new FeedlyFeed(category);

      feed->setTitle(fee_obj["title"].toString());
      feed->setDescription(fee_obj["description"].toString());
      feed->setCustomId(fee_obj["id"].toString());

      if (obtain_icons) {
        QIcon icon;
        auto result = NetworkFactory::downloadIcon({ fee_obj["iconUrl"].toString(),
                                                     fee_obj["logo"].toString(),
                                                     fee_obj["website"].toString() },
                                                   timeout,
                                                   icon,
                                                   proxy);

        if (result == QNetworkReply::NetworkError::NoError && !icon.isNull()) {
          feed->setIcon(icon);
        }
      }

      used_feeds.append(feed->customId());
      category->appendChild(feed);
    }

    if (category->childCount() == 0) {
      delete category;
    }
    else {
      parent->appendChild(category);
    }
  }

  return parent;
}

QVariantHash FeedlyNetwork::profile(const QNetworkProxy& network_proxy) {
  QString bear = bearer();

  if (bear.isEmpty()) {
    qCriticalNN << LOGSEC_FEEDLY << "Cannot obtain profile information, because bearer is empty.";
    throw NetworkException(QNetworkReply::NetworkError::AuthenticationRequiredError);
  }

  QString target_url = fullUrl(Service::Profile);
  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output_msgs;

  // This method uses proxy via parameter,
  // not via "m_service" field.
  auto result = NetworkFactory::performNetworkOperation(target_url,
                                                        timeout,
                                                        {},
                                                        output_msgs,
                                                        QNetworkAccessManager::Operation::GetOperation,
                                                        { bearerHeader(bear) },
                                                        false,
                                                        {},
                                                        {},
                                                        network_proxy);

  if (result.first != QNetworkReply::NetworkError::NoError) {
    throw NetworkException(result.first);
  }

  return QJsonDocument::fromJson(output_msgs).object().toVariantHash();
}

QList<RootItem*> FeedlyNetwork::tags() {
  return {};
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
    m_oauth->logout(false);
    m_oauth->login();
  });
}

void FeedlyNetwork::onAuthFailed() {
  qApp->showGuiMessage(tr("Feedly: authorization denied"),
                       tr("Click this to login again."),
                       QSystemTrayIcon::MessageIcon::Critical,
                       nullptr, false,
                       [this]() {
    m_oauth->logout(false);
    m_oauth->login();
  });
}

void FeedlyNetwork::onTokensRetrieved(const QString& access_token, const QString& refresh_token, int expires_in) {
  Q_UNUSED(expires_in)
  Q_UNUSED(access_token)

  if (m_service != nullptr && !refresh_token.isEmpty()) {
    QSqlDatabase database = qApp->database()->connection(metaObject()->className());

    DatabaseQueries::storeNewOauthTokens(database, QSL("FeedlyAccounts"), refresh_token, m_service->accountId());
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

QString FeedlyNetwork::fullUrl(FeedlyNetwork::Service service) const {
  switch (service) {
    case Service::Profile:
      return QSL(FEEDLY_API_URL_BASE) + FEEDLY_API_URL_PROFILE;

    case Service::Collections:
      return QSL(FEEDLY_API_URL_BASE) + FEEDLY_API_URL_COLLETIONS;

    default:
      return FEEDLY_API_URL_BASE;
  }
}

QString FeedlyNetwork::bearer() const {
#if defined (FEEDLY_OFFICIAL_SUPPORT)
  if (m_developerAccessToken.simplified().isEmpty()) {
    return m_oauth->bearer().toLocal8Bit();
  }
#endif

  return QString("Bearer %1").arg(m_developerAccessToken);
}

QPair<QByteArray, QByteArray> FeedlyNetwork::bearerHeader(const QString& bearer) const {
  return { QString(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), bearer.toLocal8Bit() };
}

void FeedlyNetwork::setService(FeedlyServiceRoot* service) {
  m_service = service;
}
