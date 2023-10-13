// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/standardfeed.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/feedrecognizedbutfailedexception.h"
#include "exceptions/networkexception.h"
#include "exceptions/scriptexception.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "services/standard/gui/formstandardfeeddetails.h"
#include "services/standard/standardserviceroot.h"

#include "services/standard/parsers/atomparser.h"
#include "services/standard/parsers/jsonparser.h"
#include "services/standard/parsers/rdfparser.h"
#include "services/standard/parsers/rssparser.h"
#include "services/standard/parsers/sitemapparser.h"

#include <QCommandLineParser>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QScopedPointer>
#include <QTextCodec>
#include <QVariant>
#include <QXmlStreamReader>

StandardFeed::StandardFeed(RootItem* parent_item) : Feed(parent_item) {
  m_type = Type::Rss0X;
  m_sourceType = SourceType::Url;
  m_encoding = m_postProcessScript = QString();

  m_protection = NetworkFactory::NetworkAuthentication::NoAuthentication;
  m_username = QString();
  m_password = QString();
}

StandardFeed::StandardFeed(const StandardFeed& other) : Feed(other) {
  m_type = other.type();
  m_postProcessScript = other.postProcessScript();
  m_sourceType = other.sourceType();
  m_encoding = other.encoding();
  m_protection = other.protection();
  m_username = other.username();
  m_password = other.password();
}

QList<QAction*> StandardFeed::contextMenuFeedsList() {
  return serviceRoot()->getContextMenuForFeed(this);
}

QString StandardFeed::additionalTooltip() const {
  return Feed::additionalTooltip() + tr("\nEncoding: %1\n"
                                        "Type: %2\n"
                                        "Post-processing script: %3")
                                       .arg(encoding(),
                                            StandardFeed::typeToString(type()),
                                            m_postProcessScript.isEmpty() ? QSL("-") : m_postProcessScript);
}

bool StandardFeed::canBeDeleted() const {
  return true;
}

StandardServiceRoot* StandardFeed::serviceRoot() const {
  return qobject_cast<StandardServiceRoot*>(getParentServiceRoot());
}

bool StandardFeed::editViaGui() {
  QScopedPointer<FormStandardFeedDetails> form_pointer(new FormStandardFeedDetails(serviceRoot(),
                                                                                   nullptr,
                                                                                   {},
                                                                                   qApp->mainFormWidget()));

  form_pointer->addEditFeed(this);
  return false;
}

bool StandardFeed::deleteViaGui() {
  if (removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
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

    case Type::SitemapIndex:
      return QSL("Sitemap Index");

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
    StandardFeed* metadata = guessFeed(sourceType(),
                                       source(),
                                       postProcessScript(),
                                       protection(),
                                       true,
                                       username(),
                                       password(),
                                       getParentServiceRoot()->networkProxy());

    // Copy metadata to our object.
    setTitle(metadata->title());
    setDescription(metadata->description());
    setType(metadata->type());
    setEncoding(metadata->encoding());
    setIcon(metadata->icon());

    metadata->deleteLater();

    QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

    DatabaseQueries::createOverwriteFeed(database, this, getParentServiceRoot()->accountId(), parent()->id());
    serviceRoot()->itemChanged({this});
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB << "Cannot overwrite feed:" << QUOTE_W_SPACE_DOT(ex.message());
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot save feed data"),
                          tr("Cannot save data for feed: %1").arg(ex.message()),
                          QSystemTrayIcon::MessageIcon::Critical});
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

