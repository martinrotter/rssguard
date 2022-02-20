// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/newsblur/newsblurnetwork.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedfetchexception.h"
#include "exceptions/networkexception.h"
#include "miscellaneous/application.h"
#include "network-web/networkfactory.h"
#include "network-web/webfactory.h"
#include "services/abstract/category.h"
#include "services/abstract/label.h"
#include "services/abstract/labelsnode.h"
#include "services/newsblur/definitions.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

NewsBlurNetwork::NewsBlurNetwork(QObject* parent)
  : QObject(parent), m_root(nullptr), m_username(QString()), m_password(QString()), m_baseUrl(QSL(NEWSBLUR_URL)),
  m_batchSize(NEWSBLUR_DEFAULT_BATCH_SIZE),
  m_downloadOnlyUnreadMessages(false) {
  clearCredentials();
}

RootItem* NewsBlurNetwork::categoriesFeedsLabelsTree(const QNetworkProxy& proxy) {
  QJsonDocument json = feeds(proxy);
  RootItem* root = new RootItem();
  const auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QMap<QString, RootItem*> cats;
  QList<QPair<RootItem*, QJsonArray>> cats_array = {
    { root, json.object()["folders"].toArray() }
  };

  while (!cats_array.isEmpty()) {
    // Add direct descendants as categories to parent, then process their children.
    QPair<RootItem*, QJsonArray> cats_for_parent = cats_array.takeFirst();

    for (const QJsonValue& var : cats_for_parent.second) {
      if (var.type() == QJsonValue::Type::Double) {
        // We have feed.
        Feed* feed = new Feed();

        feed->setCustomId(QString::number(var.toInt()));

        QJsonObject feed_json = json.object()["feeds"].toObject()[feed->customId()].toObject();

        feed->setTitle(feed_json["feed_title"].toString());
        feed->setSource(feed_json["feed_link"].toString());

        QString favicon_url = feed_json["favicon_url"].toString();

        if (!favicon_url.isEmpty()) {
          QIcon icon;

          if (NetworkFactory::downloadIcon({ { favicon_url, true } }, timeout, icon, {}, proxy) ==
              QNetworkReply::NetworkError::NoError) {
            feed->setIcon(icon);
          }
        }

        cats_for_parent.first->appendChild(feed);
      }
      else if (var.type() == QJsonValue::Type::Object) {
        const QString category_name = var.toObject().keys().first();
        Category* category = new Category();

        category->setTitle(category_name);
        category->setCustomId(category_name);

        cats_for_parent.first->appendChild(category);
        cats_array.append({
          category,
          var.toObject()[category_name].toArray()
        });
      }
    }
  }

  return root;
}

QJsonDocument NewsBlurNetwork::feeds(const QNetworkProxy& proxy) {
  ensureLogin(proxy);

  const QString full_url = generateFullUrl(Operations::Feeds);
  const auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray output;
  auto network_result = NetworkFactory::performNetworkOperation(full_url,
                                                                timeout,
                                                                {},
                                                                output,
                                                                QNetworkAccessManager::Operation::GetOperation,
                                                                {},
                                                                false,
                                                                {},
                                                                {},
                                                                proxy);

  if (network_result.m_networkError == QNetworkReply::NetworkError::NoError) {
    ApiResult res; res.decodeBaseResponse(output);

    return res.m_json;
  }
  else {
    throw NetworkException(network_result.m_networkError, output);
  }
}

