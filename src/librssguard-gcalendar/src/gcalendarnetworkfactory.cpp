// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendarnetworkfactory.h"

#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/settingskeys.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/network-web/oauth2service.h>
#include <utility>

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSet>

namespace {
  QUrlQuery queryItems(const QList<QPair<QString, QString>>& items) {
    QUrlQuery query;
    query.setQueryItems(items);
    return query;
  }
} // namespace

CalendarApiException::CalendarApiException(Feed::Status status,
                                            const QString& message,
                                            const NetworkResult& network)
  : FeedFetchException(status, message, QVariant::fromValue(network)) {}

CalendarApiException::CalendarApiException(Feed::Status status, const QString& message, int http_code)
  : CalendarApiException(status, message, NetworkResult(QNetworkReply::NoError, http_code, {}, {})) {}

int CalendarApiException::httpStatus() const {
  return data().value<NetworkResult>().m_httpCode;
}

GCalendarNetworkFactory::GCalendarNetworkFactory(Context context, Transport transport)
  : m_context(std::move(context)), m_transport(transport ? std::move(transport) : send) {}

QUrl GCalendarNetworkFactory::apiUrl() {
  return QUrl(QStringLiteral("https://www.googleapis.com/calendar/v3/"));
}

GCalendarNetworkFactory::Context GCalendarNetworkFactory::networkContext(const QNetworkProxy& proxy) {
  Context context;
  context.m_proxy = proxy;
  context.m_timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
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
  oauth->setTokenRequestProxy(proxy);
  oauth->setRedirectUrl(QStringLiteral("http://127.0.0.1:0"), false);
  return oauth;
}

bool GCalendarNetworkFactory::Context::isCancelled() const {
  return m_cancelled->load();
}

void GCalendarNetworkFactory::Context::checkCancelled() const {
  if (isCancelled()) {
    throw ApplicationException(QObject::tr("Calendar operation cancelled."));
  }
}

GCalendarNetworkFactory::Response GCalendarNetworkFactory::send(const Request& input, const Context& context) {
  QList<QPair<QByteArray, QByteArray>> headers;
  headers.reserve(input.m_headers.size());
  for (auto header = input.m_headers.constBegin(); header != input.m_headers.constEnd(); ++header) {
    headers.append({header.key(), header.value()});
  }

  using Operation = QNetworkAccessManager::Operation;
  const auto operation = input.m_method == "GET"    ? Operation::GetOperation
                         : input.m_method == "POST" ? Operation::PostOperation
                                                    : Operation::CustomOperation;
  NetworkFactory::HttpRequestOptions options;
  if (operation == Operation::CustomOperation) {
    options.m_customMethod = input.m_method;
  }
  options.m_redirectPolicy = NetworkFactory::RedirectPolicy::ReturnResponse;

  Response result;
  result.m_network = NetworkFactory::performNetworkOperation(input.m_url.toString(QUrl::FullyEncoded),
                                                                 context.m_timeout,
                                                                 input.m_body,
                                                                 result.m_body,
                                                                 operation,
                                                                 headers,
                                                                 false,
                                                                 {},
                                                                 {},
                                                                 context.m_proxy,
                                                                 NetworkFactory::Http2Status::DontSet,
                                                                 NetworkFactory::CookiePolicy::IgnoreCookies,
                                                                 options);
  if (result.m_network.m_networkError == QNetworkReply::TimeoutError) {
    result.m_error = QObject::tr("Google Calendar request timed out.");
  }
  else if (result.m_network.m_networkError == QNetworkReply::OperationCanceledError) {
    result.m_error = QObject::tr("Calendar operation cancelled.");
  }
  else if (result.m_network.m_networkError != QNetworkReply::NoError) {
    result.m_error = NetworkFactory::networkErrorText(result.m_network.m_networkError);
  }
  return result;
}

