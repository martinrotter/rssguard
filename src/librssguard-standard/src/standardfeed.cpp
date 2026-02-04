// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/standardfeed.h"

#include "src/definitions.h"
#include "src/gui/formstandardfeeddetails.h"
#include "src/parsers/atomparser.h"
#include "src/parsers/icalparser.h"
#include "src/parsers/jsonparser.h"
#include "src/parsers/rdfparser.h"
#include "src/parsers/rssparser.h"
#include "src/parsers/sitemapparser.h"
#include "src/standardserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/exceptions/applicationexception.h>
#include <librssguard/exceptions/feedrecognizedbutfailedexception.h>
#include <librssguard/exceptions/networkexception.h>
#include <librssguard/exceptions/scriptexception.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/textfactory.h>

#if defined(ENABLE_COMPRESSED_SITEMAP)
#include "src/3rd-party/qcompressor/qcompressor.h"
#endif

#include <QCommandLineParser>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopedPointer>
#include <QStringLiteral>
#include <QVariant>
#include <QXmlStreamReader>

StandardFeed::StandardFeed(RootItem* parent_item) : Feed(parent_item) {
  m_type = Type::Rss0X;
  m_sourceType = SourceType::Url;
  m_encoding = m_postProcessScript = QString();

  m_protection = NetworkFactory::NetworkAuthentication::NoAuthentication;
  m_username = QString();
  m_password = QString();
  m_httpHeaders = {};
  m_dontUseRawXmlSaving = false;
  m_http2Status = NetworkFactory::Http2Status::DontSet;
  m_fetchCommentsEnabled = false;
  m_useAccountProxy = true;
  m_fetchFullArticles = false;
  m_fetchFullArticlesInPlainText = false;
}

StandardFeed::StandardFeed(const StandardFeed& other) : Feed(other) {
  m_type = other.type();
  m_postProcessScript = other.postProcessScript();
  m_sourceType = other.sourceType();
  m_encoding = other.encoding();
  m_protection = other.protection();
  m_username = other.username();
  m_password = other.password();
  m_dontUseRawXmlSaving = other.dontUseRawXmlSaving();
  m_fetchCommentsEnabled = other.fetchCommentsEnabled();
  m_httpHeaders = other.httpHeaders();
  m_http2Status = other.http2Status();
  m_proxy = other.networkProxy();
  m_useAccountProxy = other.useAccountProxy();
  m_fetchFullArticles = other.fetchFullArticles();
  m_fetchFullArticlesInPlainText = other.fetchFullArticlesInPlainText();
}

QString StandardFeed::additionalTooltip() const {
  QString base_tooltip = Feed::additionalTooltip();

  return base_tooltip + QSL("\n") +
         tr("Encoding: %1\n"
            "Type: %2\n"
            "Post-processing script: %3\n"
            "Use raw XML saving: %4\n"
            "Fetch article comments: %5\n"
            "HTTP/2: %6\n"
            "Fetch full articles: %7 (plain text only: %8)\n")
           .arg(encoding(),
                StandardFeed::typeToString(type()),
                m_postProcessScript.isEmpty() ? QSL("-") : m_postProcessScript,
                !dontUseRawXmlSaving() ? tr("yes") : tr("no"),
                fetchCommentsEnabled() ? tr("yes") : tr("no"),
                getHttpDescription(),
                fetchFullArticles() ? tr("yes") : tr("no"),
                fetchFullArticlesInPlainText() ? tr("yes") : tr("no"));
}

NetworkFactory::Http2Status StandardFeed::http2Status() const {
  return m_http2Status;
}

void StandardFeed::setHttp2Status(NetworkFactory::Http2Status status) {
  m_http2Status = status;
}

bool StandardFeed::canBeDeleted() const {
  return true;
}

StandardServiceRoot* StandardFeed::serviceRoot() const {
  return qobject_cast<StandardServiceRoot*>(account());
}

void StandardFeed::deleteItem() {
  removeItself();
  serviceRoot()->requestItemRemoval(this);
}

NetworkFactory::NetworkAuthentication StandardFeed::protection() const {
  return m_protection;
}

void StandardFeed::setProtection(NetworkFactory::NetworkAuthentication protect) {
  m_protection = protect;
}

QString StandardFeed::username() const {
  return m_username;
}

void StandardFeed::setUsername(const QString& username) {
  m_username = username;
}

QString StandardFeed::password() const {
  return m_password;
}

