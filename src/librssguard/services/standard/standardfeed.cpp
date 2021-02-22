// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/standardfeed.h"

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "exceptions/scriptexception.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/simplecrypt/simplecrypt.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/recyclebin.h"
#include "services/standard/atomparser.h"
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
  m_networkError = QNetworkReply::NetworkError::NoError;
  m_type = Type::Rss0X;
  m_sourceType = SourceType::Url;
  m_encoding = m_postProcessScript = QString();
}

StandardFeed::StandardFeed(const StandardFeed& other)
  : Feed(other) {
  m_networkError = other.networkError();
  m_type = other.type();
  m_postProcessScript = other.postProcessScript();
  m_sourceType = other.sourceType();
  m_encoding = other.encoding();
}

StandardFeed::~StandardFeed() {
  qDebugNN << LOGSEC_CORE << "Destroying StandardFeed instance.";
}

QList<QAction*> StandardFeed::contextMenuFeedsList() {
  return serviceRoot()->getContextMenuForFeed(this);
}

QString StandardFeed::additionalTooltip() const {
  return Feed::additionalTooltip() + tr("\nNetwork status: %1\n"
                                        "Encoding: %2\n"
                                        "Type: %3").arg(NetworkFactory::networkErrorText(m_networkError),
                                                        encoding(),
                                                        StandardFeed::typeToString(type()));
}

bool StandardFeed::canBeEdited() const {
  return true;
}

bool StandardFeed::canBeDeleted() const {
  return true;
}

StandardServiceRoot* StandardFeed::serviceRoot() const {
  return qobject_cast<StandardServiceRoot*>(getParentServiceRoot());
}

