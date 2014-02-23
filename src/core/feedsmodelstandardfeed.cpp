#include "core/feedsmodelstandardfeed.h"

#include "core/defs.h"
#include "core/settings.h"
#include "core/parsingfactory.h"
#include "core/databasefactory.h"
#include "core/networkfactory.h"
#include "core/textfactory.h"
#include "gui/iconfactory.h"
#include "gui/iconthemefactory.h"

#include <QVariant>
#include <QTextCodec>
#include <QSqlQuery>

#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>
#include <QXmlStreamReader>


FeedsModelStandardFeed::FeedsModelStandardFeed(FeedsModelRootItem *parent_item)
  : FeedsModelFeed(parent_item),
    m_autoUpdateType(DontAutoUpdate),
    m_autoUpdateInitialInterval(DEFAULT_AUTO_UPDATE_INTERVAL) {
}

FeedsModelStandardFeed::~FeedsModelStandardFeed() {
  qDebug("Destroying FeedsModelStandardFeed instance.");
}

FeedsModelStandardFeed *FeedsModelStandardFeed::loadFromRecord(const QSqlRecord &record) {
  FeedsModelStandardFeed *feed = new FeedsModelStandardFeed(NULL);

  feed->setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  feed->setId(record.value(FDS_DB_ID_INDEX).toInt());
  feed->setDescription(record.value(FDS_DB_DESCRIPTION_INDEX).toString());
  feed->setCreationDate(TextFactory::parseDateTime(record.value(FDS_DB_DCREATED_INDEX).value<qint64>()).toLocalTime());
  feed->setIcon(IconFactory::fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  feed->setEncoding(record.value(FDS_DB_ENCODING_INDEX).toString());
  feed->setUrl(record.value(FDS_DB_URL_INDEX).toString());
  feed->setPasswordProtected(record.value(FDS_DB_PROTECTED_INDEX).toBool());
  feed->setUsername(record.value(FDS_DB_USERNAME_INDEX).toString());
  feed->setPassword(record.value(FDS_DB_PASSWORD_INDEX).toString());
  feed->setAutoUpdateType(static_cast<FeedsModelStandardFeed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  feed->setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  feed->updateCounts();

  return feed;
}

QPair<FeedsModelStandardFeed*, QNetworkReply::NetworkError> FeedsModelStandardFeed::guessFeed(const QString &url,
                                                                                              const QString &username,
                                                                                              const QString &password) {
  QPair<FeedsModelStandardFeed*, QNetworkReply::NetworkError> result; result.first = NULL;

  // Try to obtain icon.
  QIcon icon_data;

  if ((result.second = NetworkFactory::downloadIcon(url,
                                                    5000,
                                                    icon_data)) ==
      QNetworkReply::NoError) {
    // Icon for feed was downloaded and is stored now in _icon_data.
    result.first = new FeedsModelStandardFeed();
    result.first->setIcon(icon_data);
  }

  QByteArray feed_contents;
  if ((result.second = NetworkFactory::downloadFeedFile(url,
                                                        Settings::instance()->value(APP_CFG_FEEDS, "feed_update_timeout", DOWNLOAD_TIMEOUT).toInt(),
                                                        feed_contents,
                                                        !username.isEmpty(),
                                                        username,
                                                        password)) == QNetworkReply::NoError) {
    // Feed XML was obtained, now we need to try to guess
    // its encoding before we can read further data.
    QString xml_schema_encoding;
    QString xml_contents_encoded;
    QRegExp encoding_rexp("encoding=\"[^\"]\\S+\"");

    if (encoding_rexp.indexIn(feed_contents) != -1 &&
        !(xml_schema_encoding = encoding_rexp.cap(0)).isEmpty()) {
      // Some "encoding" attribute was found get the encoding
      // out of it.
      encoding_rexp.setPattern("[^\"]\\S+[^\"]");
      encoding_rexp.indexIn(xml_schema_encoding, 9);
      xml_schema_encoding = encoding_rexp.cap(0);
    }

    if (result.first == NULL) {
      result.first = new FeedsModelStandardFeed();
    }

    QTextCodec *custom_codec = QTextCodec::codecForName(xml_schema_encoding.toLocal8Bit());

    if (custom_codec != NULL) {
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
      qDebug("XML of feed '%s' is not valid and cannot be loaded. Error: '%s' "
             "(line %d, column %d).",
             qPrintable(url),
             qPrintable(error_msg),
             error_line, error_column);

      result.second = QNetworkReply::UnknownContentError;

      // XML is invalid, exit.
      return result;
    }

    QDomElement root_element = xml_document.documentElement();
    QString root_tag_name = root_element.tagName();

    if (root_tag_name == "rdf:RDF") {
      // We found RDF feed.
      QDomElement channel_element = root_element.namedItem("channel").toElement();

      result.first->setType(StandardRdf);
      result.first->setTitle(channel_element.namedItem("title").toElement().text());
      result.first->setDescription(channel_element.namedItem("description").toElement().text());
    }
    else if (root_tag_name == "rss") {
      // We found RSS 0.91/0.92/0.93/2.0/2.0.1 feed.
      QString rss_type = root_element.attribute("version", "2.0");

      if (rss_type == "0.91" || rss_type == "0.92" || rss_type == "0.93") {
        result.first->setType(StandardRss0X);
      }
      else {
        result.first->setType(StandardRss2X);
      }

      QDomElement channel_element = root_element.namedItem("channel").toElement();

      result.first->setTitle(channel_element.namedItem("title").toElement().text());
      result.first->setDescription(channel_element.namedItem("description").toElement().text());
    }
    else if (root_tag_name == "feed") {
      // We found ATOM feed.
      result.first->setType(StandardAtom10);
      result.first->setTitle(root_element.namedItem("title").toElement().text());
      result.first->setDescription(root_element.namedItem("subtitle").toElement().text());
    }
    else {
      // File was downloaded and it really was XML file
      // but feed format was NOT recognized.
      result.second = QNetworkReply::UnknownContentError;
    }
  }

  return result;
}

QVariant FeedsModelStandardFeed::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        // TODO: Changeable text.
        return QString("%1").arg(QString::number(countOfUnreadMessages()));
                                 //QString::number(countOfAllMessages()));
      }
      else {
        return QVariant();
      }

    case Qt::EditRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return countOfUnreadMessages();
      }
      else {
        return QVariant();
      }

    case Qt::DecorationRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_icon;
      }
      else {
        return QVariant();
      }

    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        QString auto_update_string;

        switch (m_autoUpdateType) {
          case DontAutoUpdate:
            auto_update_string = QObject::tr("does not use auto-update");
            break;

          case DefaultAutoUpdate:
            auto_update_string = QObject::tr("uses global settings");
            break;

          case SpecificAutoUpdate:
          default:
            auto_update_string = QObject::tr("uses specific settings "
                                             "(%n minute(s) to next auto-update)",
                                             0,
                                             m_autoUpdateRemainingInterval);
            break;
        }

        return QObject::tr("%1 (%2)\n"
                           "%3\n\n"
                           "Encoding: %4\n"
                           "Auto-update status: %5").arg(m_title,
                                                         FeedsModelFeed::typeToString(m_type),
                                                         m_description,
                                                         m_encoding,
                                                         auto_update_string);
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return QObject::tr("%n unread message(s).", 0, countOfUnreadMessages());
      }
      else {
        return QVariant();
      }

    case Qt::TextAlignmentRole:
      if (column == FDS_MODEL_COUNTS_INDEX) {
        return Qt::AlignCenter;
      }
      else {
        return QVariant();
      }

    default:
      return QVariant();
  }
}

