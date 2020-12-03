// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/standardfeed.h"

#include "core/feedsmodel.h"
#include "definitions/definitions.h"
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
#include <QTextCodec>
#include <QVariant>
#include <QXmlStreamReader>

StandardFeed::StandardFeed(RootItem* parent_item)
  : Feed(parent_item) {
  m_networkError = QNetworkReply::NoError;
  m_type = Type::Rss0X;
  m_encoding = QString();
}

StandardFeed::StandardFeed(const StandardFeed& other)
  : Feed(other) {
  m_networkError = other.networkError();
  m_type = other.type();
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

void StandardFeed::fetchMetadataForItself() {
  QPair<StandardFeed*, QNetworkReply::NetworkError> metadata = guessFeed(url(), username(), password());

  if (metadata.first != nullptr && metadata.second == QNetworkReply::NetworkError::NoError) {
    // Some properties are not updated when new metadata are fetched.
    metadata.first->setParent(parent());
    metadata.first->setUrl(url());
    metadata.first->setPasswordProtected(passwordProtected());
    metadata.first->setUsername(username());
    metadata.first->setPassword(password());
    metadata.first->setAutoUpdateType(autoUpdateType());
    metadata.first->setAutoUpdateInitialInterval(autoUpdateInitialInterval());
    editItself(metadata.first);
    delete metadata.first;

    // Notify the model about fact, that it needs to reload new information about
    // this item, particularly the icon.
    serviceRoot()->itemChanged(QList<RootItem*>() << this);
  }
  else {
    qApp->showGuiMessage(tr("Metadata not fetched"),
                         tr("Metadata was not fetched because: %1.").arg(NetworkFactory::networkErrorText(metadata.second)),
                         QSystemTrayIcon::Critical);
  }
}

QPair<StandardFeed*, QNetworkReply::NetworkError> StandardFeed::guessFeed(const QString& url,
                                                                          const QString& username,
                                                                          const QString& password) {
  QPair<StandardFeed*, QNetworkReply::NetworkError> result;

  result.first = nullptr;
  QByteArray feed_contents;
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << NetworkFactory::generateBasicAuthHeader(username, password);

  NetworkResult network_result = NetworkFactory::performNetworkOperation(url,
                                                                         qApp->settings()->value(GROUP(Feeds),
                                                                                                 SETTING(Feeds::UpdateTimeout)).toInt(),
                                                                         QByteArray(),
                                                                         feed_contents,
                                                                         QNetworkAccessManager::GetOperation,
                                                                         headers);

  result.second = network_result.first;

  if (result.second == QNetworkReply::NoError || !feed_contents.isEmpty()) {
    if (result.first == nullptr) {
      result.first = new StandardFeed();
    }

    QList<QString> icon_possible_locations;

    icon_possible_locations.append(url);

    if (network_result.second.toString().contains(QSL("json"), Qt::CaseSensitivity::CaseInsensitive)) {
      // We have JSON feed.
      result.first->setEncoding(DEFAULT_FEED_ENCODING);
      result.first->setType(Type::Json);

      QJsonDocument json = QJsonDocument::fromJson(feed_contents);

      result.first->setTitle(json.object()["title"].toString());
      result.first->setDescription(json.object()["description"].toString());

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
                                       QRegularExpression::PatternOption::CaseInsensitiveOption).match(feed_contents).captured(1);

      if (!enc.isEmpty()) {
        // Some "encoding" attribute was found get the encoding
        // out of it.
        xml_schema_encoding = enc;
      }

      QTextCodec* custom_codec = QTextCodec::codecForName(xml_schema_encoding.toLocal8Bit());

      if (custom_codec != nullptr) {
        // Feed encoding was probably guessed.
        xml_contents_encoded = custom_codec->toUnicode(feed_contents);
        result.first->setEncoding(xml_schema_encoding);
      }
      else {
        // Feed encoding probably not guessed, set it as
        // default.
        xml_contents_encoded = feed_contents;
        result.first->setEncoding(DEFAULT_FEED_ENCODING);
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
                 << "XML of feed" << QUOTE_W_SPACE(url) << "is not valid and cannot be loaded. "
                 << "Error:" << QUOTE_W_SPACE(error_msg) << "(line " << error_line
                 << ", column " << error_column << ").";
        result.second = QNetworkReply::UnknownContentError;

        // XML is invalid, exit.
        return result;
      }

      QDomElement root_element = xml_document.documentElement();
      QString root_tag_name = root_element.tagName();

      if (root_tag_name == QL1S("rdf:RDF")) {
        // We found RDF feed.
        QDomElement channel_element = root_element.namedItem(QSL("channel")).toElement();

        result.first->setType(Type::Rdf);
        result.first->setTitle(channel_element.namedItem(QSL("title")).toElement().text());
        result.first->setDescription(channel_element.namedItem(QSL("description")).toElement().text());
        QString source_link = channel_element.namedItem(QSL("link")).toElement().text();

        if (!source_link.isEmpty()) {
          icon_possible_locations.prepend(source_link);
        }
      }
      else if (root_tag_name == QL1S("rss")) {
        // We found RSS 0.91/0.92/0.93/2.0/2.0.1 feed.
        QString rss_type = root_element.attribute("version", "2.0");

        if (rss_type == QL1S("0.91") || rss_type == QL1S("0.92") || rss_type == QL1S("0.93")) {
          result.first->setType(Type::Rss0X);
        }
        else {
          result.first->setType(Type::Rss2X);
        }

        QDomElement channel_element = root_element.namedItem(QSL("channel")).toElement();

        result.first->setTitle(channel_element.namedItem(QSL("title")).toElement().text());
        result.first->setDescription(channel_element.namedItem(QSL("description")).toElement().text());

        QString icon_link = channel_element.namedItem(QSL("image")).toElement().text();

        if (!icon_link.isEmpty()) {
          icon_possible_locations.prepend(icon_link);
        }

        QString source_link = channel_element.namedItem(QSL("link")).toElement().text();

        if (!source_link.isEmpty()) {
          icon_possible_locations.prepend(source_link);
        }
      }
      else if (root_tag_name == QL1S("feed")) {
        // We found ATOM feed.
        result.first->setType(Type::Atom10);
        result.first->setTitle(root_element.namedItem(QSL("title")).toElement().text());
        result.first->setDescription(root_element.namedItem(QSL("subtitle")).toElement().text());

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
        result.second = QNetworkReply::UnknownContentError;
      }
    }

    // Try to obtain icon.
    QIcon icon_data;

    if ((result.second = NetworkFactory::downloadIcon(icon_possible_locations,
                                                      DOWNLOAD_TIMEOUT,
                                                      icon_data)) == QNetworkReply::NoError) {
      // Icon for feed was downloaded and is stored now in _icon_data.
      result.first->setIcon(icon_data);
    }
  }

  return result;
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
  int new_id = DatabaseQueries::addStandardFeed(database, parent->id(), parent->getParentServiceRoot()->accountId(), title(),
                                                description(), creationDate(), icon(), encoding(), url(), passwordProtected(),
                                                username(), password(), autoUpdateType(), autoUpdateInitialInterval(), type(), &ok);

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
  QByteArray feed_contents;
  int download_timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  QList<QPair<QByteArray, QByteArray>> headers;

  headers << NetworkFactory::generateBasicAuthHeader(username(), password());
  m_networkError = NetworkFactory::performNetworkOperation(url(),
                                                           download_timeout,
                                                           QByteArray(),
                                                           feed_contents,
                                                           QNetworkAccessManager::GetOperation,
                                                           headers).first;

  if (m_networkError != QNetworkReply::NoError) {
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
  QString formatted_feed_contents;

  if (codec == nullptr) {
    // No suitable codec for this encoding was found.
    // Use non-converted data.
    formatted_feed_contents = feed_contents;
  }
  else {
    formatted_feed_contents = codec->toUnicode(feed_contents);
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

QNetworkReply::NetworkError StandardFeed::networkError() const {
  return m_networkError;
}

StandardFeed::StandardFeed(const QSqlRecord& record) : Feed(record) {
  setEncoding(record.value(FDS_DB_ENCODING_INDEX).toString());

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

  m_networkError = QNetworkReply::NoError;
}
