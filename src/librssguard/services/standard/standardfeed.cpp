// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/standardfeed.h"

#include "3rd-party/sc/simplecrypt.h"
#include "core/feedsmodel.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/networkexception.h"
#include "exceptions/scriptexception.h"
#include "exceptions/scriptexception.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/recyclebin.h"
#include "services/standard/atomparser.h"
#include "services/standard/definitions.h"
#include "services/standard/gui/formstandardfeeddetails.h"
#include "services/standard/jsonparser.h"
#include "services/standard/rdfparser.h"
#include "services/standard/rssparser.h"
#include "services/standard/standardserviceroot.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTextCodec>
#include <QVariant>
#include <QXmlStreamReader>

StandardFeed::StandardFeed(RootItem* parent_item)
  : Feed(parent_item) {
  m_type = Type::Rss0X;
  m_sourceType = SourceType::Url;
  m_encoding = m_postProcessScript = QString();

  m_passwordProtected = false;
  m_username = QString();
  m_password = QString();
}

StandardFeed::StandardFeed(const StandardFeed& other)
  : Feed(other) {
  m_type = other.type();
  m_postProcessScript = other.postProcessScript();
  m_sourceType = other.sourceType();
  m_encoding = other.encoding();
  m_passwordProtected = other.passwordProtected();
  m_username = other.username();
  m_password = other.password();
}

QList<QAction*> StandardFeed::contextMenuFeedsList() {
  return serviceRoot()->getContextMenuForFeed(this);
}

