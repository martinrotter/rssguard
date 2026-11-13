// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendarnetworkfactory.h"

#include <algorithm>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/settingskeys.h>
#include <librssguard/network-web/oauth2service.h>
#include <librssguard/network-web/webfactory.h>
#include <utility>

#include <QEventLoop>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QUuid>

namespace {
  QUrlQuery queryItems(const QList<QPair<QString, QString>>& items) {
    QUrlQuery query;
    query.setQueryItems(items);
    return query;
  }
} // namespace

GCalendarNetworkFactory::GCalendarNetworkFactory(Context context, Transport transport)
  : m_context(std::move(context)), m_transport(transport ? std::move(transport) : send) {}

GCalendarNetworkFactory::Context GCalendarNetworkFactory::networkContext(const QNetworkProxy& proxy) {
  Context context;
  context.m_proxy = proxy;
  context.m_timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  context.m_http2 = qApp->settings()->value(GROUP(Network), SETTING(Network::EnableHttp2)).toBool();
  context.m_userAgent = qApp->web()->customUserAgent().toUtf8();
  if (context.m_userAgent.isEmpty()) {
    context.m_userAgent = qApp->web()->defaultUserAgent().toUtf8();
  }
  return context;
}

OAuth2Service* GCalendarNetworkFactory::createOAuth(const QString& client_id,
                                                    const QString& client_secret,
                                                    const QNetworkProxy& proxy,
                                                    QObject* parent) {
  auto* oauth = new OAuth2Service(QStringLiteral("https://accounts.google.com/o/oauth2/v2/auth"),
                                  QStringLiteral("https://oauth2.googleapis.com/token"),
                                  client_id,
                                  client_secret,
                                  QStringLiteral("https://www.googleapis.com/auth/calendar.calendarlist.readonly "
                                                 "https://www.googleapis.com/auth/calendar.events"),
                                  parent);
#if defined(GCALENDAR_OFFICIAL_SUPPORT)
  if (client_id.trimmed().isEmpty()) {
    oauth->setClientSecretId(QStringLiteral(GCALENDAR_CLIENT_ID));
    oauth->setClientSecretSecret(QStringLiteral(GCALENDAR_CLIENT_SECRET));
  }
#endif
  oauth->setPkceEnabled(true);
  oauth->setNetworkProxy(proxy);
  oauth->setRedirectUrl(QStringLiteral("http://127.0.0.1:0"), false);
  return oauth;
}

void GCalendarNetworkFactory::checkCancelled() const {
  if (m_context.m_cancelled->load()) {
    throw CalendarApiException(0, QObject::tr("Calendar operation cancelled."));
  }
}

void GCalendarNetworkFactory::wait(int milliseconds) const {
  while (milliseconds > 0) {
    checkCancelled();
    const int step = std::min(milliseconds, 100);
    QThread::msleep(static_cast<unsigned long>(step));
    milliseconds -= step;
  }
}

GCalendarNetworkFactory::Response GCalendarNetworkFactory::send(const Request& input, const Context& context) {
  QNetworkAccessManager manager;
  manager.setProxy(context.m_proxy);
  QNetworkRequest request(input.m_url);
  request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::ManualRedirectPolicy);
  request.setAttribute(QNetworkRequest::Http2AllowedAttribute, context.m_http2);
  for (auto header = input.m_headers.constBegin(); header != input.m_headers.constEnd(); ++header) {
    request.setRawHeader(header.key(), header.value());
  }
  auto* reply = manager.sendCustomRequest(request, input.m_method, input.m_body);
  QEventLoop loop;
  QTimer timeout;
  timeout.setSingleShot(true);
  QObject::connect(&timeout, &QTimer::timeout, reply, &QNetworkReply::abort);
  QTimer cancelled;
  QObject::connect(&cancelled, &QTimer::timeout, reply, [&]() {
    if (context.m_cancelled->load()) {
      reply->abort();
    }
  });
  QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
  timeout.start(context.m_timeout);
  cancelled.start(100);
  if (!reply->isFinished()) {
    loop.exec();
  }
  Response result;
  result.m_status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  result.m_body = reply->readAll();
  if (reply->error() != QNetworkReply::NoError) {
    result.m_error = reply->errorString();
  }
  for (const auto& header : reply->rawHeaderPairs()) {
    result.m_headers.insert(header.first.toLower(), header.second);
  }
  return result;
}