bool StandardFeed::editViaGui() {
  QScopedPointer<FormStandardFeedDetails> form_pointer(new FormStandardFeedDetails(serviceRoot(),
                                                                                   qApp->mainFormWidget()));

  form_pointer->addEditFeed(this, this);
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
  bool result;
  StandardFeed* metadata = guessFeed(sourceType(),
                                     url(),
                                     postProcessScript(),
                                     &result,
                                     username(),
                                     password(),
                                     getParentServiceRoot()->networkProxy());

  if (metadata != nullptr && result) {
    // Some properties are not updated when new metadata are fetched.
    metadata->setParent(parent());
    metadata->setUrl(url());
    metadata->setPasswordProtected(passwordProtected());
    metadata->setUsername(username());
    metadata->setPassword(password());
    metadata->setAutoUpdateType(autoUpdateType());
    metadata->setAutoUpdateInitialInterval(autoUpdateInitialInterval());
    metadata->setPostProcessScript(postProcessScript());
    metadata->setSourceType(sourceType());
    editItself(metadata);
    delete metadata;

    // Notify the model about fact, that it needs to reload new information about
    // this item, particularly the icon.
    serviceRoot()->itemChanged(QList<RootItem*>() << this);
  }
  else {
    qApp->showGuiMessage(tr("Metadata not fetched"),
                         tr("Metadata was not fetched."),
                         QSystemTrayIcon::MessageIcon::Critical);
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

void StandardFeed::setSourceType(const SourceType& source_type) {
  m_sourceType = source_type;
}

StandardFeed* StandardFeed::guessFeed(StandardFeed::SourceType source_type,
                                      const QString& source,
                                      const QString& post_process_script,
                                      bool* result,
                                      const QString& username,
                                      const QString& password,
                                      const QNetworkProxy& custom_proxy) {
  auto timeout = qApp->settings()->value(GROUP(Feeds),
                                         SETTING(Feeds::UpdateTimeout)).toInt();
  QByteArray feed_contents;
  QList<QString> icon_possible_locations;
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
      *result = false;
      return nullptr;
    }

    icon_possible_locations.append(source);
  }
  else {
    qDebugNN << LOGSEC_CORE
             << "Running custom script for guessing"
             << QUOTE_W_SPACE(source)
             << "to obtain feed data.";

    // Use script to generate feed file.
    try {
      feed_contents = generateFeedFileWithScript(source, timeout).toUtf8();
    }
    catch (const ScriptException& ex) {
      qCriticalNN << LOGSEC_CORE
                  << "Custom script for generating feed file failed during guessing:"
                  << QUOTE_W_SPACE_DOT(ex.message());

      *result = false;
      return nullptr;
    }
  }

  if (!post_process_script.simplified().isEmpty()) {
    qDebugNN << LOGSEC_CORE
             << "Post-processing obtained feed data with custom script for guessing"
             << QUOTE_W_SPACE_DOT(post_process_script);

    try {
      feed_contents = postProcessFeedFileWithScript(post_process_script,
                                                    feed_contents,
                                                    timeout).toUtf8();
    }
    catch (const ScriptException& ex) {
      qCriticalNN << LOGSEC_CORE
                  << "Post-processing script for feed file for guessing failed:"
                  << QUOTE_W_SPACE_DOT(ex.message());

      *result = false;
      return nullptr;
    }
  }

  StandardFeed* feed = nullptr;

  if (content_type.contains(QSL("json"), Qt::CaseSensitivity::CaseInsensitive) ||
      feed_contents.startsWith('{')) {
    feed = new StandardFeed();

    // We have JSON feed.
    feed->setEncoding(DEFAULT_FEED_ENCODING);
    feed->setType(Type::Json);

    QJsonDocument json = QJsonDocument::fromJson(feed_contents);

    feed->setTitle(json.object()["title"].toString());
    feed->setDescription(json.object()["description"].toString());

    auto icon = json.object()["icon"].toString();

    if (icon.isEmpty()) {
      icon = json.object()["favicon"].toString();
    }

    if (!icon.isEmpty()) {
      icon_possible_locations.prepend(icon);
    }
  }
  else {
    // Feed XML was obtained, now we need to try to guess
    // its encoding before we can read further data.
    QString xml_schema_encoding;
    QString xml_contents_encoded;
    QString enc = QRegularExpression(QSL("encoding=\"([A-Z0-9\\-]+)\""),
                                     QRegularExpression::PatternOption::CaseInsensitiveOption)
                  .match(feed_contents)
                  .captured(1);

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
                                 &error_msg,
                                 &error_line,
                                 &error_column)) {
      qDebugNN << LOGSEC_CORE
               << "XML of feed" << QUOTE_W_SPACE(source) << "is not valid and cannot be loaded. "
               << "Error:" << QUOTE_W_SPACE(error_msg) << "(line " << error_line
               << ", column " << error_column << ").";

      *result = false;
      return nullptr;
    }

    feed = new StandardFeed();
    feed->setEncoding(encod);

    QDomElement root_element = xml_document.documentElement();
    QString root_tag_name = root_element.tagName();

    if (root_tag_name == QL1S("rdf:RDF")) {
      // We found RDF feed.
      QDomElement channel_element = root_element.namedItem(QSL("channel")).toElement();

      feed->setType(Type::Rdf);
      feed->setTitle(channel_element.namedItem(QSL("title")).toElement().text());
      feed->setDescription(channel_element.namedItem(QSL("description")).toElement().text());

      QString source_link = channel_element.namedItem(QSL("link")).toElement().text();

      if (!source_link.isEmpty()) {
        icon_possible_locations.prepend(source_link);
      }
    }
    else if (root_tag_name == QL1S("rss")) {
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
      QString icon_url_link = channel_element.namedItem(QSL("image")).namedItem(QSL("url")).toElement().text();

      if (!icon_url_link.isEmpty()) {
        icon_possible_locations.prepend(icon_url_link);
      }
      else if (!icon_link.isEmpty()) {
        icon_possible_locations.prepend(icon_link);
      }

      QString source_link = channel_element.namedItem(QSL("link")).toElement().text();

      if (!source_link.isEmpty()) {
        icon_possible_locations.append(source_link);
      }
    }
    else if (root_tag_name == QL1S("feed")) {
      // We found ATOM feed.
      feed->setType(Type::Atom10);
      feed->setTitle(root_element.namedItem(QSL("title")).toElement().text());
      feed->setDescription(root_element.namedItem(QSL("subtitle")).toElement().text());

      QString icon_link = root_element.namedItem(QSL("icon")).toElement().text();

      if (!icon_link.isEmpty()) {
        icon_possible_locations.prepend(icon_link);
      }

      QString logo_link = root_element.namedItem(QSL("logo")).toElement().text();

      if (!logo_link.isEmpty()) {
        icon_possible_locations.prepend(logo_link);
      }

      QString source_link = root_element.namedItem(QSL("link")).toElement().text();

      if (!source_link.isEmpty()) {
        icon_possible_locations.prepend(source_link);
      }
    }
    else {
      // File was downloaded and it really was XML file
      // but feed format was NOT recognized.
      feed->deleteLater();

      *result = false;
      return nullptr;
    }
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

  *result = true;
  return feed;
}

