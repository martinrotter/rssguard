// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/parsers/mediawikiparser.h"

#include "src/definitions.h"

#include <cmath>
#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/exceptions/feedfetchexception.h>
#include <librssguard/exceptions/feedrecognizedbutfailedexception.h>
#include <librssguard/exceptions/sqlexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/settingskeys.h>
#include <librssguard/network-web/networkfactory.h>
#include <librssguard/network-web/webfactory.h>
#include <librssguard/services/abstract/serviceroot.h>
#include <limits>
#include <optional>
#include <utility>

#include <QJsonParseError>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QSet>
#include <QThread>
#include <QUrlQuery>

namespace {

  // Both streams request at most this many entries per page. A refresh returns
  // one newest batch plus one backfill page, with duplicate page IDs removed.
  constexpr int CATALOG_BATCH_SIZE = 5;
  // Bound newest-stream pagination when invalid or duplicate entries prevent
  // filling the batch. Backfill consumes exactly one page per refresh.
  constexpr int MAX_NEWEST_CATALOG_PAGES = 5;
  constexpr int STATE_VERSION = 1;
  constexpr unsigned long API_REQUEST_PAUSE_MS = 2000;
  constexpr int MAX_HTML_BYTES = 2 * 1024 * 1024;

  enum class SourceType {
    Category,
    Search
  };

  enum class CatalogStream {
    Newest,
    Backfill
  };

  struct Source {
      QUrl m_apiUrl;
      QString m_value;
      SourceType m_type;
  };

  struct FeedCandidate {
      Source m_source;
      QString m_siteName;
      bool m_requiresProbe = false;
  };

  QString catalogList(SourceType type) {
    return type == SourceType::Search ? QSL("search") : QSL("categorymembers");
  }

  QString continuationParameter(SourceType type) {
    return type == SourceType::Search ? QSL("sroffset") : QSL("cmcontinue");
  }

  std::optional<SourceType> detectApiSourceType(const QString& list) {
    if (list == QSL("categorymembers")) {
      return SourceType::Category;
    }

    if (list == QSL("search")) {
      return SourceType::Search;
    }

    return std::nullopt;
  }

  std::optional<SourceType> detectPageSourceType(const QJsonObject& config) {
    // Namespace IDs also recognize localized category names.
    if (config.value(QSL("wgNamespaceNumber")).toInt(-999) == 14) {
      return SourceType::Category;
    }

    if (config.value(QSL("wgCanonicalSpecialPageName")).toString() == QSL("Search")) {
      return SourceType::Search;
    }

    return std::nullopt;
  }

  bool validSourceValue(const QString& value) {
    return !value.isEmpty() && value.size() <= 1024;
  }

  bool isHttpUrl(const QUrl& url) {
    return url.isValid() && (url.scheme() == QSL("https") || url.scheme() == QSL("http")) && !url.host().isEmpty() &&
           url.userInfo().isEmpty();
  }

  bool sameOrigin(const QUrl& a, const QUrl& b) {
    const int default_port = a.scheme() == QSL("https") ? 443 : 80;
    return isHttpUrl(a) && isHttpUrl(b) && a.scheme() == b.scheme() &&
           a.host().compare(b.host(), Qt::CaseInsensitive) == 0 && a.port(default_port) == b.port(default_port);
  }

  std::optional<QMap<QString, QString>> queryItems(const QUrl& url) {
    QMap<QString, QString> items;

    for (const QString& part : url.query(QUrl::FullyEncoded).split(QL1C('&'), SPLIT_BEHAVIOR::SkipEmptyParts)) {
      const int eq = part.indexOf(QL1C('='));
      QString key = eq < 0 ? part : part.left(eq);
      QString value = eq < 0 ? QString() : part.mid(eq + 1);
      key.replace(QL1C('+'), QL1C(' '));
      value.replace(QL1C('+'), QL1C(' '));
      key = QUrl::fromPercentEncoding(key.toUtf8());
      value = QUrl::fromPercentEncoding(value.toUtf8());

      if (items.contains(key)) {
        return std::nullopt;
      }

      items.insert(key, value);
    }

    return items;
  }

  std::optional<Source> sourceFromUrl(const QUrl& url) {
    if (!isHttpUrl(url) || !url.path().endsWith(QSL("/api.php"), Qt::CaseInsensitive)) {
      return std::nullopt;
    }

    const auto items = queryItems(url);

    if (!items || items->value(QSL("action")) != QSL("query") || items->value(QSL("format")) != QSL("json")) {
      return std::nullopt;
    }

    const auto type = detectApiSourceType(items->value(QSL("list")));

    if (!type) {
      return std::nullopt;
    }

    const QString value = items->value(*type == SourceType::Search ? QSL("srsearch") : QSL("cmtitle")).trimmed();

    if (!validSourceValue(value)) {
      return std::nullopt;
    }

    QUrl endpoint = url;
    endpoint.setQuery(QString());
    endpoint.setFragment({});
    return Source{endpoint, value, *type};
  }