void StandardFeed::setPassword(const QString& password) {
  m_password = password;
}

QVariantHash StandardFeed::customDatabaseData() const {
  QVariantHash data;

  data[QSL("source_type")] = int(sourceType());
  data[QSL("type")] = int(type());
  data[QSL("encoding")] = encoding();
  data[QSL("post_process")] = postProcessScript();
  data[QSL("protected")] = int(protection());
  data[QSL("username")] = username();
  data[QSL("password")] = TextFactory::encrypt(password());
  data[QSL("fetch_comments")] = fetchCommentsEnabled();
  data[QSL("dont_use_raw_xml_saving")] = dontUseRawXmlSaving();
  data[QSL("http_headers")] = httpHeaders();
  data[QSL("http2_status")] = int(http2Status());
  data[QSL("use_account_proxy")] = useAccountProxy();

  data[QSL("fetch_full_articles")] = fetchFullArticles();
  data[QSL("fetch_full_articles_plain_text")] = fetchFullArticlesInPlainText();

  data[QSL("proxy_type")] = int(networkProxy().type());
  data[QSL("proxy_host")] = networkProxy().hostName();
  data[QSL("proxy_port")] = networkProxy().port();
  data[QSL("proxy_username")] = networkProxy().user();
  data[QSL("proxy_password")] = TextFactory::encrypt(networkProxy().password());

  return data;
}

void StandardFeed::setCustomDatabaseData(const QVariantHash& data) {
  setSourceType(SourceType(data[QSL("source_type")].toInt()));
  setType(Type(data[QSL("type")].toInt()));
  setEncoding(data[QSL("encoding")].toString());
  setPostProcessScript(data[QSL("post_process")].toString());
  setProtection(NetworkFactory::NetworkAuthentication(data[QSL("protected")].toInt()));
  setUsername(data[QSL("username")].toString());
  setPassword(TextFactory::decrypt(data[QSL("password")].toString()));
  setFetchCommentsEnabled(data[QSL("fetch_comments")].toBool());
  setDontUseRawXmlSaving(data[QSL("dont_use_raw_xml_saving")].toBool());
  setHttpHeaders(data[QSL("http_headers")].toHash());
  setHttp2Status(NetworkFactory::Http2Status(data[QSL("http2_status")].toInt()));

  setFetchFullArticles(data[QSL("fetch_full_articles")].toBool());
  setFetchFullArticlesInPlainText(data[QSL("fetch_full_articles_plain_text")].toBool());

  setUseAccountProxy(data[QSL("use_account_proxy")].toBool());

  QNetworkProxy proxy(static_cast<QNetworkProxy::ProxyType>(data[QSL("proxy_type")].toInt()),
                      data[QSL("proxy_host")].toString(),
                      data[QSL("proxy_port")].toInt(),
                      data[QSL("proxy_username")].toString(),
                      TextFactory::decrypt(data[QSL("proxy_password")].toString()));

  setNetworkProxy(proxy);
}

QString StandardFeed::typeToString(StandardFeed::Type type) {
  switch (type) {
    case Type::Atom10:
      return QSL("ATOM 1.0");

    case Type::Rdf:
      return QSL("RDF (RSS 1.0)");

    case Type::Rss0X:
      return QSL("RSS 0.91/0.92/0.93");

    case Type::Json:
      return QSL("JSON 1.0/1.1");

    case Type::Sitemap:
      return QSL("Sitemap");

    case Type::iCalendar:
      return QSL("iCalendar");

    case Type::Rss2X:
    default:
      return QSL("RSS 2.0/2.0.1");
  }
}

QString StandardFeed::sourceTypeToString(StandardFeed::SourceType type) {
  switch (type) {
    case StandardFeed::SourceType::Url:
      return QSL("URL");

    case StandardFeed::SourceType::Script:
      return tr("Script");

    case StandardFeed::SourceType::LocalFile:
      return tr("Local file");

    default:
      return tr("Unknown");
  }
}