QString StandardFeed::additionalTooltip() const {
  return Feed::additionalTooltip() + tr("\nEncoding: %2\n"
                                        "Type: %3").arg(encoding(), StandardFeed::typeToString(type()));
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

bool StandardFeed::passwordProtected() const {
  return m_passwordProtected;
}

void StandardFeed::setPasswordProtected(bool passwordProtected) {
  m_passwordProtected = passwordProtected;
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

  data["source_type"] = int(sourceType());
  data["type"] = int(type());
  data["encoding"] = encoding();
  data["post_process"] = postProcessScript();
  data["protected"] = passwordProtected();
  data["username"] = username();
  data["password"] = TextFactory::encrypt(password());

  return data;
}

void StandardFeed::setCustomDatabaseData(const QVariantHash& data) {
  setSourceType(SourceType(data["source_type"].toInt()));
  setType(Type(data["type"].toInt()));
  setEncoding(data["encoding"].toString());
  setPostProcessScript(data["post_process"].toString());
  setPasswordProtected(data["protected"].toBool());
  setUsername(data["username"].toString());
  setPassword(TextFactory::decrypt(data["password"].toString()));
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
    serviceRoot()->itemChanged({ this });
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB
                << "Cannot overwrite feed:"
                << QUOTE_W_SPACE_DOT(ex.message());
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         tr("Error"),
                         tr("Cannot save data for feed: %1").arg(ex.message()),
                         QSystemTrayIcon::MessageIcon::Critical,
                         true);
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
                                      const QString& username,
                                      const QString& password,
                                      const QNetworkProxy& custom_proxy) {
  auto timeout = qApp->settings()->value(GROUP(Feeds),
                                         SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray feed_contents;
  QString content_type;

  if (source_type == StandardFeed::SourceType::Url) {
    QList<QPair<QByteArray, QByteArray>> headers = { NetworkFactory::generateBasicAuthHeader(username, password) };
    NetworkResult network_result = NetworkFactory::performNetworkOperation(source,
                                                                           timeout,
                                                                           QByteArray(),
                                                                           feed_contents,
                                                                           QNetworkAccessManager::Operation::GetOperation,
                                                                           headers,
                                                                           false,
                                                                           {},
                                                                           {},
                                                                           custom_proxy);

    content_type = network_result.second.toString();

    if (network_result.first != QNetworkReply::NetworkError::NoError) {
      throw NetworkException(network_result.first);
    }
  }
  else {
    qDebugNN << LOGSEC_CORE
             << "Running custom script for guessing"
             << QUOTE_W_SPACE(source)
             << "to obtain feed data.";

    // Use script to generate feed file.
    feed_contents = generateFeedFileWithScript(source, timeout).toUtf8();
  }

  if (!post_process_script.simplified().isEmpty()) {
    qDebugNN << LOGSEC_CORE
             << "Post-processing obtained feed data with custom script for guessing"
             << QUOTE_W_SPACE_DOT(post_process_script);
    feed_contents = postProcessFeedFileWithScript(post_process_script, feed_contents, timeout).toUtf8();
  }

  StandardFeed* feed = nullptr;

  // Now we need to obtain list of URLs of icons.
  // Priority of links:
  //   1. Links of "homepage" obtained from feed files which will be processed via DuckDuckGo.
  //   2. Direct links of "favicon", "icon", "logo" obtained from feed files which will be downloaded directly.
  //   3. Link of the feed file itself which will be processed via DuckDuckGo.
  // The "bool" if true means that the URL is direct and download directly, if false then
  // only use its domain and download via DuckDuckGo.
  QList<QPair<QString, bool>> icon_possible_locations;

  if (content_type.contains(QSL("json"), Qt::CaseSensitivity::CaseInsensitive) || feed_contents.startsWith('{')) {
    feed = new StandardFeed();

    // We have JSON feed.
    feed->setEncoding(DEFAULT_FEED_ENCODING);
    feed->setType(Type::Json);

    QJsonDocument json = QJsonDocument::fromJson(feed_contents);

    feed->setTitle(json.object()["title"].toString());
    feed->setDescription(json.object()["description"].toString());

    auto home_page = json.object()["home_page_url"].toString();

    if (!home_page.isEmpty()) {
      icon_possible_locations.prepend({ home_page, false });
    }

    auto icon = json.object()["favicon"].toString();

    if (icon.isEmpty()) {
      icon = json.object()["icon"].toString();
    }

    if (!icon.isEmpty()) {
      // Low priority, download directly.
      icon_possible_locations.append({ icon, true });
    }
  }
  else {
    // Feed XML was obtained, now we need to try to guess
    // its encoding before we can read further data.
    QString xml_schema_encoding;
    QString xml_contents_encoded;
    QString enc = QRegularExpression(QSL("encoding=\"([A-Z0-9\\-]+)\""),
                                     QRegularExpression::PatternOption::CaseInsensitiveOption).match(feed_contents).captured(1);

    if (!enc.isEmpty()) {
      // Some "encoding" attribute was found get the encoding
      // out of it.
      xml_schema_encoding = enc;
    }

    QTextCodec* custom_codec = QTextCodec::codecForName(xml_schema_encoding.toLocal8Bit());
    QString encod;

    if (custom_codec != nullptr) {
      // Feed encoding was probably guessed.
      xml_contents_encoded = custom_codec->toUnicode(feed_contents);
      encod = xml_schema_encoding;
    }
    else {
      // Feed encoding probably not guessed, set it as
      // default.
      xml_contents_encoded = feed_contents;
      encod = DEFAULT_FEED_ENCODING;
    }

    // Feed XML was obtained, guess it now.
    QDomDocument xml_document;
    QString error_msg;
    int error_line, error_column;

    if (!xml_document.setContent(xml_contents_encoded,
                                 true,
                                 &error_msg,
                                 &error_line,
                                 &error_column)) {
      throw ApplicationException(tr("XML is not well-formed, %1").arg(error_msg));
    }

    feed = new StandardFeed();
    feed->setEncoding(encod);

    QDomElement root_element = xml_document.documentElement();
    RdfParser rdf(QSL("<a/>"));
    AtomParser atom(QSL("<a/>"));

    if (root_element.namespaceURI() == rdf.rdfNamespace()) {
      // We found RDF feed.
      QDomElement channel_element = root_element.elementsByTagNameNS(rdf.rssNamespace(), QSL("channel")).at(0).toElement();

      feed->setType(Type::Rdf);
      feed->setTitle(channel_element.elementsByTagNameNS(rdf.rssNamespace(), QSL("title")).at(0).toElement().text());
      feed->setDescription(channel_element.elementsByTagNameNS(rdf.rssNamespace(), QSL("description")).at(0).toElement().text());

      QString home_page = channel_element.elementsByTagNameNS(rdf.rssNamespace(), QSL("link")).at(0).toElement().text();

      if (!home_page.isEmpty()) {
        icon_possible_locations.prepend({ home_page, false });
      }
    }
    else if (root_element.tagName() == QL1S("rss")) {
      // We found RSS 0.91/0.92/0.93/2.0/2.0.1 feed.
      QString rss_type = root_element.attribute("version", "2.0");

      if (rss_type == QL1S("0.91") || rss_type == QL1S("0.92") || rss_type == QL1S("0.93")) {
        feed->setType(Type::Rss0X);
      }
      else {
        feed->setType(Type::Rss2X);
      }

      QDomElement channel_element = root_element.namedItem(QSL("channel")).toElement();

      feed->setTitle(channel_element.namedItem(QSL("title")).toElement().text());
      feed->setDescription(channel_element.namedItem(QSL("description")).toElement().text());

      QString icon_link = channel_element.namedItem(QSL("image")).toElement().text();
      QString icon_url_link = channel_element.namedItem(QSL("image")).toElement().attribute(QSL("url"));

      if (!icon_url_link.isEmpty()) {
        icon_possible_locations.append({ icon_url_link, true });
      }
      else if (!icon_link.isEmpty()) {
        icon_possible_locations.append({ icon_link, true });
      }

      QString home_page = channel_element.namedItem(QSL("link")).toElement().text();

      if (!home_page.isEmpty()) {
        icon_possible_locations.prepend({ home_page, false });
      }
    }
    else if (root_element.namespaceURI() == atom.atomNamespace()) {
      // We found ATOM feed.
      feed->setType(Type::Atom10);
      feed->setTitle(root_element.namedItem(QSL("title")).toElement().text());
      feed->setDescription(root_element.namedItem(QSL("subtitle")).toElement().text());

      QString icon_link = root_element.namedItem(QSL("icon")).toElement().text();

      if (!icon_link.isEmpty()) {
        icon_possible_locations.append({ icon_link, true });
      }

      QString home_page = root_element.namedItem(QSL("link")).toElement().attribute(QSL("href"));

      if (!home_page.isEmpty()) {
        icon_possible_locations.prepend({ home_page, false });
      }
    }
    else {
      // File was downloaded and it really was XML file
      // but feed format was NOT recognized.
      feed->deleteLater();
      throw ApplicationException(tr("XML feed file format unrecognized"));
    }
  }

  if (source_type == SourceType::Url && icon_possible_locations.isEmpty()) {
    // We have no source for feed icon, we use the URL of the feed file itself.
    icon_possible_locations.append({ source, false });
  }

  // Try to obtain icon.
  QIcon icon_data;

  if (NetworkFactory::downloadIcon(icon_possible_locations,
                                   DOWNLOAD_TIMEOUT,
                                   icon_data,
                                   custom_proxy) == QNetworkReply::NetworkError::NoError) {
    // Icon for feed was downloaded and is stored now in _icon_data.
    feed->setIcon(icon_data);
  }

  return feed;
}

Qt::ItemFlags StandardFeed::additionalFlags() const {
  return Qt::ItemFlag::ItemIsDragEnabled;
}

bool StandardFeed::performDragDropChange(RootItem* target_item) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  try {
    DatabaseQueries::createOverwriteFeed(database, this, getParentServiceRoot()->accountId(), target_item->id());
    serviceRoot()->requestItemReassignment(this, target_item);
    return true;
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_DB
                << "Cannot overwrite feed:"
                << QUOTE_W_SPACE_DOT(ex.message());
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         tr("Error"),
                         tr("Cannot move feed, detailed information was logged via debug log."),
                         QSystemTrayIcon::MessageIcon::Critical,
                         true);
    return false;
  }
}