void FeedsModelStandardFeed::update() {
  QByteArray feed_contents;
  int download_timeout = Settings::instance()->value(APP_CFG_FEEDS, "feed_update_timeout", DOWNLOAD_TIMEOUT).toInt();
  QNetworkReply::NetworkError download_result = NetworkFactory::downloadFeedFile(url(),
                                                                                 download_timeout,
                                                                                 feed_contents,
                                                                                 passwordProtected(),
                                                                                 username(),
                                                                                 password());

  if (download_result != QNetworkReply::NoError) {
    qWarning("Error during fetching of new messages for feed '%s' (id %d).",
             qPrintable(url()),
             id());
    return;
  }

  // Encode downloaded data for further parsing.
  QTextCodec *codec = QTextCodec::codecForName(encoding().toLocal8Bit());
  QString formatted_feed_contents;

  if (codec == NULL) {
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
    case FeedsModelFeed::StandardRss0X:
    case FeedsModelFeed::StandardRss2X:
      messages = ParsingFactory::parseAsRSS20(formatted_feed_contents);
      break;

    case FeedsModelFeed::StandardRdf:
      messages = ParsingFactory::parseAsRDF(formatted_feed_contents);
      break;

    case FeedsModelFeed::StandardAtom10:
      messages = ParsingFactory::parseAsATOM10(formatted_feed_contents);

    default:
      break;
  }

  updateMessages(messages);
}

