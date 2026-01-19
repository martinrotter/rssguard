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

#include <librssguard/network-web/webfactory.h>

#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QUrl>
#include <QUrlQuery>

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

QString RedditNetworkFactory::prefixedSubredditToBare(const QString& subr) const {
  QString subr_edit = subr;
  return subr_edit.remove(QSL("/r/")).remove(QL1C('/'));
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

RedditComment RedditNetworkFactory::commentFromJson(const QJsonObject& data) const {
  RedditComment c;

  c.id = data["id"].toString();
  c.parentId = data["parent_id"].toString();
  c.author = data["author"].toString();
  c.bodyHtml = cleanScOnOff(data["body_html"].toString());
  c.bodyText = data["body"].toString();
  c.score = data["score"].toInt();

  c.created = TextFactory::parseDateTime(1000 * data["created_utc"].toVariant().toLongLong());

  return c;
}

QJsonArray RedditNetworkFactory::fetchMoreChildren(const QString& link_fullname,
                                                   const QStringList& children_ids,
                                                   const QNetworkProxy& proxy) const {
  if (children_ids.isEmpty()) {
    return {};
  }

  QList<QPair<QByteArray, QByteArray>> headers;
  headers.append({QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), m_oauth2->bearer().toLocal8Bit()});

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  QString url = QSL("https://oauth.reddit.com/api/morechildren?");
  QUrlQuery query;

  query.addQueryItem(QSL("api_type"), QSL("json"));
  query.addQueryItem(QSL("link_id"), link_fullname);
  query.addQueryItem(QSL("children"), children_ids.join(','));
  query.addQueryItem(QSL("raw_json"), QSL("1"));

  url += query.toString();

  QByteArray output;

  auto result = NetworkFactory::performNetworkOperation(url,
                                                        timeout,
                                                        {},
                                                        output,
                                                        QNetworkAccessManager::GetOperation,
                                                        headers,
                                                        false,
                                                        {},
                                                        {},
                                                        proxy)
                  .m_networkError;

  if (result != QNetworkReply::NoError) {
    throw NetworkException(result, output);
  }

  QJsonDocument doc = QJsonDocument::fromJson(output);
  return doc["json"].toObject()["data"].toObject()["things"].toArray();
}

QList<RedditComment> RedditNetworkFactory::parseCommentTree(const QJsonArray& children,
                                                            const QString& link_fullname,
                                                            const QNetworkProxy& proxy) const {
  QList<RedditComment> result;

  for (const QJsonValue& val : children) {
    QJsonObject obj = val.toObject();
    QString kind = obj["kind"].toString();
    QJsonObject data = obj["data"].toObject();

    if (kind == QSL("t1")) {
      RedditComment c = commentFromJson(data);

      if (data["replies"].isObject()) {
        auto replyChildren = data["replies"].toObject()["data"].toObject()["children"].toArray();

        c.replies = parseCommentTree(replyChildren, link_fullname, proxy);
      }

      result.append(c);
    }
    else if (kind == QSL("more")) {
      QStringList ids;
      for (const QJsonValue& id : data["children"].toArray()) {
        ids.append(id.toString());
      }

      // Fetch missing comments
      QJsonArray expanded = fetchMoreChildren(link_fullname, ids, proxy);

      // Recursively parse fetched comments
      auto expandedComments = parseCommentTree(expanded, link_fullname, proxy);

      result.append(expandedComments);
    }
  }

  return result;
}