QJsonObject GCalendarNetworkFactory::request(const QByteArray& method,
                                             const QByteArray& path,
                                             const QUrlQuery& query,
                                             const QJsonObject& body,
                                             const QString& etag) const {
  checkCancelled();
  const auto bearer = m_context.m_authorize ? m_context.m_authorize({}) : m_context.m_bearer;
  if (bearer.isEmpty()) {
    throw CalendarApiException(401, QObject::tr("Authorize the Calendar account first."));
  }
  Request input;
  input.m_method = method;
  input.m_url = m_context.m_apiUrl.resolved(QUrl::fromEncoded(path));
  input.m_url.setQuery(query);
  input.m_headers = {{"Authorization", bearer.toUtf8()},
                     {"Accept", "application/json"},
                     {"Content-Type", "application/json; charset=utf-8"}};
  if (!m_context.m_userAgent.isEmpty()) {
    input.m_headers.insert("User-Agent", m_context.m_userAgent);
  }
  if (!m_context.m_language.isEmpty()) {
    input.m_headers.insert("Accept-Language", m_context.m_language);
  }
  if (!etag.isEmpty()) {
    input.m_headers.insert("If-Match", etag.toUtf8());
  }
  if (!body.isEmpty()) {
    input.m_body = QJsonDocument(body).toJson(QJsonDocument::Compact);
  }
  bool authorization_retried = false;
  for (int attempt = 0;;) {
    checkCancelled();
    const auto response = m_transport(input, m_context);
    checkCancelled();
    QJsonParseError parse_error;
    const auto document = QJsonDocument::fromJson(response.m_body, &parse_error);
    const auto object = document.object();
    if (response.m_status == 401 && m_context.m_authorize && !authorization_retried) {
      authorization_retried = true;
      input.m_headers["Authorization"] =
        m_context.m_authorize(QString::fromUtf8(input.m_headers.value("Authorization"))).toUtf8();
      continue;
    }
    if (response.m_status >= 200 && response.m_status < 300 && response.m_error.isEmpty()) {
      if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        throw CalendarApiException(response.m_status, QObject::tr("Invalid JSON response from Google Calendar."));
      }
      return object;
    }
    const auto error = object.value(QStringLiteral("error")).toObject();
    const auto reasons = error.value(QStringLiteral("errors")).toArray();
    const QString reason =
      reasons.isEmpty() ? QString() : reasons.first().toObject().value(QStringLiteral("reason")).toString();
    const bool temporary = response.m_status == 0 || response.m_status == 429 || response.m_status >= 500 ||
                           (response.m_status == 403 && (reason == QLatin1String("rateLimitExceeded") ||
                                                         reason == QLatin1String("userRateLimitExceeded")));
    if (method == "GET" && temporary && attempt < 2) {
      int delay = m_context.m_retryDelay * (1 << attempt);
      bool valid_retry;
      const int retry_after = response.m_headers.value("retry-after").toInt(&valid_retry);
      if (valid_retry && retry_after >= 0) {
        delay = int(std::min(qint64(30000), qint64(retry_after) * 1000));
      }
      wait(delay);
      ++attempt;
      continue;
    }
    QString message = error.value(QStringLiteral("message")).toString();
    if (message.isEmpty()) {
      message = response.m_error.isEmpty()
                  ? QObject::tr("Google Calendar request failed (HTTP %1).").arg(response.m_status)
                  : response.m_error;
    }
    throw CalendarApiException(response.m_status, message);
  }
}

QList<QJsonObject> GCalendarNetworkFactory::collection(const QByteArray& path,
                                                       QUrlQuery query,
                                                       const QString& kind) const {
  QList<QJsonObject> result;
  QSet<QString> tokens;
  for (int page = 0; page < 10000; ++page) {
    const auto response = request("GET", path, query);
    if (response.value(QStringLiteral("kind")).toString() != kind ||
        (response.contains(QStringLiteral("items")) && !response.value(QStringLiteral("items")).isArray())) {
      throw CalendarApiException(200, QObject::tr("Invalid Google Calendar collection response."));
    }
    for (const auto& value : response.value(QStringLiteral("items")).toArray()) {
      if (!value.isObject() || value.toObject().value(QStringLiteral("id")).toString().isEmpty()) {
        throw CalendarApiException(200, QObject::tr("Invalid item in Google Calendar response."));
      }
      result.append(value.toObject());
    }
    const auto token_value = response.value(QStringLiteral("nextPageToken"));
    if (!token_value.isUndefined() && !token_value.isString()) {
      throw CalendarApiException(200, QObject::tr("Invalid Google Calendar page token."));
    }
    const QString token = token_value.toString();
    if (token.isEmpty()) {
      return result;
    }
    if (tokens.contains(token)) {
      break;
    }
    tokens.insert(token);
    query.removeAllQueryItems(QStringLiteral("pageToken"));
    query.addQueryItem(QStringLiteral("pageToken"), token);
  }
  throw CalendarApiException(200, QObject::tr("Google Calendar pagination did not complete."));
}