  QUrl catalogUrl(const Source& source, CatalogStream stream = CatalogStream::Newest) {
    QUrl url = source.m_apiUrl;
    QUrlQuery query;
    query.addQueryItem(QSL("action"), QSL("query"));
    query.addQueryItem(QSL("format"), QSL("json"));
    query.addQueryItem(QSL("formatversion"), QSL("2"));
    query.addQueryItem(QSL("list"), catalogList(source.m_type));

    switch (source.m_type) {
      case SourceType::Search:
        query.addQueryItem(QSL("srsearch"), source.m_value);
        query.addQueryItem(QSL("srnamespace"), QSL("0"));
        query.addQueryItem(QSL("srsort"), QSL("last_edit_desc"));
        query.addQueryItem(QSL("srprop"), QSL("snippet|timestamp|wordcount"));
        query.addQueryItem(QSL("srlimit"), QString::number(CATALOG_BATCH_SIZE));
        break;

      case SourceType::Category:
        query.addQueryItem(QSL("cmtitle"), source.m_value);
        query.addQueryItem(QSL("cmnamespace"), QSL("0"));
        query.addQueryItem(QSL("cmprop"), QSL("ids|title|timestamp"));
        query.addQueryItem(QSL("cmsort"), QSL("timestamp"));
        query.addQueryItem(QSL("cmdir"), stream == CatalogStream::Backfill ? QSL("asc") : QSL("desc"));
        query.addQueryItem(QSL("cmlimit"), QString::number(CATALOG_BATCH_SIZE));
        break;
    }

    url.setQuery(query);
    return url;
  }

  QString sourceKey(const Source& source) {
    // The canonical URL excludes caller-supplied continuation, limits and parameter order.
    return catalogUrl(source).toString(QUrl::FullyEncoded);
  }