void StandardFeed::fetchMetadataForItself() {
  try {
    auto metadata = guessFeed(sourceType(),
                              source(),
                              postProcessScript(),
                              serviceRoot(),
                              protection(),
                              true,
                              username(),
                              password(),
                              {},
                              useAccountProxy() ? account()->networkProxy() : networkProxy());

    // Copy metadata to our object.
    setTitle(metadata.first->title());
    setDescription(metadata.first->description());
    setType(metadata.first->type());
    setEncoding(metadata.first->encoding());
    setIcon(metadata.first->icon());

    if (metadata.second.m_url.isValid()) {
      setSource(metadata.second.m_url.toString());
    }

    metadata.first->deleteLater();

    QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());

    DatabaseQueries::createOverwriteFeed(database, this, account()->accountId(), parent()->id());
  }
  catch (const ApplicationException& ex) {
    setStatus(Feed::Status::OtherError, ex.message());
    qCriticalNN << LOGSEC_DB << "Cannot save feed after metadata fetch:" << QUOTE_W_SPACE_DOT(ex.message());
  }
}

QString StandardFeed::postProcessScript() const {
  return m_postProcessScript;
}

void StandardFeed::setPostProcessScript(const QString& post_process_script) {
  m_postProcessScript = post_process_script;
}

StandardFeed::SourceType StandardFeed::sourceType() const {
  return m_sourceType;
}

void StandardFeed::setSourceType(SourceType source_type) {
  m_sourceType = source_type;
}

QPair<StandardFeed*, NetworkResult> StandardFeed::guessFeed(StandardFeed::SourceType source_type,
                                                            const QString& source,
                                                            const QString& post_process_script,
                                                            StandardServiceRoot* account,
                                                            NetworkFactory::NetworkFactory::NetworkAuthentication
                                                              protection,
                                                            bool fetch_icons,
                                                            const QString& username,
                                                            const QString& password,
                                                            const QList<QPair<QByteArray, QByteArray>>& http_headers,
                                                            const QNetworkProxy& custom_proxy,
                                                            NetworkFactory::Http2Status http2_status) {
  auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray feed_contents;
  NetworkResult network_result;

  if (source_type == StandardFeed::SourceType::Url) {
    QString host = QUrl(source).host();
    account->spaceHost(host, source);

    QList<QPair<QByteArray, QByteArray>> headers = http_headers;
    headers << NetworkFactory::generateBasicAuthHeader(protection, username, password);

    network_result = NetworkFactory::performNetworkOperation(source,
                                                             timeout,
                                                             QByteArray(),
                                                             feed_contents,
                                                             QNetworkAccessManager::Operation::GetOperation,
                                                             headers,
                                                             false,
                                                             {},
                                                             {},
                                                             custom_proxy,
                                                             http2_status);

    // account->resetHostSpacing(host);

    if (network_result.m_networkError != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(network_result.m_networkError);
    }
  }
  else if (source_type == StandardFeed::SourceType::LocalFile) {
    feed_contents = IOFactory::readFile(source);
  }
  else {
    qDebugNN << LOGSEC_STANDARD << "Running custom script for guessing" << QUOTE_W_SPACE(source)
             << "to obtain feed data.";

    // Use script to generate feed file.
    feed_contents = generateFeedFileWithScript(source, timeout);
  }

  // Sitemap parser supports gzip-encoded data too.
  // We need to decode it here before encoding
  // stuff kicks in.
  if (SitemapParser::isGzip(feed_contents)) {
#if defined(ENABLE_COMPRESSED_SITEMAP)
    qWarningNN << LOGSEC_STANDARD << "Decompressing gzipped feed data.";

    QByteArray uncompressed_feed_contents;

    if (!QCompressor::gzipDecompress(feed_contents, uncompressed_feed_contents)) {
      throw ApplicationException("gzip decompression failed");
    }

    feed_contents = uncompressed_feed_contents;
#else
    qWarningNN << LOGSEC_STANDARD << "This feed is gzipped.";
#endif
  }

  if (!post_process_script.simplified().isEmpty()) {
    qDebugNN << LOGSEC_STANDARD << "Post-processing obtained feed data with custom script for guessing"
             << QUOTE_W_SPACE_DOT(post_process_script);
    feed_contents = postProcessFeedFileWithScript(post_process_script, feed_contents, timeout);
  }

  StandardFeed* feed = nullptr;
  QList<IconLocation> icon_possible_locations;
  QList<QSharedPointer<FeedParser>> parsers;

  parsers.append(QSharedPointer<FeedParser>(new AtomParser({})));
  parsers.append(QSharedPointer<FeedParser>(new RssParser({})));
  parsers.append(QSharedPointer<FeedParser>(new RdfParser({})));
  parsers.append(QSharedPointer<FeedParser>(new IcalParser({})));
  parsers.append(QSharedPointer<FeedParser>(new JsonParser({})));
  parsers.append(QSharedPointer<FeedParser>(new SitemapParser({})));

  for (const QSharedPointer<FeedParser>& parser : parsers) {
    try {
      QPair<StandardFeed*, QList<IconLocation>> res = parser->guessFeed(feed_contents, network_result);

      feed = res.first;
      icon_possible_locations = res.second;
      break;
    }
    catch (const FeedRecognizedButFailedException& format_ex) {
      // Parser reports that it is right parser for this feed
      // but its support is not enabled or available or it is broken.
      // In this case abort.
      throw format_ex;
    }
    catch (const ApplicationException& ex) {
      qWarningNN << LOGSEC_STANDARD << "Feed guessing error:" << QUOTE_W_SPACE_DOT(ex.message());
    }
  }

  if (feed == nullptr) {
    throw ApplicationException(tr("feed format not recognized"));
  }

  if (source_type == SourceType::Url && icon_possible_locations.isEmpty()) {
    // We have no source for feed icon, we use the URL of the feed file itself.
    icon_possible_locations.append({source, false});
  }

  if (fetch_icons) {
    // Try to obtain icon.
    QPixmap icon_data;

    if (NetworkFactory::downloadIcon(icon_possible_locations,
                                     DOWNLOAD_TIMEOUT,
                                     icon_data,
                                     http_headers,
                                     custom_proxy) == QNetworkReply::NetworkError::NoError) {
      // Icon for feed was downloaded and is stored now in icon_data.
      feed->setIcon(icon_data);
    }
  }

  return {feed, network_result};
}