StandardFeed* StandardFeed::guessFeed(StandardFeed::SourceType source_type,
                                      const QString& source,
                                      const QString& post_process_script,
                                      NetworkFactory::NetworkFactory::NetworkAuthentication protection,
                                      bool fetch_icons,
                                      const QString& username,
                                      const QString& password,
                                      const QNetworkProxy& custom_proxy) {
  auto timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray feed_contents;
  QString content_type;

  if (source_type == StandardFeed::SourceType::Url) {
    QList<QPair<QByteArray, QByteArray>> headers = {
      NetworkFactory::generateBasicAuthHeader(protection, username, password)};
    NetworkResult network_result =
      NetworkFactory::performNetworkOperation(source,
                                              timeout,
                                              QByteArray(),
                                              feed_contents,
                                              QNetworkAccessManager::Operation::GetOperation,
                                              headers,
                                              false,
                                              {},
                                              {},
                                              custom_proxy);

    content_type = network_result.m_contentType;

    if (network_result.m_networkError != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(network_result.m_networkError);
    }
  }
  else {
    qDebugNN << LOGSEC_CORE << "Running custom script for guessing" << QUOTE_W_SPACE(source) << "to obtain feed data.";

    // Use script to generate feed file.
    feed_contents = generateFeedFileWithScript(source, timeout).toUtf8();
  }

  if (!post_process_script.simplified().isEmpty()) {
    qDebugNN << LOGSEC_CORE << "Post-processing obtained feed data with custom script for guessing"
             << QUOTE_W_SPACE_DOT(post_process_script);
    feed_contents = postProcessFeedFileWithScript(post_process_script, feed_contents, timeout).toUtf8();
  }

  StandardFeed* feed = nullptr;
  QList<IconLocation> icon_possible_locations;
  QList<QSharedPointer<FeedParser>> parsers;

  parsers.append(QSharedPointer<FeedParser>(new AtomParser({})));
  parsers.append(QSharedPointer<FeedParser>(new RssParser({})));
  parsers.append(QSharedPointer<FeedParser>(new RdfParser({})));
  parsers.append(QSharedPointer<FeedParser>(new JsonParser({})));
  parsers.append(QSharedPointer<FeedParser>(new SitemapParser({})));

  for (const QSharedPointer<FeedParser>& parser : parsers) {
    try {
      QPair<StandardFeed*, QList<IconLocation>> res = parser->guessFeed(feed_contents, content_type);

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
      qWarningNN << LOGSEC_CORE << "Feed guessing error:" << QUOTE_W_SPACE_DOT(ex.message());
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

    if (NetworkFactory::downloadIcon(icon_possible_locations, DOWNLOAD_TIMEOUT, icon_data, {}, custom_proxy) ==
        QNetworkReply::NetworkError::NoError) {
      // Icon for feed was downloaded and is stored now in icon_data.
      feed->setIcon(icon_data);
    }
  }

  return feed;
}

Qt::ItemFlags StandardFeed::additionalFlags() const {
  return Feed::additionalFlags() | Qt::ItemFlag::ItemIsDragEnabled;
}

bool StandardFeed::performDragDropChange(RootItem* target_item) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  try {
    DatabaseQueries::createOverwriteFeed(database, this, getParentServiceRoot()->accountId(), target_item->id());
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

bool StandardFeed::removeItself() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, this, getParentServiceRoot()->accountId());
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

  return qApp->replaceDataUserDataFolderPlaceholder(args);
}

QString StandardFeed::runScriptProcess(const QStringList& cmd_args,
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

  if (!process.open()) {
    switch (process.error()) {
      case QProcess::ProcessError::FailedToStart:
        throw ScriptException(ScriptException::Reason::InterpreterNotFound);

      default:
        break;
    }
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
      qWarningNN << LOGSEC_CORE
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

QString StandardFeed::generateFeedFileWithScript(const QString& execution_line, int run_timeout) {
  auto prepared_query = prepareExecutionLine(execution_line);

  if (prepared_query.isEmpty()) {
    throw ScriptException(ScriptException::Reason::ExecutionLineInvalid);
  }

  return runScriptProcess(prepared_query, qApp->userDataFolder(), run_timeout, false);
}

QString StandardFeed::postProcessFeedFileWithScript(const QString& execution_line,
                                                    const QString& raw_feed_data,
                                                    int run_timeout) {
  auto prepared_query = prepareExecutionLine(execution_line);

  if (prepared_query.isEmpty()) {
    throw ScriptException(ScriptException::Reason::ExecutionLineInvalid);
  }

  return runScriptProcess(prepared_query, qApp->userDataFolder(), run_timeout, true, raw_feed_data);
}