  QJsonObject parseObject(const QByteArray& data) {
    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &error);

    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
      throw ApplicationException(error.error == QJsonParseError::NoError
                                   ? QObject::tr("MediaWiki API response is not a JSON object.")
                                   : QObject::tr("Invalid MediaWiki JSON: %1").arg(error.errorString()));
    }

    return doc.object();
  }

  void checkApiError(const QJsonObject& object) {
    const QJsonObject api_error = object.value(QSL("error")).toObject();

    if (!api_error.isEmpty()) {
      throw ApplicationException(QObject::tr("MediaWiki API error: %1").arg(api_error.value(QSL("info")).toString()));
    }
  }

  QJsonArray requireCatalog(const QJsonObject& object, SourceType type) {
    checkApiError(object);

    const QJsonValue value = object.value(QSL("query")).toObject().value(catalogList(type));

    if (!value.isArray()) {
      throw ApplicationException(QObject::tr("MediaWiki API response has no expected result list."));
    }

    return value.toArray();
  }

  qint64 pageId(const QJsonObject& item) {
    const QJsonValue value = item.value(QSL("pageid"));
    const double number = value.toDouble(-1);

    if (!value.isDouble() || !std::isfinite(number) || number <= 0 ||
        number >= double(std::numeric_limits<qint64>::max())) {
      return -1;
    }

    const qint64 id = qint64(number);
    return double(id) == number ? id : -1;
  }

  QUrl articleUrl(const QUrl& api_url, qint64 id) {
    QUrl url = api_url;
    QString path = url.path();
    path.chop(QString(QSL("api.php")).size());
    url.setPath(path + QSL("index.php"));
    QUrlQuery query;
    query.addQueryItem(QSL("curid"), QString::number(id));
    url.setQuery(query);
    url.setFragment({});
    return url;
  }

  QUrl parseUrl(const QUrl& api_url, qint64 id) {
    QUrl url = api_url;
    QUrlQuery query;
    query.addQueryItem(QSL("action"), QSL("parse"));
    query.addQueryItem(QSL("format"), QSL("json"));
    query.addQueryItem(QSL("formatversion"), QSL("2"));
    query.addQueryItem(QSL("pageid"), QString::number(id));
    query.addQueryItem(QSL("prop"), QSL("text"));
    url.setQuery(query);
    return url;
  }

  QString attribute(const QString& tag, const QString& name) {
    static const QRegularExpression attributes(QSL("([\\w:-]+)\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)'|([^\\s>]+))"));
    auto matches = attributes.globalMatch(tag);

    while (matches.hasNext()) {
      const auto match = matches.next();

      if (match.captured(1).compare(name, Qt::CaseInsensitive) == 0) {
        return !match.captured(2).isNull() ? match.captured(2)
                                           : (!match.captured(3).isNull() ? match.captured(3) : match.captured(4));
      }
    }

    return {};
  }

  bool hasGenerator(const QString& html) {
    static const QRegularExpression meta(QSL("<meta\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption);
    auto matches = meta.globalMatch(html);

    while (matches.hasNext()) {
      const QString tag = matches.next().captured();

      if (attribute(tag, QSL("name")).compare(QSL("generator"), Qt::CaseInsensitive) == 0 &&
          attribute(tag, QSL("content")).startsWith(QSL("MediaWiki"), Qt::CaseInsensitive)) {
        return true;
      }
    }

    return false;
  }

  QUrl apiFromLink(const QString& html, const QUrl& page_url) {
    static const QRegularExpression link(QSL("<link\\b[^>]*>"), QRegularExpression::CaseInsensitiveOption);
    auto matches = link.globalMatch(html);

    while (matches.hasNext()) {
      const QString tag = matches.next().captured();

      if (attribute(tag, QSL("rel")).compare(QSL("EditURI"), Qt::CaseInsensitive) != 0) {
        continue;
      }

      QUrl api_url = page_url.resolved(QUrl(WebFactory::unescapeHtml(attribute(tag, QSL("href")))));
      api_url.setQuery(QString());
      api_url.setFragment({});

      if (sameOrigin(page_url, api_url) && api_url.path().endsWith(QSL("/api.php"), Qt::CaseInsensitive)) {
        return api_url;
      }
    }

    return {};
  }

  QString configAsJson(const QString& config) {
    QString json;
    json.reserve(config.size());
    bool quoted = false;
    bool escaped = false;

    for (qsizetype i = 0; i < config.size(); ++i) {
      const QChar ch = config.at(i);

      if (quoted) {
        if (escaped) {
          escaped = false;
        }
        else if (ch == QL1C('\\')) {
          escaped = true;
        }
        else if (ch == QL1C('"')) {
          quoted = false;
        }
      }
      else if (ch == QL1C('"')) {
        quoted = true;
      }
      else if (ch == QL1C('!') && i + 1 < config.size() &&
               (config.at(i + 1) == QL1C('0') || config.at(i + 1) == QL1C('1'))) {
        qsizetype before = i - 1;
        qsizetype after = i + 2;

        while (before >= 0 && config.at(before).isSpace()) {
          --before;
        }

        while (after < config.size() && config.at(after).isSpace()) {
          ++after;
        }

        // Older MediaWiki pages minify booleans as !0/!1. Only normalize
        // complete values outside strings; all other JavaScript stays invalid.
        if (before >= 0 && after < config.size() &&
            (config.at(before) == QL1C(':') || config.at(before) == QL1C('[') || config.at(before) == QL1C(',')) &&
            (config.at(after) == QL1C(',') || config.at(after) == QL1C(']') || config.at(after) == QL1C('}'))) {
          json += config.at(i + 1) == QL1C('0') ? QSL("true") : QSL("false");
          ++i;
          continue;
        }
      }

      json += ch;
    }

    return json;
  }

  QJsonObject pageConfig(const QString& html) {
    const QStringList markers = {QSL("window.RLCONF"), QSL("RLCONF="), QSL("mw.config.set")};

    for (const QString& marker : markers) {
      const int marker_at = html.indexOf(marker);

      if (marker_at < 0) {
        continue;
      }

      const int open = html.indexOf(QL1C('{'), marker_at + marker.size());

      if (open < 0 || open - marker_at > 256) {
        continue;
      }

      int depth = 0;
      bool quoted = false;
      bool escaped = false;

      for (int i = open; i < html.size(); ++i) {
        const QChar ch = html.at(i);

        if (quoted) {
          if (escaped) {
            escaped = false;
          }
          else if (ch == QL1C('\\')) {
            escaped = true;
          }
          else if (ch == QL1C('"')) {
            quoted = false;
          }
        }
        else if (ch == QL1C('"')) {
          quoted = true;
        }
        else if (ch == QL1C('{')) {
          ++depth;
        }
        else if (ch == QL1C('}') && --depth == 0) {
          QJsonParseError error;
          const QJsonDocument doc =
            QJsonDocument::fromJson(configAsJson(html.mid(open, i - open + 1)).toUtf8(), &error);

          if (error.error == QJsonParseError::NoError && doc.isObject()) {
            return doc.object();
          }

          break;
        }
      }
    }

    return {};
  }

  std::optional<FeedCandidate> pageFromHtml(const QByteArray& data, const QUrl& page_url) {
    if (!isHttpUrl(page_url) || data.size() > MAX_HTML_BYTES) {
      return std::nullopt;
    }

    const QString html = QString::fromUtf8(data);

    if (!hasGenerator(html)) {
      return std::nullopt;
    }

    const QJsonObject config = pageConfig(html);
    const auto type = detectPageSourceType(config);

    if (!type) {
      return std::nullopt;
    }

    const QString script_path = config.value(QSL("wgScriptPath")).toString();

    QUrl server = page_url;
    const QString server_text =
      config.value(QSL("wgCanonicalServer")).toString(config.value(QSL("wgServer")).toString());

    if (!server_text.isEmpty()) {
      server = page_url.resolved(QUrl(server_text));
    }

    if (!sameOrigin(page_url, server)) {
      return std::nullopt;
    }

    QUrl api_url = apiFromLink(html, page_url);

    if (!api_url.isValid() || api_url.isEmpty()) {
      if (!config.value(QSL("wgScriptPath")).isString() ||
          (!script_path.isEmpty() && !script_path.startsWith(QL1C('/'))) || script_path.contains(QSL("..")) ||
          script_path.contains(QL1C('\\')) || script_path.contains(QL1C('?')) || script_path.contains(QL1C('#'))) {
        return std::nullopt;
      }

      api_url = server;
      api_url.setPath(script_path + QSL("/api.php"));
      api_url.setQuery(QString());
      api_url.setFragment({});
    }

    if (!sameOrigin(page_url, api_url)) {
      return std::nullopt;
    }

    QString value;

    switch (*type) {
      case SourceType::Category:
        value = config.value(QSL("wgPageName")).toString().trimmed();
        break;

      case SourceType::Search: {
        const auto query = queryItems(page_url);
        value = query ? query->value(QSL("search")).trimmed() : QString();
        break;
      }
    }

    if (!validSourceValue(value)) {
      return std::nullopt;
    }

    return FeedCandidate{Source{api_url, value, *type},
                         config.value(QSL("wgSiteName")).toString(page_url.host()),
                         true};
  }

  std::optional<FeedCandidate> detectFeed(const QByteArray& data, const QUrl& url) {
    if (const auto source = sourceFromUrl(url)) {
      requireCatalog(parseObject(data), source->m_type);
      const bool requires_probe = catalogUrl(*source).toString(QUrl::FullyEncoded) != url.toString(QUrl::FullyEncoded);
      return FeedCandidate{*source, {}, requires_probe};
    }

    return pageFromHtml(data, url);
  }

  StandardFeed* preparedFeed(const Source& source, const QString& site_name = {}) {
    auto* feed = new StandardFeed();
    feed->setType(StandardFeed::Type::MediaWiki);
    feed->setSourceType(StandardFeed::SourceType::Url);
    feed->setSource(catalogUrl(source).toString(QUrl::FullyEncoded));
    feed->setEncoding(QSL(DEFAULT_FEED_ENCODING));
    // Backfill can be much older than the application's default acceptance cutoff.
    feed->articleIgnoreLimit().m_addAnyArticlesToDb = true;
    const QString site = site_name.isEmpty() ? source.m_apiUrl.host() : site_name;
    switch (source.m_type) {
      case SourceType::Category:
        feed->setTitle(QObject::tr("Category: %1 — %2").arg(source.m_value, site));
        feed->setDescription(QObject::tr("Pages added to '%1'.").arg(source.m_value));
        break;

      case SourceType::Search:
        feed->setTitle(QObject::tr("Search: %1 — %2").arg(source.m_value, site));
        feed->setDescription(QObject::tr("Pages matching '%1', ordered by last edit.").arg(source.m_value));
        break;
    }
    return feed;
  }

  bool probeCatalog(ServiceRoot* root, const Source& source) {
    if (root == nullptr) {
      return false;
    }

    const QUrl url = catalogUrl(source);
    const int timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
    const QList<QPair<QByteArray, QByteArray>> headers = {{HTTP_HEADERS_ACCEPT, QByteArray("application/json")}};
    QByteArray data;
    const NetworkResult result = NetworkFactory::performNetworkOperation(url.toString(QUrl::FullyEncoded),
                                                                         timeout,
                                                                         {},
                                                                         data,
                                                                         QNetworkAccessManager::Operation::GetOperation,
                                                                         headers,
                                                                         {},
                                                                         {},
                                                                         {},
                                                                         root->networkProxy());

    QThread::msleep(100);

    if (result.m_networkError != QNetworkReply::NetworkError::NoError || !sameOrigin(source.m_apiUrl, result.m_url)) {
      return false;
    }

    try {
      requireCatalog(parseObject(data), source.m_type);
      return true;
    }
    catch (const ApplicationException&) {
      return false;
    }
  }

  QUrl continuationUrl(const QUrl& catalog_url, SourceType type, const QJsonObject& continuation) {
    QUrl result = catalog_url;
    QUrlQuery query(result);
    const QString position_parameter = continuationParameter(type);

    if (!continuation.isEmpty() && !continuation.contains(position_parameter)) {
      throw ApplicationException(QObject::tr("MediaWiki continuation has no catalog position."));
    }

    for (auto it = continuation.begin(); it != continuation.end(); ++it) {
      if (it.key() != QSL("continue") && it.key() != position_parameter) {
        throw ApplicationException(QObject::tr("Unexpected MediaWiki continuation parameter."));
      }

      if (!it.value().isString() && !it.value().isDouble()) {
        throw ApplicationException(QObject::tr("Invalid MediaWiki continuation parameter."));
      }

      if (it.value().isDouble() &&
          (type != SourceType::Search || it.key() != position_parameter || !std::isfinite(it.value().toDouble()) ||
           it.value().toDouble() < 0 || it.value().toDouble() > std::numeric_limits<int>::max() ||
           std::floor(it.value().toDouble()) != it.value().toDouble())) {
        throw ApplicationException(QObject::tr("Invalid MediaWiki continuation offset."));
      }

      const QString value = it.value().isString() ? it.value().toString() : QString::number(it.value().toInt());

      if (type == SourceType::Search && it.key() == position_parameter && it.value().isString()) {
        bool valid = false;
        const int offset = value.toInt(&valid);

        if (!valid || offset < 0) {
          throw ApplicationException(QObject::tr("Invalid MediaWiki continuation offset."));
        }
      }

      if (value.isEmpty() || value.size() > 2048) {
        throw ApplicationException(QObject::tr("MediaWiki continuation parameter is too long."));
      }

      query.removeAllQueryItems(it.key());
      query.addQueryItem(it.key(), value);
    }

    result.setQuery(query);
    return result;
  }

  QJsonObject catalogContinuation(const QJsonObject& response, const QUrl& catalog_url, SourceType type) {
    const QJsonValue value = response.value(QSL("continue"));

    if (!value.isUndefined() && !value.isObject()) {
      throw ApplicationException(QObject::tr("Invalid MediaWiki continuation object."));
    }

    const QJsonObject continuation = value.toObject();
    continuationUrl(catalog_url, type, continuation);
    return continuation;
  }

  QJsonObject savedBackfillContinuation(const QVariantHash& data, int feed_id, const Source& source) {
    const QJsonObject saved = QJsonObject::fromVariantHash(data).value(QSL("mediawiki")).toObject();

    if (saved.value(QSL("version")).toDouble() != STATE_VERSION || saved.value(QSL("feed_id")).toDouble() != feed_id ||
        saved.value(QSL("source_key")).toString() != sourceKey(source)) {
      return {};
    }

    const QJsonValue continuation = saved.value(QSL("backfill")).toObject().value(QSL("continuation"));

    if (!continuation.isObject()) {
      return {};
    }

    try {
      continuationUrl(catalogUrl(source, CatalogStream::Backfill), source.m_type, continuation.toObject());
      return continuation.toObject();
    }
    catch (const ApplicationException&) {
      // Invalid saved state starts another sweep; invalid API replies still fail.
      return {};
    }
  }

  struct CatalogSelection {
      QJsonArray m_items;
      QSet<qint64> m_seenIds;

      void append(const QJsonValue& value) {
        const QJsonObject item = value.toObject();
        const qint64 id = pageId(item);
        const QDateTime date = QDateTime::fromString(item.value(QSL("timestamp")).toString(), Qt::ISODate);

        if (id <= 0 || m_seenIds.contains(id) || item.value(QSL("title")).toString().trimmed().isEmpty() ||
            !date.isValid()) {
          return;
        }

        m_seenIds.insert(id);
        m_items.append(item);
      }
  };

  struct NewestCatalog {
      QJsonObject m_response;
      QJsonObject m_continuation;
      // Retain the last page's start token and response so search backfill can
      // consume its remainder without skipping entries or downloading it again.
      QJsonObject m_pageStart;
      bool m_hasUnconsumedItems = false;
  };

  NewestCatalog collectNewestItems(const Source& source,
                                   const QJsonObject& initial_response,
                                   bool has_saved_feed,
                                   CatalogSelection& selection,
                                   const std::function<QByteArray(const QUrl&)>& fetch_json) {
    const QUrl catalog_url = catalogUrl(source);
    QSet<QString> seen_tokens;
    NewestCatalog newest;
    newest.m_response = initial_response;

    for (int page = 0; page < MAX_NEWEST_CATALOG_PAGES && selection.m_items.size() < CATALOG_BATCH_SIZE; ++page) {
      const QJsonArray items = requireCatalog(newest.m_response, source.m_type);
      newest.m_continuation = catalogContinuation(newest.m_response, catalog_url, source.m_type);

      if (has_saved_feed && items.size() > CATALOG_BATCH_SIZE) {
        throw ApplicationException(QObject::tr("MediaWiki catalog exceeded the requested page size."));
      }

      int consumed = 0;

      for (const QJsonValue& item : items) {
        ++consumed;
        selection.append(item);

        if (selection.m_items.size() >= CATALOG_BATCH_SIZE) {
          newest.m_hasUnconsumedItems = consumed < items.size();
          break;
        }
      }

      if (selection.m_items.size() >= CATALOG_BATCH_SIZE || page + 1 >= MAX_NEWEST_CATALOG_PAGES ||
          newest.m_continuation.isEmpty()) {
        break;
      }

      const QString token = QString::fromUtf8(QJsonDocument(newest.m_continuation).toJson(QJsonDocument::Compact));

      if (seen_tokens.contains(token)) {
        throw ApplicationException(QObject::tr("MediaWiki API repeated a continuation token."));
      }

      seen_tokens.insert(token);

      if (!fetch_json) {
        throw ApplicationException(QObject::tr("MediaWiki continuation requires a resource handler."));
      }

      const QUrl next = continuationUrl(catalog_url, source.m_type, newest.m_continuation);

      if (!sameOrigin(source.m_apiUrl, next)) {
        throw ApplicationException(QObject::tr("MediaWiki continuation changed origin."));
      }

      newest.m_pageStart = newest.m_continuation;
      newest.m_response = parseObject(fetch_json(next));
    }

    return newest;
  }

  QJsonObject collectBackfillItems(const Source& source,
                                   QJsonObject continuation,
                                   const NewestCatalog& newest,
                                   CatalogSelection& selection,
                                   const std::function<QByteArray(const QUrl&)>& fetch_json) {
    bool reuse_newest_page = false;

    if (source.m_type == SourceType::Search && continuation.isEmpty()) {
      // Search continues after newest results; categories sweep oldest-first.
      // An empty saved token (including EOF) begins another sweep.
      reuse_newest_page = newest.m_hasUnconsumedItems;
      continuation = reuse_newest_page ? newest.m_pageStart : newest.m_continuation;

      if (continuation.isEmpty() && !reuse_newest_page) {
        return {};
      }
    }

    if (!fetch_json) {
      throw ApplicationException(QObject::tr("MediaWiki backfill requires a resource handler."));
    }

    const QUrl catalog_url = catalogUrl(source, CatalogStream::Backfill);
    const QUrl next = continuationUrl(catalog_url, source.m_type, continuation);
    const QJsonObject response = reuse_newest_page ? newest.m_response : parseObject(fetch_json(next));
    const QJsonArray items = requireCatalog(response, source.m_type);

    // Never advance a token past entries omitted by the per-refresh bound.
    if (items.size() > CATALOG_BATCH_SIZE) {
      throw ApplicationException(QObject::tr("MediaWiki backfill exceeded the requested page size."));
    }

    for (const QJsonValue& item : items) {
      selection.append(item);
    }

    const QJsonObject next_continuation = catalogContinuation(response, catalog_url, source.m_type);

    if (!next_continuation.isEmpty() && next_continuation == continuation) {
      throw ApplicationException(QObject::tr("MediaWiki API repeated a continuation token."));
    }

    return next_continuation;
  }

  QString absoluteArticleLinks(QString html, const QUrl& article_url) {
    static const QRegularExpression links(QSL("(?<![\\w:-])(href|src)\\s*=\\s*([\"'])([^\"']*)\\2"),
                                          QRegularExpression::CaseInsensitiveOption);
    auto matches = links.globalMatch(html);
    struct Replacement {
        qsizetype m_start;
        qsizetype m_length;
        QString m_value;
    };
    QList<Replacement> replacements;

    while (matches.hasNext()) {
      const auto match = matches.next();
      const QUrl target = article_url.resolved(QUrl(WebFactory::unescapeHtml(match.captured(3))));

      if (isHttpUrl(target)) {
        replacements.prepend({match.capturedStart(),
                              match.capturedLength(),
                              match.captured(1) + QL1C('=') + match.captured(2) +
                                target.toString(QUrl::FullyEncoded).toHtmlEscaped() + match.captured(2)});
      }
    }

    for (const auto& replacement : std::as_const(replacements)) {
      html.replace(replacement.m_start, replacement.m_length, replacement.m_value);
    }

    return html;
  }

} // namespace

MediaWikiParser::MediaWikiParser(const QString& data,
                                 const QUrl& source_url,
                                 std::function<QByteArray(const QUrl&)> resource_handler,
                                 std::function<void()> request_pause)
  : FeedParser(data, DataType::Json), m_sourceUrl(source_url), m_requestPause(std::move(request_pause)) {
  setResourceHandler(std::move(resource_handler));

  if (!m_requestPause) {
    m_requestPause = []() {
      QThread::msleep(API_REQUEST_PAUSE_MS);
    };
  }
}

QList<StandardFeed*> MediaWikiParser::discoverFeeds(ServiceRoot* root,
                                                    const QUrl& url,
                                                    bool deep_discovery,
                                                    const QList<DocumentWithUrl>& documents) const {
  Q_UNUSED(deep_discovery)
  QList<StandardFeed*> feeds;
  QSet<QString> seen_sources;

  for (const DocumentWithUrl& document : documents) {
    const QUrl document_url = document.m_documentUrl.isValid() ? document.m_documentUrl : url;
    std::optional<FeedCandidate> candidate;

    try {
      candidate = detectFeed(document.m_documentData, document_url);
    }
    catch (const ApplicationException&) {
      continue;
    }

    if (!candidate || (candidate->m_requiresProbe && !probeCatalog(root, candidate->m_source))) {
      continue;
    }

    const QString source_key = sourceKey(candidate->m_source);

    if (seen_sources.contains(source_key)) {
      continue;
    }

    seen_sources.insert(source_key);
    feeds.append(preparedFeed(candidate->m_source, candidate->m_siteName));
  }

  return feeds;
}

GuessedFeedWithIcons MediaWikiParser::guessFeed(const QByteArray& content, const NetworkResult& network_res) const {
  const QUrl url = network_res.m_url;
  std::optional<FeedCandidate> candidate;

  try {
    candidate = detectFeed(content, url);

    if (candidate && candidate->m_requiresProbe && m_resourceHandler) {
      requireCatalog(parseObject(m_resourceHandler(catalogUrl(candidate->m_source))), candidate->m_source.m_type);
    }
  }
  catch (const ApplicationException& ex) {
    throw FeedRecognizedButFailedException(ex.message());
  }

  if (!candidate) {
    throw ApplicationException(QObject::tr("Not a supported MediaWiki feed."));
  }

  return {preparedFeed(candidate->m_source, candidate->m_siteName), {{url.toString(), false}}};
}

QJsonArray MediaWikiParser::jsonMessageElements() {
  m_hasPendingState = false;
  m_pendingState.clear();
  m_articleHtml.clear();
  const auto source = sourceFromUrl(m_sourceUrl);

  if (!source) {
    throw FeedFetchException(Feed::Status::ParsingError, QObject::tr("Invalid MediaWiki feed source URL."));
  }

  m_sourceKey = sourceKey(*source);
  m_feedId = feed() == nullptr ? 0 : feed()->id();
  const auto feed_source = feed() == nullptr ? std::optional<Source>{} : sourceFromUrl(QUrl(feed()->source()));
  m_feedSourceKey = feed_source ? sourceKey(*feed_source) : QString();

  std::function<QByteArray(const QUrl&)> fetch_json;

  if (m_resourceHandler) {
    fetch_json = [this](const QUrl& url) {
      return fetchAdditionalJson(url);
    };
  }

  CatalogSelection selection;

  try {
    const QJsonObject backfill_continuation =
      m_feedId > 0 ? savedBackfillContinuation(loadFeedCustomData(), m_feedId, *source) : QJsonObject{};
    const NewestCatalog newest = collectNewestItems(*source, m_json.object(), m_feedId > 0, selection, fetch_json);

    if (m_feedId > 0) {
      const QJsonObject next_continuation =
        collectBackfillItems(*source, backfill_continuation, newest, selection, fetch_json);
      m_pendingState = QJsonObject{{QSL("version"), STATE_VERSION},
                                   {QSL("feed_id"), m_feedId},
                                   {QSL("source_key"), m_sourceKey},
                                   {QSL("backfill"), QJsonObject{{QSL("continuation"), next_continuation}}}}
                         .toVariantHash();
    }
  }
  catch (const SqlException&) {
    throw;
  }
  catch (const FeedFetchException&) {
    throw;
  }
  catch (const ApplicationException& ex) {
    throw FeedFetchException(Feed::Status::ParsingError, ex.message());
  }

  fetchArticleHtml(selection.m_items, source->m_apiUrl);
  // Make the staged checkpoint eligible for commit only after every article succeeds.
  m_hasPendingState = m_feedId > 0;
  return selection.m_items;
}

QByteArray MediaWikiParser::fetchAdditionalJson(const QUrl& url) const {
  // Serialize supplemental API calls from all MediaWiki feeds in this process.
  static QMutex request_mutex;
  QMutexLocker lock(&request_mutex);
  m_requestPause();
  return m_resourceHandler(url);
}

void MediaWikiParser::fetchArticleHtml(const QJsonArray& items, const QUrl& api_url) {
  if (!items.isEmpty() && !m_resourceHandler) {
    throw FeedFetchException(Feed::Status::ParsingError,
                             QObject::tr("MediaWiki article requests require a resource handler."));
  }

  for (const QJsonValue& item : items) {
    const qint64 id = pageId(item.toObject());
    const QUrl request = parseUrl(api_url, id);

    try {
      const QByteArray data = fetchAdditionalJson(request);

      if (data.toLower().contains("too many requests")) {
        throw FeedFetchException(Feed::Status::OtherError,
                                 QObject::tr("MediaWiki API rate limit reached while fetching article %1.").arg(id));
      }

      const QJsonObject article = parseObject(data);
      checkApiError(article);
      const QJsonObject parsed = article.value(QSL("parse")).toObject();
      const QString html = parsed.value(QSL("text")).toString();

      if (pageId(parsed) != id || html.isEmpty()) {
        throw ApplicationException(QObject::tr("MediaWiki article response did not match the requested page."));
      }

      m_articleHtml.insert(id, absoluteArticleLinks(html, articleUrl(api_url, id)));
    }
    catch (const FeedFetchException&) {
      throw;
    }
    catch (const ApplicationException& ex) {
      throw FeedFetchException(Feed::Status::ParsingError,
                               QObject::tr("Could not fetch MediaWiki article %1: %2").arg(id).arg(ex.message()));
    }
  }
}

QVariantHash MediaWikiParser::loadFeedCustomData() const {
  return qApp->database()->worker()->read<QVariantHash>([this](const QSqlDatabase& db) {
    return DatabaseQueries::loadFeedCustomData(db, feed());
  });
}

void MediaWikiParser::storeFeedCustomData() {
  qApp->database()->worker()->write([this](const QSqlDatabase& db) {
    DatabaseQueries::storeFeedCustomData(db, feed());
  });
}

void MediaWikiParser::commitCustomDatabaseData() {
  if (!m_hasPendingState || feed() == nullptr || feed()->id() != m_feedId) {
    return;
  }

  const auto current_source = sourceFromUrl(QUrl(feed()->source()));
  const auto* standard_feed = qobject_cast<StandardFeed*>(feed());

  if (!current_source || sourceKey(*current_source) != m_feedSourceKey ||
      (standard_feed != nullptr && standard_feed->type() != StandardFeed::Type::MediaWiki)) {
    m_hasPendingState = false;
    return;
  }

  QVariantHash data = feed()->customDatabaseData();
  const bool had_state = data.contains(QSL("mediawiki"));
  const QVariant previous_state = data.value(QSL("mediawiki"));
  data[QSL("mediawiki")] = m_pendingState;
  feed()->setCustomDatabaseData(data);

  try {
    storeFeedCustomData();
  }
  catch (...) {
    // Restore only this namespace, preserving other live settings.
    data = feed()->customDatabaseData();

    if (had_state) {
      data[QSL("mediawiki")] = previous_state;
    }
    else {
      data.remove(QSL("mediawiki"));
    }

    feed()->setCustomDatabaseData(data);
    throw;
  }

  m_hasPendingState = false;
}

QString MediaWikiParser::jsonMessageTitle(const QJsonObject& item) const {
  return item.value(QSL("title")).toString();
}

QString MediaWikiParser::jsonMessageUrl(const QJsonObject& item) const {
  return articleUrl(m_sourceUrl, pageId(item)).toString(QUrl::FullyEncoded);
}

QString MediaWikiParser::jsonMessageDescription(const QJsonObject& item) const {
  return m_articleHtml.value(pageId(item));
}

QString MediaWikiParser::jsonMessageAuthor(const QJsonObject& item) const {
  Q_UNUSED(item)
  return {};
}

QDateTime MediaWikiParser::jsonMessageDateCreated(const QJsonObject& item) {
  return QDateTime::fromString(item.value(QSL("timestamp")).toString(), Qt::ISODate).toUTC();
}

QString MediaWikiParser::jsonMessageId(const QJsonObject& item) const {
  return QString::number(pageId(item));
}

QString MediaWikiParser::jsonMessageRawContents(const QJsonObject& item) const {
  return QString::fromUtf8(QJsonDocument(item).toJson(QJsonDocument::Compact));
}
