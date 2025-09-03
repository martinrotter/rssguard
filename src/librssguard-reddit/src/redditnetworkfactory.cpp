// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/redditnetworkfactory.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/networkexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/networkfactory.h"
#include "network-web/oauth2service.h"
#include "src/definitions.h"
#include "src/redditserviceroot.h"
#include "src/redditsubscription.h"

#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>

RedditNetworkFactory::RedditNetworkFactory(QObject* parent)
  : QObject(parent), m_service(nullptr), m_username(QString()), m_batchSize(REDDIT_DEFAULT_BATCH_SIZE),
    m_downloadOnlyUnreadMessages(false), m_oauth2(new OAuth2Service(QSL(REDDIT_OAUTH_AUTH_URL),
                                                                    QSL(REDDIT_OAUTH_TOKEN_URL),
                                                                    {},
                                                                    {},
                                                                    QSL(REDDIT_OAUTH_SCOPE),
                                                                    this)) {
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
  m_oauth2->setRedirectUrl(QSL(OAUTH_REDIRECT_URI) + QL1C(':') + QString::number(REDDIT_OAUTH_REDIRECT_URI_PORT), true);

  connect(m_oauth2, &OAuth2Service::tokensRetrieveError, this, &RedditNetworkFactory::onTokensError);
  connect(m_oauth2, &OAuth2Service::authFailed, this, &RedditNetworkFactory::onAuthFailed);
  connect(m_oauth2,
          &OAuth2Service::tokensRetrieved,
          this,
          [this](QString access_token, QString refresh_token, int expires_in) {
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
                                                        custom_proxy)
                  .m_networkError;

  if (result != QNetworkReply::NetworkError::NoError) {
    throw NetworkException(result, output);
  }
  else {
    QJsonDocument doc = QJsonDocument::fromJson(output);

    return doc.object().toVariantHash();
  }
}

QList<Feed*> RedditNetworkFactory::subreddits(const QNetworkProxy& custom_proxy) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    throw ApplicationException(tr("you are not logged in"));
  }

  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QString after;
  QList<Feed*> subs;

  do {
    QByteArray output;
    QString final_url = QSL(REDDIT_API_SUBREDDITS).arg(QString::number(100));

    if (!after.isEmpty()) {
      final_url += QSL("&after=%1").arg(after);
    }

    auto result = NetworkFactory::performNetworkOperation(final_url,
                                                          timeout,
                                                          {},
                                                          output,
                                                          QNetworkAccessManager::Operation::GetOperation,
                                                          headers,
                                                          false,
                                                          {},
                                                          {},
                                                          custom_proxy)
                    .m_networkError;

    if (result != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(result, output);
    }
    else {
      QJsonDocument doc = QJsonDocument::fromJson(output);
      QJsonObject root_doc = doc.object();

      after = root_doc["data"].toObject()["after"].toString();

      for (const QJsonValue& sub_val : root_doc["data"].toObject()["children"].toArray()) {
        const auto sub_obj = sub_val.toObject()["data"].toObject();

        RedditSubscription* new_sub = new RedditSubscription();

        new_sub->setCustomId(sub_obj["id"].toString());
        new_sub->setTitle(sub_obj["title"].toString());
        new_sub->setDescription(sub_obj["public_description"].toString());

        new_sub->setPrefixedName(sub_obj["url"].toString());

        QPixmap icon;
        QString icon_url = sub_obj["community_icon"].toString();

        if (icon_url.isEmpty()) {
          icon_url = sub_obj["icon_img"].toString();
        }

        if (icon_url.contains(QL1S("?"))) {
          icon_url = icon_url.mid(0, icon_url.indexOf(QL1S("?")));
        }

        if (!icon_url.isEmpty() &&
            NetworkFactory::downloadIcon({{icon_url, true}}, timeout, icon, headers, custom_proxy) ==
              QNetworkReply::NetworkError::NoError) {
          new_sub->setIcon(icon);
        }

        subs.append(new_sub);
      }
    }
  }
  while (!after.isEmpty());

  // posty dle jmena redditu
  // https://oauth.reddit.com/<SUBREDDIT>/new
  //
  // komenty pro post dle id postu
  // https://oauth.reddit.com/<SUBREDDIT>/comments/<ID-POSTU>

  return subs;
}