QByteArray GCalendarNetworkFactory::eventPath(const QString& calendar_id, const QString& event_id) {
  QByteArray path = "calendars/" + QUrl::toPercentEncoding(calendar_id) + "/events";
  if (!event_id.isEmpty()) {
    path += '/' + QUrl::toPercentEncoding(event_id);
  }
  return path;
}

QList<QJsonObject> GCalendarNetworkFactory::calendars() const {
  return collection("users/me/calendarList",
                    queryItems({{QStringLiteral("showHidden"), QStringLiteral("true")}}),
                    QStringLiteral("calendar#calendarList"));
}

QList<QJsonObject> GCalendarNetworkFactory::events(const QString& calendar_id,
                                                   const QDateTime& from,
                                                   const QDateTime& until) const {
  if (!from.isValid() || !until.isValid() || until <= from) {
    throw CalendarApiException(0, QObject::tr("Invalid calendar cache window."));
  }
  return collection(eventPath(calendar_id),
                    queryItems({{QStringLiteral("singleEvents"), QStringLiteral("true")},
                                {QStringLiteral("showDeleted"), QStringLiteral("true")},
                                {QStringLiteral("maxResults"), QStringLiteral("2500")},
                                {QStringLiteral("timeMin"), from.toUTC().toString(Qt::ISODateWithMs)},
                                {QStringLiteral("timeMax"), until.toUTC().toString(Qt::ISODateWithMs)}}),
                    QStringLiteral("calendar#events"));
}

QJsonObject GCalendarNetworkFactory::event(const QString& calendar_id, const QString& event_id) const {
  return request("GET", eventPath(calendar_id, event_id));
}

QList<QJsonObject> GCalendarNetworkFactory::instances(const QString& calendar_id,
                                                      const QString& event_id,
                                                      const QDateTime& from,
                                                      const QDateTime& until) const {
  if (!from.isValid() || !until.isValid() || until <= from || event_id.isEmpty()) {
    throw CalendarApiException(0, QObject::tr("Invalid recurring-event instance window."));
  }
  return collection(eventPath(calendar_id, event_id) + "/instances",
                    queryItems({{QStringLiteral("showDeleted"), QStringLiteral("true")},
                                {QStringLiteral("maxResults"), QStringLiteral("2500")},
                                {QStringLiteral("timeMin"), from.toUTC().toString(Qt::ISODateWithMs)},
                                {QStringLiteral("timeMax"), until.toUTC().toString(Qt::ISODateWithMs)}}),
                    QStringLiteral("calendar#events"));
}

QJsonObject GCalendarNetworkFactory::insertEvent(const QString& calendar_id,
                                                 const QJsonObject& resource,
                                                 bool notify_attendees,
                                                 bool* recovered) const {
  if (recovered != nullptr) {
    *recovered = false;
  }
  if (resource.value(QStringLiteral("id")).toString().isEmpty()) {
    throw CalendarApiException(0, QObject::tr("Missing client event ID."));
  }
  try {
    return request("POST",
                   eventPath(calendar_id),
                   queryItems({{QStringLiteral("sendUpdates"),
                                notify_attendees ? QStringLiteral("all") : QStringLiteral("none")}}),
                   resource);
  }
  catch (const CalendarApiException& error) {
    if (error.m_status == 409) {
      if (recovered != nullptr) {
        *recovered = true;
      }
      return event(calendar_id, resource.value(QStringLiteral("id")).toString());
    }
    throw;
  }
}

QJsonObject GCalendarNetworkFactory::updateEvent(const QString& calendar_id,
                                                 const QString& event_id,
                                                 const QJsonObject& changes,
                                                 const QString& etag,
                                                 bool notify_attendees) const {
  if (etag.isEmpty()) {
    throw CalendarApiException(0, QObject::tr("Reload the event before editing it."));
  }
  return request("PATCH",
                 eventPath(calendar_id, event_id),
                 queryItems({{QStringLiteral("sendUpdates"),
                              notify_attendees ? QStringLiteral("all") : QStringLiteral("none")}}),
                 changes,
                 etag);
}

QString GCalendarNetworkFactory::newEventId() {
  return QUuid::createUuid().toString(QUuid::WithoutBraces).remove(QLatin1Char('-'));
}