Qt::ItemFlags StandardFeed::additionalFlags() const {
  return Feed::additionalFlags() | Qt::ItemFlag::ItemIsDragEnabled;
}

bool StandardFeed::performDragDropChange(RootItem* target_item) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  try {
    DatabaseQueries::createOverwriteFeed(database, this, account()->accountId(), target_item->id());
    serviceRoot()->requestItemReassignment(this, target_item);
    return true;
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB << "Cannot overwrite feed:" << QUOTE_W_SPACE_DOT(ex.message());

    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot move feed"),
                          tr("Cannot move feed, detailed "
                             "information was logged via "
                             "debug log."),
                          QSystemTrayIcon::MessageIcon::Critical});
    return false;
  }
}

void StandardFeed::removeItself() {
  DatabaseQueries::deleteFeed(qApp->database()->driver()->connection(metaObject()->className()),
                              this,
                              account()->accountId());
}

QString StandardFeed::getHttpDescription() const {
  switch (m_http2Status) {
    case NetworkFactory::Http2Status::DontSet:
      return tr("uses application setting");

    case NetworkFactory::Http2Status::Enabled:
      return tr("enabled");

    case NetworkFactory::Http2Status::Disabled:
      return tr("disabled");

    default:
      return tr("unknown state");
  }
}

bool StandardFeed::fetchFullArticlesInPlainText() const {
  return m_fetchFullArticlesInPlainText;
}

void StandardFeed::setFetchFullArticlesInPlainText(bool fetch_full_articles_in_plain_text) {
  m_fetchFullArticlesInPlainText = fetch_full_articles_in_plain_text;
}

bool StandardFeed::fetchFullArticles() const {
  return m_fetchFullArticles;
}

void StandardFeed::setFetchFullArticles(bool fetch_full_articles) {
  m_fetchFullArticles = fetch_full_articles;
}

QNetworkProxy StandardFeed::networkProxy() const {
  return m_proxy;
}

void StandardFeed::setNetworkProxy(const QNetworkProxy& new_proxy) {
  m_proxy = new_proxy;
}

bool StandardFeed::useAccountProxy() const {
  return m_useAccountProxy;
}

void StandardFeed::setUseAccountProxy(bool use) {
  m_useAccountProxy = use;
}

bool StandardFeed::fetchCommentsEnabled() const {
  return m_fetchCommentsEnabled;
}

void StandardFeed::setFetchCommentsEnabled(bool enabled) {
  m_fetchCommentsEnabled = enabled;
}

QVariantHash StandardFeed::httpHeaders() const {
  return m_httpHeaders;
}

void StandardFeed::setHttpHeaders(const QVariantHash& http_headers) {
  m_httpHeaders = http_headers;
}

bool StandardFeed::dontUseRawXmlSaving() const {
  return m_dontUseRawXmlSaving;
}