bool FeedsModelStandardFeed::removeItself() {
  QSqlDatabase database = DatabaseFactory::instance()->connection("FeedsModelStandardFeed",
                                                                  DatabaseFactory::FromSettings);
  QSqlQuery query_remove(database);

  query_remove.setForwardOnly(true);

  // Remove all messages from this standard feed.
  query_remove.prepare("DELETE FROM Messages WHERE feed = :feed;");
  query_remove.bindValue(":feed", id());

  if (!query_remove.exec()) {
    return false;
  }

  // Remove feed itself.
  query_remove.prepare("DELETE FROM Feeds WHERE id = :feed;");
  query_remove.bindValue(":feed", id());

  return query_remove.exec();
}

void FeedsModelStandardFeed::updateMessages(const QList<Message> &messages) {
  int feed_id = id();
  QSqlDatabase database = DatabaseFactory::instance()->connection("FeedsModelStandardFeed",
                                                                  DatabaseFactory::FromSettings);

  // Prepare queries.
  QSqlQuery query_select(database);
  QSqlQuery query_insert(database);

  // Used to check if give feed contains with message with given
  // title, url and date_created.
  // WARNING: One feed CANNOT contain
  // two (or more) messages with same
  // AUTHOR AND TITLE AND URL AND DATE_CREATED.
  query_select.setForwardOnly(true);
  query_select.prepare("SELECT id, feed, date_created FROM Messages "
                       "WHERE feed = :feed AND title = :title AND url = :url AND author = :author;");

  // Used to insert new messages.
  query_insert.setForwardOnly(true);
  query_insert.prepare("INSERT INTO Messages "
                       "(feed, title, url, author, date_created, contents) "
                       "VALUES (:feed, :title, :url, :author, :date_created, :contents);");

  if (!database.transaction()) {
    database.rollback();

    qDebug("Transaction start for message downloader failed.");
    return;
  }

  foreach (const Message &message, messages) {
    query_select.bindValue(":feed", feed_id);
    query_select.bindValue(":title", message.m_title);
    query_select.bindValue(":url", message.m_url);
    query_select.bindValue(":author", message.m_author);
    query_select.exec();

    QList<qint64> datetime_stamps;

    while (query_select.next()) {
      datetime_stamps << query_select.value(2).value<qint64>();
    }

    query_select.finish();

    if (datetime_stamps.size() == 0 ||
        (message.m_createdFromFeed &&
         !datetime_stamps.contains(message.m_created.toMSecsSinceEpoch()))) {
      // Message is not fetched in this feed yet
      // or it is. If it is, then go
      // through datetime stamps of stored messages
      // and check if new (not auto-generated timestamp
      // is among them and add this message if it is not.
      query_insert.bindValue(":feed", feed_id);
      query_insert.bindValue(":title", message.m_title);
      query_insert.bindValue(":url", message.m_url);
      query_insert.bindValue(":author", message.m_author);
      query_insert.bindValue(":date_created", message.m_created.toMSecsSinceEpoch());
      query_insert.bindValue(":contents", message.m_contents);

      query_insert.exec();
      query_insert.finish();
    }
  }

  if (!database.commit()) {
    database.rollback();

    qDebug("Transaction commit for message downloader failed.");
  }
}