QJsonObject GCalendarNetworkFactory::request(const QByteArray& method,
                                             const QByteArray& path,
                                             const QUrlQuery& query,
                                             const QJsonObject& body,
                                             const QString& etag) const {
  m_context.checkCancelled();
  const auto bearer = m_context.m_authorize ? m_context.m_authorize({}, m_context) : m_context.m_bearer;
  if (bearer.isEmpty()) {
    throw CalendarApiException(Feed::Status::AuthError, QObject::tr("Authorize the Calendar account first."), 401);
  }
  Request input;
  input.m_method = method;
  input.m_url = m_context.m_apiUrl.resolved(QUrl::fromEncoded(path));
  input.m_url.setQuery(query);
  input.m_headers = {{"Authorization", bearer.toUtf8()},
                     {"Accept", "application/json"},
                     {"Content-Type", "application/json; charset=utf-8"}};
  if (!etag.isEmpty()) {
    input.m_headers.insert("If-Match", etag.toUtf8());
  }
  if (!body.isEmpty()) {
    input.m_body = QJsonDocument(body).toJson(QJsonDocument::Compact);
  }
  bool authorization_retried = false;
  for (;;) {
    m_context.checkCancelled();
    const auto response = m_transport(input, m_context);
    m_context.checkCancelled();
    const int status = response.m_network.m_httpCode;
    QJsonParseError parse_error;
    const auto document = QJsonDocument::fromJson(response.m_body, &parse_error);
    const auto object = document.object();
    if (status == 401 && m_context.m_authorize && !authorization_retried) {
      authorization_retried = true;
      const auto renewed = m_context.m_authorize(QString::fromUtf8(input.m_headers.value("Authorization")), m_context);
      if (renewed.isEmpty()) {
        throw CalendarApiException(Feed::Status::AuthError,
                                    QObject::tr("Authorize the Calendar account first."),
                                    response.m_network);
      }
      input.m_headers["Authorization"] = renewed.toUtf8();
      continue;
    }
    if (status >= 200 && status < 300 && response.m_network.m_networkError == QNetworkReply::NoError &&
        response.m_error.isEmpty()) {
      if (parse_error.error != QJsonParseError::NoError || !document.isObject()) {
        throw CalendarApiException(Feed::Status::ParsingError,
                                    QObject::tr("Invalid JSON response from Google Calendar."),
                                    response.m_network);
      }
      return object;
    }
    const auto error = object.value(QStringLiteral("error")).toObject();
    const auto reasons = error.value(QStringLiteral("errors")).toArray();
    bool user_rate_limit = false;
    bool usage_limit = false;
    if (status == 403 || status == 429) {
      for (const auto& entry : reasons) {
        const auto reason = entry.toObject().value(QStringLiteral("reason")).toString();
        user_rate_limit |= reason == QLatin1String("rateLimitExceeded") ||
                           reason == QLatin1String("userRateLimitExceeded");
        usage_limit |= reason == QLatin1String("quotaExceeded");
      }
    }
    QString message = error.value(QStringLiteral("message")).toString();
    if (message.isEmpty()) {
      message = response.m_error.isEmpty()
                  ? QObject::tr("Google Calendar request failed (HTTP %1).").arg(status)
                  : response.m_error;
    }
    const bool account_quota = user_rate_limit || usage_limit;
    const auto feed_status = status == 401 || (status == 403 && !account_quota) ? Feed::Status::AuthError
                                                                               : Feed::Status::NetworkError;
    throw CalendarApiException(feed_status, message, response.m_network);
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
      throw CalendarApiException(Feed::Status::ParsingError,
                                  QObject::tr("Invalid Google Calendar collection response."),
                                  200);
    }
    for (const auto& value : response.value(QStringLiteral("items")).toArray()) {
      m_context.checkCancelled();
      if (!value.isObject() || value.toObject().value(QStringLiteral("id")).toString().isEmpty()) {
        throw CalendarApiException(Feed::Status::ParsingError, QObject::tr("Invalid item in Google Calendar response."), 200);
      }
      result.append(value.toObject());
    }
    const auto token_value = response.value(QStringLiteral("nextPageToken"));
    if (!token_value.isUndefined() && !token_value.isString()) {
      throw CalendarApiException(Feed::Status::ParsingError, QObject::tr("Invalid Google Calendar page token."), 200);
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
  throw CalendarApiException(Feed::Status::ParsingError, QObject::tr("Google Calendar pagination did not complete."), 200);
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
    throw CalendarApiException(Feed::Status::OtherError, QObject::tr("Invalid calendar cache window."));
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
    throw CalendarApiException(Feed::Status::OtherError, QObject::tr("Invalid recurring-event instance window."));
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
    throw CalendarApiException(Feed::Status::OtherError, QObject::tr("Missing client event ID."));
  }
  try {
    return request("POST",
                   eventPath(calendar_id),
                   queryItems({{QStringLiteral("sendUpdates"),
                                notify_attendees ? QStringLiteral("all") : QStringLiteral("none")}}),
                   resource);
  }
  catch (const CalendarApiException& error) {
    if (error.httpStatus() == 409) {
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
    throw CalendarApiException(Feed::Status::OtherError, QObject::tr("Reload the event before editing it."));
  }
  return request("PATCH",
                 eventPath(calendar_id, event_id),
                 queryItems({{QStringLiteral("sendUpdates"),
                              notify_attendees ? QStringLiteral("all") : QStringLiteral("none")}}),
                 changes,
                 etag);
}