LoginResult NewsBlurNetwork::login(const QNetworkProxy& proxy) {
  const QString full_url = generateFullUrl(Operations::Login);
  const auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  const QString data = QSL("username=%1&password=%2").arg(m_username, m_password);
  QByteArray output;
  auto network_result = NetworkFactory::performNetworkOperation(full_url,
                                                                timeout,
                                                                data.toUtf8(),
                                                                output,
                                                                QNetworkAccessManager::Operation::PostOperation,
                                                                { {
                                                                  QSL(HTTP_HEADERS_CONTENT_TYPE).toLocal8Bit(),
                                                                  QSL("application/x-www-form-urlencoded").toLocal8Bit()
                                                                } },
                                                                false,
                                                                {},
                                                                {},
                                                                proxy);

  if (network_result.m_networkError == QNetworkReply::NetworkError::NoError) {
    LoginResult res; res.decodeBaseResponse(output);

    res.m_userId = res.m_json.object()["user_id"].toInt();
    res.m_sessiodId = boolinq::from(network_result.m_cookies).firstOrDefault([](const QNetworkCookie& c) {
      return c.name() == QSL(NEWSBLUS_AUTH_COOKIE);
    }).value();

    return res;
  }
  else {
    throw NetworkException(network_result.m_networkError, output);
  }
}

QString NewsBlurNetwork::username() const {
  return m_username;
}

void NewsBlurNetwork::setUsername(const QString& username) {
  m_username = username;
}

QString NewsBlurNetwork::password() const {
  return m_password;
}

void NewsBlurNetwork::setPassword(const QString& password) {
  m_password = password;
}

QString NewsBlurNetwork::baseUrl() const {
  return m_baseUrl;
}

void NewsBlurNetwork::setBaseUrl(const QString& base_url) {
  m_baseUrl = base_url;
}

QPair<QByteArray, QByteArray> NewsBlurNetwork::authHeader() const {
  return { QSL("Cookie").toLocal8Bit(),
           QSL("newsblur_sessionid=%1").arg(m_authSid).toLocal8Bit() };
}

void NewsBlurNetwork::ensureLogin(const QNetworkProxy& proxy) {
  if (m_authSid.isEmpty()) {
    try {
      auto log = login(proxy);

      if (log.m_authenticated && !log.m_sessiodId.isEmpty()) {
        m_authSid = log.m_sessiodId;
      }
      else {
        throw ApplicationException(log.m_errors.join(QSL(", ")));
      }
    }
    catch (const NetworkException& ex) {
      throw ex;
    }
    catch (const ApplicationException& ex) {
      throw ex;
    }
  }
}

int NewsBlurNetwork::batchSize() const {
  return m_batchSize;
}

void NewsBlurNetwork::setBatchSize(int batch_size) {
  m_batchSize = batch_size;
}

void NewsBlurNetwork::clearCredentials() {
  m_authSid = QString{};
  m_userId = {};
}

QString NewsBlurNetwork::sanitizedBaseUrl() const {
  QString base_url = m_baseUrl;

  if (!base_url.endsWith('/')) {
    base_url = base_url + QL1C('/');
  }

  return base_url;
}

QString NewsBlurNetwork::generateFullUrl(NewsBlurNetwork::Operations operation) const {
  switch (operation) {
    case Operations::Login:
      return sanitizedBaseUrl() + QSL(NEWSBLUR_API_LOGIN);

    case Operations::Feeds:
      return sanitizedBaseUrl() + QSL(NEWSBLUR_API_FEEDS);

    default:
      return sanitizedBaseUrl();
  }
}

void NewsBlurNetwork::setRoot(NewsBlurServiceRoot* root) {
  m_root = root;
}

bool NewsBlurNetwork::downloadOnlyUnreadMessages() const {
  return m_downloadOnlyUnreadMessages;
}

void NewsBlurNetwork::setDownloadOnlyUnreadMessages(bool download_only_unread) {
  m_downloadOnlyUnreadMessages = download_only_unread;
}

void ApiResult::decodeBaseResponse(const QByteArray& json_data) {
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(json_data, &err);

  if (err.error != QJsonParseError::ParseError::NoError) {
    throw ApplicationException(err.errorString());
  }

  m_json = doc;
  m_authenticated = doc.object()["authenticated"].toBool();
  m_code = doc.object()["code"].toInt();

  QStringList errs;
  QJsonObject obj_errs = doc.object()["errors"].toObject();

  for (const QString& key : obj_errs.keys()) {
    for (const QJsonValue& val: obj_errs.value(key).toArray()) {
      errs << val.toString();
    }
  }

  m_errors = errs;
}