Qt::ItemFlags StandardFeed::additionalFlags() const {
  return Qt::ItemFlag::ItemIsDragEnabled;
}

bool StandardFeed::performDragDropChange(RootItem* target_item) {
  auto* feed_new = new StandardFeed(*this);

  feed_new->setParent(target_item);

  if (editItself(feed_new)) {
    serviceRoot()->requestItemReassignment(this, target_item);
    delete feed_new;
    return true;
  }
  else {
    delete feed_new;
    return false;
  }
}

bool StandardFeed::removeItself() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, customId().toInt(), getParentServiceRoot()->accountId());
}

bool StandardFeed::addItself(RootItem* parent) {
  // Now, add feed to persistent storage.
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  bool ok;
  int new_id = DatabaseQueries::addStandardFeed(database, parent->id(), parent->getParentServiceRoot()->accountId(),
                                                title(), description(), creationDate(), icon(), encoding(), url(),
                                                passwordProtected(), username(), password(), autoUpdateType(),
                                                autoUpdateInitialInterval(), sourceType(), postProcessScript(),
                                                type(), &ok);

  if (!ok) {
    // Query failed.
    return false;
  }
  else {
    // New feed was added, fetch is primary id from the database.
    setId(new_id);
    setCustomId(QString::number(new_id));
    return true;
  }
}

bool StandardFeed::editItself(StandardFeed* new_feed_data) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  StandardFeed* original_feed = this;
  RootItem* new_parent = new_feed_data->parent();

  if (!DatabaseQueries::editStandardFeed(database, new_parent->id(), original_feed->id(), new_feed_data->title(),
                                         new_feed_data->description(), new_feed_data->icon(),
                                         new_feed_data->encoding(), new_feed_data->url(), new_feed_data->passwordProtected(),
                                         new_feed_data->username(), new_feed_data->password(),
                                         new_feed_data->autoUpdateType(), new_feed_data->autoUpdateInitialInterval(),
                                         new_feed_data->sourceType(), new_feed_data->postProcessScript(),
                                         new_feed_data->type())) {
    // Persistent storage update failed, no way to continue now.
    qWarningNN << LOGSEC_CORE
               << "Self-editing of standard feed failed.";
    return false;
  }

  // Setup new model data for the original item.
  original_feed->setTitle(new_feed_data->title());
  original_feed->setDescription(new_feed_data->description());
  original_feed->setIcon(new_feed_data->icon());
  original_feed->setEncoding(new_feed_data->encoding());
  original_feed->setDescription(new_feed_data->description());
  original_feed->setUrl(new_feed_data->url());
  original_feed->setPasswordProtected(new_feed_data->passwordProtected());
  original_feed->setUsername(new_feed_data->username());
  original_feed->setPassword(new_feed_data->password());
  original_feed->setAutoUpdateType(new_feed_data->autoUpdateType());
  original_feed->setAutoUpdateInitialInterval(new_feed_data->autoUpdateInitialInterval());
  original_feed->setType(new_feed_data->type());
  original_feed->setSourceType(new_feed_data->sourceType());
  original_feed->setPostProcessScript(new_feed_data->postProcessScript());

  // Editing is done.
  return true;
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