bool StandardFeed::removeItself() {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, customId().toInt(), getParentServiceRoot()->accountId());
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
  auto split_exec = execution_line.split(EXECUTION_LINE_SEPARATOR,
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                         Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                         QString::SplitBehavior::SkipEmptyParts);
#endif

  return qApp->replaceDataUserDataFolderPlaceholder(split_exec);
}

QString StandardFeed::runScriptProcess(const QStringList& cmd_args, const QString& working_directory,
                                       int run_timeout, bool provide_input, const QString& input) {
  QProcess process;

  if (provide_input) {
    process.setInputChannelMode(QProcess::InputChannelMode::ManagedInputChannel);
  }

  process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
  process.setProcessChannelMode(QProcess::ProcessChannelMode::SeparateChannels);
  process.setWorkingDirectory(working_directory);
  process.setProgram(cmd_args.at(0));
  process.setArguments(cmd_args.mid(1));

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

  if (process.waitForFinished(run_timeout) &&
      process.exitStatus() == QProcess::ExitStatus::NormalExit &&
      process.exitCode() == EXIT_SUCCESS) {
    auto raw_output = process.readAllStandardOutput();
    auto raw_error = process.readAllStandardError();

    if (!raw_error.simplified().isEmpty()) {
      qWarningNN << LOGSEC_CORE
                 << "Received error output from custom script even if it reported that it exited normally:"
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

  return runScriptProcess(prepared_query, qApp->userDataFolder(), run_timeout, false);
}

QString StandardFeed::postProcessFeedFileWithScript(const QString& execution_line,
                                                    const QString raw_feed_data,
                                                    int run_timeout) {
  auto prepared_query = prepareExecutionLine(execution_line);

  return runScriptProcess(prepared_query, qApp->userDataFolder(), run_timeout, true, raw_feed_data);
}