QList<RedditComment> RedditNetworkFactory::commentsTree(const QString& subreddit,
                                                        const QString& post_id,
                                                        const QNetworkProxy& proxy) const {
  if (m_oauth2->bearer().isEmpty()) {
    throw ApplicationException(tr("you are not logged in"));
  }

  QList<QPair<QByteArray, QByteArray>> headers;
  headers.append({QSL(HTTP_HEADERS_AUTHORIZATION).toLocal8Bit(), m_oauth2->bearer().toLocal8Bit()});

  int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  QByteArray output;

  QString url = QSL("https://oauth.reddit.com/r/%1/comments/%2?limit=0&raw_json=1").arg(subreddit, post_id);

  auto result = NetworkFactory::performNetworkOperation(url,
                                                        timeout,
                                                        {},
                                                        output,
                                                        QNetworkAccessManager::GetOperation,
                                                        headers,
                                                        false,
                                                        {},
                                                        {},
                                                        proxy)
                  .m_networkError;

  if (result != QNetworkReply::NoError) {
    throw NetworkException(result, output);
  }

  QJsonDocument doc = QJsonDocument::fromJson(output);
  if (!doc.isArray()) {
    throw ApplicationException(tr("Invalid Reddit response"));
  }

  QJsonArray root = doc.array();
  if (root.size() < 2) {
    return {};
  }

  // ✅ CORRECT way
  QString link_fullname = QSL("t3_") + post_id;

  QJsonArray initialComments = root.at(1).toObject()["data"].toObject()["children"].toArray();

  return parseCommentTree(initialComments, link_fullname, proxy);
}

void RedditNetworkFactory::renderCommentHtml(const RedditComment& c, QString& html, int depth) const {
  const int indent_px = 4;

  html += QSL("<div style=\""
              "  margin-left: %1px;"
              "  border-left: 1px solid #ccc;"
              "  padding-left: 8px;"
              "  margin-top: 8px;"
              "\">")
            .arg(indent_px);

  // Header.
  html += QSL("<div style=\"font-size: 0.9em; color: #555;\">"
              "<b>%1</b> · %2 · score %3"
              "</div>")
            .arg(c.author.toHtmlEscaped(), c.created.toString(Qt::ISODate), QString::number(c.score));

  // Body.
  if (!c.bodyHtml.isEmpty()) {
    html += QSL("<div class=\"comment-body\">%1</div>").arg(c.bodyHtml);
  }
  else {
    html += QSL("<div class=\"comment-body\"><pre>%1</pre></div>").arg(c.bodyText.toHtmlEscaped());
  }

  // Replies.
  for (const RedditComment& reply : c.replies) {
    renderCommentHtml(reply, html, depth + 1);
  }

  html += QSL("</div>");
}

QString RedditNetworkFactory::commentsTreeToHtml(const QList<RedditComment>& comments,
                                                 const QString& post_title,
                                                 const QString& post_url) const {
  QString html = QSL("<hr>");

  if (comments.isEmpty()) {
    html += QSL("<i>No comments.</i>");
  }
  else {
    for (const RedditComment& c : comments) {
      renderCommentHtml(c, html, 0);
    }
  }

  return html;
}

QString RedditNetworkFactory::cleanScOnOff(const QString& html) const {
  static QRegularExpression sc_on_off(QSL("(<|&lt;)!-- ?SC_(ON|OFF) ?--(>|&gt;) ?"));

  QString loc_html = html;
  loc_html.remove(sc_on_off);

  return loc_html;
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
        const auto msg_obj = sub_val.toObject().value("data").toObject();

        Message new_msg;

        new_msg.m_customId = msg_obj["id"].toString();
        new_msg.m_title = msg_obj["title"].toString();
        new_msg.m_author = msg_obj["author"].toString();
        new_msg.m_createdFromFeed = true;
        new_msg.m_created = TextFactory::parseDateTime(1000 * msg_obj["created_utc"].toVariant().toLongLong());
        new_msg.m_url = QSL("https://reddit.com") + msg_obj["permalink"].toString();
        new_msg.m_rawContents = QJsonDocument(msg_obj).toJson(QJsonDocument::JsonFormat::Compact);
        new_msg.m_contents = WebFactory::unescapeHtml(msg_obj["selftext_html"].toString());

        if (new_msg.m_contents.isEmpty() && msg_obj.contains(QSL("url"))) {
          // NOTE: Post does not have text, maybe URL?
          new_msg.m_contents = QSL("<a href=\"%1\">%2</a>")
                                 .arg(msg_obj.value(QSL("url")).toString(), msg_obj.value(QSL("domain")).toString());
        }

        // auto cmnts = commentsTree(msg_obj.value(QSL("subreddit")).toString(), new_msg.m_customId, custom_proxy);
        // new_msg.m_contents += commentsTreeToHtml(cmnts, new_msg.m_title, new_msg.m_url);

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