QList<Message> StandardFeed::obtainNewMessages(bool* error_during_obtaining) {
  QString formatted_feed_contents;
  int download_timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();

  if (sourceType() == SourceType::Url) {
    qDebugNN << LOGSEC_CORE
             << "Downloading URL"
             << QUOTE_W_SPACE(url())
             << "to obtain feed data.";

    QByteArray feed_contents;
    QList<QPair<QByteArray, QByteArray>> headers;

    headers << NetworkFactory::generateBasicAuthHeader(username(), password());
    m_networkError = NetworkFactory::performNetworkOperation(url(),
                                                             download_timeout,
                                                             QByteArray(),
                                                             feed_contents,
                                                             QNetworkAccessManager::Operation::GetOperation,
                                                             headers,
                                                             false,
                                                             {},
                                                             {},
                                                             getParentServiceRoot()->networkProxy()).first;

    if (m_networkError != QNetworkReply::NetworkError::NoError) {
      qWarningNN << LOGSEC_CORE
                 << "Error"
                 << QUOTE_W_SPACE(m_networkError)
                 << "during fetching of new messages for feed"
                 << QUOTE_W_SPACE_DOT(url());
      setStatus(Status::NetworkError);
      *error_during_obtaining = true;
      return QList<Message>();
    }
    else {
      *error_during_obtaining = false;
    }

    // Encode downloaded data for further parsing.
    QTextCodec* codec = QTextCodec::codecForName(encoding().toLocal8Bit());

    if (codec == nullptr) {
      // No suitable codec for this encoding was found.
      // Use non-converted data.
      formatted_feed_contents = feed_contents;
    }
    else {
      formatted_feed_contents = codec->toUnicode(feed_contents);
    }
  }
  else {
    qDebugNN << LOGSEC_CORE
             << "Running custom script"
             << QUOTE_W_SPACE(url())
             << "to obtain feed data.";

    // Use script to generate feed file.
    try {
      formatted_feed_contents = generateFeedFileWithScript(url(), download_timeout);
    }
    catch (const ScriptException& ex) {
      qCriticalNN << LOGSEC_CORE
                  << "Custom script for generating feed file failed:"
                  << QUOTE_W_SPACE_DOT(ex.message());

      setStatus(Status::OtherError);
      *error_during_obtaining = true;
      return {};
    }
  }

  if (!postProcessScript().simplified().isEmpty()) {
    qDebugNN << LOGSEC_CORE
             << "Post-processing obtained feed data with custom script"
             << QUOTE_W_SPACE_DOT(postProcessScript());

    try {
      formatted_feed_contents = postProcessFeedFileWithScript(postProcessScript(),
                                                              formatted_feed_contents,
                                                              download_timeout);
    }
    catch (const ScriptException& ex) {
      qCriticalNN << LOGSEC_CORE
                  << "Post-processing script for feed file failed:"
                  << QUOTE_W_SPACE_DOT(ex.message());

      setStatus(Status::OtherError);
      *error_during_obtaining = true;
      return {};
    }
  }

  // Feed data are downloaded and encoded.
  // Parse data and obtain messages.
  QList<Message> messages;

  switch (type()) {
    case StandardFeed::Type::Rss0X:
    case StandardFeed::Type::Rss2X:
      messages = RssParser(formatted_feed_contents).messages();
      break;

    case StandardFeed::Type::Rdf:
      messages = RdfParser().parseXmlData(formatted_feed_contents);
      break;

    case StandardFeed::Type::Atom10:
      messages = AtomParser(formatted_feed_contents).messages();
      break;

    case StandardFeed::Type::Json:
      messages = JsonParser(formatted_feed_contents).messages();
      break;

    default:
      break;
  }

  return messages;
}

QStringList StandardFeed::prepareExecutionLine(const QString& execution_line) {
  auto split_exec = execution_line.split('#',
#if QT_VERSION >= 0x050F00 // Qt >= 5.15.0
                                         Qt::SplitBehaviorFlags::SkipEmptyParts);
#else
                                         QString::SplitBehavior::SkipEmptyParts);
#endif
  auto user_data_folder = qApp->userDataFolder();

  return split_exec.replaceInStrings(EXECUTION_LINE_USER_DATA_PLACEHOLDER, user_data_folder);
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

  if (process.waitForFinished(run_timeout) && process.exitStatus() == QProcess::ExitStatus::NormalExit) {
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
    process.kill();

    auto raw_error = process.readAllStandardError().simplified();

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

QNetworkReply::NetworkError StandardFeed::networkError() const {
  return m_networkError;
}

StandardFeed::StandardFeed(const QSqlRecord& record) : Feed(record) {
  setEncoding(record.value(FDS_DB_ENCODING_INDEX).toString());
  setSourceType(SourceType(record.value(FDS_DB_SOURCE_TYPE_INDEX).toInt()));
  setPostProcessScript(record.value(FDS_DB_POST_PROCESS).toString());

  StandardFeed::Type type = static_cast<StandardFeed::Type>(record.value(FDS_DB_TYPE_INDEX).toInt());

  switch (type) {
    case StandardFeed::Type::Atom10:
    case StandardFeed::Type::Rdf:
    case StandardFeed::Type::Rss0X:
    case StandardFeed::Type::Rss2X:
    case StandardFeed::Type::Json: {
      setType(type);
      break;
    }
  }

  m_networkError = QNetworkReply::NetworkError::NoError;
}