void StandardFeed::setDontUseRawXmlSaving(bool no_raw_xml_saving) {
  m_dontUseRawXmlSaving = no_raw_xml_saving;
}

QString StandardFeed::dateTimeFormat() const {
  return m_dateTimeFormat;
}

void StandardFeed::setDateTimeFormat(const QString& dt_format) {
  m_dateTimeFormat = dt_format;
}

QString StandardFeed::lastEtag() const {
  return m_lastEtag;
}

void StandardFeed::setLastEtag(const QString& etag) {
  m_lastEtag = etag;
}

StandardFeed::Type StandardFeed::type() const {
  return m_type;
}

void StandardFeed::setType(StandardFeed::Type type) {
  m_type = type;
}

QString StandardFeed::encoding() const {
  return m_encoding;
}

void StandardFeed::setEncoding(const QString& encoding) {
  m_encoding = encoding;
}

QStringList StandardFeed::prepareExecutionLine(const QString& execution_line) {
  auto args = TextFactory::tokenizeProcessArguments(execution_line);

  return qApp->replaceUserDataFolderPlaceholder(args);
}

QByteArray StandardFeed::runScriptProcess(const QStringList& cmd_args,
                                          const QString& working_directory,
                                          int run_timeout,
                                          bool provide_input,
                                          const QString& input) {
  QProcess process;

  if (provide_input) {
    process.setInputChannelMode(QProcess::InputChannelMode::ManagedInputChannel);
  }

  process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
  process.setProcessChannelMode(QProcess::ProcessChannelMode::SeparateChannels);
  process.setWorkingDirectory(working_directory);
  process.setProgram(cmd_args.at(0));

  if (cmd_args.size() > 1) {
    process.setArguments(cmd_args.mid(1));
  }

  process.open();

  switch (process.error()) {
    case QProcess::ProcessError::FailedToStart:
      throw ScriptException(ScriptException::Reason::InterpreterNotFound);

    default:
      break;
  }

  if (provide_input) {
    process.write(input.toUtf8());
    process.closeWriteChannel();
  }

  if (process.waitForFinished(run_timeout) && process.exitStatus() == QProcess::ExitStatus::NormalExit &&
      process.exitCode() == EXIT_SUCCESS) {
    auto raw_output = process.readAllStandardOutput();
    auto raw_error = process.readAllStandardError();

    if (!raw_error.simplified().isEmpty()) {
      qWarningNN << LOGSEC_STANDARD
                 << "Received error output from "
                    "custom script even if it "
                    "reported that it exited "
                    "normally:"
                 << QUOTE_W_SPACE_DOT(raw_error);
    }

    return raw_output;
  }
  else {
    auto raw_error = process.readAllStandardError().simplified();

    if (raw_error.isEmpty()) {
      raw_error = process.readAllStandardOutput().simplified();
    }

    switch (process.error()) {
      case QProcess::ProcessError::Timedout:
        throw ScriptException(ScriptException::Reason::InterpreterTimeout);

      default:
        throw ScriptException(ScriptException::Reason::InterpreterError, raw_error);
    }
  }
}

QList<QPair<QByteArray, QByteArray>> StandardFeed::httpHeadersToList(const QVariantHash& headers) {
  QList<QPair<QByteArray, QByteArray>> hdrs_list;

  for (auto i = headers.cbegin(), end = headers.cend(); i != end; i++) {
    hdrs_list.append({i.key().toLocal8Bit(), i.value().toString().toLocal8Bit()});
  }

  return hdrs_list;
}

QByteArray StandardFeed::generateFeedFileWithScript(const QString& execution_line, int run_timeout) {
  auto prepared_query = prepareExecutionLine(execution_line);

  if (prepared_query.isEmpty()) {
    throw ScriptException(ScriptException::Reason::ExecutionLineInvalid);
  }

  return runScriptProcess(prepared_query, qApp->userDataFolder(), run_timeout, false);
}

QByteArray StandardFeed::postProcessFeedFileWithScript(const QString& execution_line,
                                                       const QString& raw_feed_data,
                                                       int run_timeout) {
  auto prepared_query = prepareExecutionLine(execution_line);

  if (prepared_query.isEmpty()) {
    throw ScriptException(ScriptException::Reason::ExecutionLineInvalid);
  }

  return runScriptProcess(prepared_query, qApp->userDataFolder(), run_timeout, true, raw_feed_data);
}