QList<Message> RedditNetworkFactory::hot(const QString& sub_name, const QNetworkProxy& custom_proxy) {
  QString bearer = m_oauth2->bearer().toLocal8Bit();

  if (bearer.isEmpty()) {
    throw ApplicationException(tr("you are not logged in"));
  }

  QList<QPair<QByteArray, QByteArray>> headers;

  headers.append(QPair<QByteArray, QByteArray>(QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(),
                                               m_oauth2->bearer().toLocal8Bit()));

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QString after;
  QList<Message> msgs;

  int desired_count = batchSize();

  do {
    int next_batch = desired_count <= 0 ? 100 : std::min(100, int(desired_count - msgs.size()));

    QByteArray output;
    QString final_url =
      QSL(REDDIT_API_HOT).arg(sub_name, QString::number(next_batch), QString::number(msgs.size()), QSL("GLOBAL"));

    if (!after.isEmpty()) {
      final_url += QSL("&after=%1").arg(after);
    }

    auto result = NetworkFactory::performNetworkOperation(final_url,
                                                          timeout,
                                                          {},
                                                          output,
                                                          QNetworkAccessManager::Operation::GetOperation,
                                                          headers,
                                                          false,
                                                          {},
                                                          {},
                                                          custom_proxy)
                    .m_networkError;

    if (result != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(result, output);
    }
    else {
      QJsonDocument doc = QJsonDocument::fromJson(output);
      QJsonObject root_doc = doc.object();

      after = root_doc["data"].toObject()["after"].toString();

      for (const QJsonValue& sub_val : root_doc["data"].toObject()["children"].toArray()) {
        const auto msg_obj = sub_val.toObject()["data"].toObject();

        Message new_msg;

        new_msg.m_customId = msg_obj["id"].toString();
        new_msg.m_title = msg_obj["title"].toString();
        new_msg.m_author = msg_obj["author"].toString();
        new_msg.m_createdFromFeed = true;
        new_msg.m_created = QDateTime::fromSecsSinceEpoch(msg_obj["created_utc"].toVariant().toLongLong(),
#if QT_VERSION >= 0x060900 // Qt >= 6.9.0
                                                          QTimeZone::utc());
#else
                                                          Qt::TimeSpec::UTC);
#endif
        new_msg.m_url = QSL("https://reddit.com") + msg_obj["permalink"].toString();
        new_msg.m_contents =
          msg_obj["description_html"]
            .toString(); // když prazdny, je poustnutej třeba obrazek či odkaz?, viz property "post_hint"?
        new_msg.m_rawContents = QJsonDocument(msg_obj).toJson(QJsonDocument::JsonFormat::Compact);

        msgs.append(new_msg);
      }
    }
  }
  while (!after.isEmpty() && (desired_count <= 0 || desired_count > msgs.size()));

  // posty dle jmena redditu
  // https://oauth.reddit.com/<SUBREDDIT>/new
  //
  // komenty pro post dle id postu
  // https://oauth.reddit.com/<SUBREDDIT>/comments/<ID-POSTU>

  return msgs;
}

void RedditNetworkFactory::onTokensError(const QString& error, const QString& error_description) {
  Q_UNUSED(error)

  qApp->showGuiMessage(Notification::Event::LoginFailure,
                       {tr("Reddit: authentication error"),
                        tr("Click this to login again. Error is: '%1'").arg(error_description),
                        QSystemTrayIcon::MessageIcon::Critical},
                       {},
                       {tr("Login"), [this]() {
                          m_oauth2->setAccessToken(QString());
                          m_oauth2->setRefreshToken(QString());
                          m_oauth2->login();
                        }});
}

void RedditNetworkFactory::onAuthFailed() {
  qApp->showGuiMessage(Notification::Event::LoginFailure,
                       {tr("Reddit: authorization denied"),
                        tr("Click this to login again."),
                        QSystemTrayIcon::MessageIcon::Critical},
                       {},
                       {tr("Login"), [this]() {
                          m_oauth2->login();
                        }});
}
