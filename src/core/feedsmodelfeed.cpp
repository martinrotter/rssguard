// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "core/feedsmodelfeed.h"

#include "definitions/definitions.h"
#include "core/parsingfactory.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QTextCodec>
#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>
#include <QXmlStreamReader>


FeedsModelFeed::FeedsModelFeed(FeedsModelRootItem *parent_item)
  : FeedsModelRootItem(parent_item),
    m_passwordProtected(false),
    m_username(QString()),
    m_password(QString()),
    m_status(Normal),
    m_networkError(QNetworkReply::NoError),
    m_type(Rss0X),
    m_totalCount(0),
    m_unreadCount(0),
    m_autoUpdateType(DontAutoUpdate),
    m_autoUpdateInitialInterval(DEFAULT_AUTO_UPDATE_INTERVAL),
    m_autoUpdateRemainingInterval(DEFAULT_AUTO_UPDATE_INTERVAL),
    m_encoding(QString()),
    m_url(QString()) {
  m_kind = FeedsModelRootItem::Feed;
}

FeedsModelFeed::FeedsModelFeed(const FeedsModelFeed &other)
  : FeedsModelRootItem(NULL), m_totalCount(0), m_unreadCount(0) {
  m_passwordProtected = other.passwordProtected();
  m_username = other.username();
  m_password = other.password();
  m_status = other.status();
  m_networkError = other.networkError();
  m_type = other.type();
  m_autoUpdateType = other.autoUpdateType();
  m_autoUpdateInitialInterval = other.autoUpdateInitialInterval();
  m_autoUpdateRemainingInterval = other.autoUpdateRemainingInterval();
  m_encoding = other.encoding();
  m_url = other.url();
  m_kind = FeedsModelRootItem::Feed;
  m_title = other.title();
  m_id = other.id();
  m_icon = other.icon();
  m_childItems = other.childItems();
  m_parentItem = other.parent();
  m_creationDate = other.creationDate();
  m_description = other.description();
}

FeedsModelFeed::~FeedsModelFeed() {
  qDebug("Destroying FeedsModelFeed instance.");
}

int FeedsModelFeed::childCount() const {
  // Because feed has no children.
  return 0;
}

int FeedsModelFeed::countOfAllMessages() const {
  return m_totalCount;
}

int FeedsModelFeed::countOfUnreadMessages() const {
  return m_unreadCount;
}

QString FeedsModelFeed::typeToString(FeedsModelFeed::Type type) {
  switch (type) {
    case Atom10:
      return "ATOM 1.0";

    case Rdf:
      return "RDF (RSS 1.0)";

    case Rss0X:
      return "RSS 0.91/0.92/0.93";

    case Rss2X:
    default:
      return "RSS 2.0/2.0.1";
  }
}

void FeedsModelFeed::updateCounts(bool including_total_count, bool update_feed_statuses) {
  QSqlDatabase database = qApp->database()->connection("FeedsModelFeed",
                                                       DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);
  query_all.setForwardOnly(true);

  if (including_total_count) {
    if (query_all.exec(QString("SELECT count(*) FROM Messages "
                               "WHERE feed = %1 AND is_deleted = 0;").arg(id())) &&
        query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
  }

  // Obtain count of unread messages.
  if (query_all.exec(QString("SELECT count(*) FROM Messages "
                             "WHERE feed = %1 AND is_deleted = 0 AND is_read = 0;").arg(id())) &&
      query_all.next()) {
    int new_unread_count = query_all.value(0).toInt();

    if (update_feed_statuses && m_status == NewMessages && new_unread_count < m_unreadCount) {
      m_status = Normal;
    }

    m_unreadCount = new_unread_count;
  }
}

FeedsModelFeed *FeedsModelFeed::loadFromRecord(const QSqlRecord &record) {
  FeedsModelFeed *feed = new FeedsModelFeed();

  feed->setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  feed->setId(record.value(FDS_DB_ID_INDEX).toInt());
  feed->setDescription(record.value(FDS_DB_DESCRIPTION_INDEX).toString());
  feed->setCreationDate(TextFactory::parseDateTime(record.value(FDS_DB_DCREATED_INDEX).value<qint64>()).toLocalTime());
  feed->setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  feed->setEncoding(record.value(FDS_DB_ENCODING_INDEX).toString());
  feed->setUrl(record.value(FDS_DB_URL_INDEX).toString());
  feed->setPasswordProtected(record.value(FDS_DB_PROTECTED_INDEX).toBool());
  feed->setUsername(record.value(FDS_DB_USERNAME_INDEX).toString());
  feed->setPassword(record.value(FDS_DB_PASSWORD_INDEX).toString());
  feed->setAutoUpdateType(static_cast<FeedsModelFeed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  feed->setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
  feed->updateCounts();

  return feed;
}

QPair<FeedsModelFeed*, QNetworkReply::NetworkError> FeedsModelFeed::guessFeed(const QString &url,
                                                                              const QString &username,
                                                                              const QString &password) {
  QPair<FeedsModelFeed*, QNetworkReply::NetworkError> result; result.first = NULL;

  // Try to obtain icon.
  QIcon icon_data;

  if ((result.second = NetworkFactory::downloadIcon(url, 5000, icon_data)) ==
      QNetworkReply::NoError) {
    // Icon for feed was downloaded and is stored now in _icon_data.
    result.first = new FeedsModelFeed();
    result.first->setIcon(icon_data);
  }

  QByteArray feed_contents;
  if ((result.second = NetworkFactory::downloadFile(url,
                                                    qApp->settings()->value(APP_CFG_FEEDS, "feed_update_timeout", DOWNLOAD_TIMEOUT).toInt(),
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
      result.first = new FeedsModelFeed();
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

      result.first->setType(Rdf);
      result.first->setTitle(channel_element.namedItem("title").toElement().text());
      result.first->setDescription(channel_element.namedItem("description").toElement().text());
    }
    else if (root_tag_name == "rss") {
      // We found RSS 0.91/0.92/0.93/2.0/2.0.1 feed.
      QString rss_type = root_element.attribute("version", "2.0");

      if (rss_type == "0.91" || rss_type == "0.92" || rss_type == "0.93") {
        result.first->setType(Rss0X);
      }
      else {
        result.first->setType(Rss2X);
      }

      QDomElement channel_element = root_element.namedItem("channel").toElement();

      result.first->setTitle(channel_element.namedItem("title").toElement().text());
      result.first->setDescription(channel_element.namedItem("description").toElement().text());
    }
    else if (root_tag_name == "feed") {
      // We found ATOM feed.
      result.first->setType(Atom10);
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

QVariant FeedsModelFeed::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return qApp->settings()->value(APP_CFG_FEEDS,
                                       "count_format",
                                       "(%unread)").toString()
            .replace("%unread", QString::number(countOfUnreadMessages()))
            .replace("%all", QString::number(countOfAllMessages()));
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
            //: Describes feed auto-update status.
            auto_update_string = tr("does not use auto-update");
            break;

          case DefaultAutoUpdate:
            //: Describes feed auto-update status.
            auto_update_string = tr("uses global settings");
            break;

          case SpecificAutoUpdate:
          default:
            //: Describes feed auto-update status.
            auto_update_string = tr("uses specific settings "
                                    "(%n minute(s) to next auto-update)",
                                    0,
                                    m_autoUpdateRemainingInterval);
            break;
        }

        //: Tooltip for feed.
        return tr("%1 (%2)\n"
                  "%3\n\n"
                  "Network status: %6\n"
                  "Encoding: %4\n"
                  "Auto-update status: %5").arg(m_title,
                                                FeedsModelFeed::typeToString(m_type),
                                                m_description,
                                                m_encoding,
                                                auto_update_string,
                                                NetworkFactory::networkErrorText(m_networkError));
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        //: Tooltip for "unread" column of feed list.
        return tr("%n unread message(s).", 0, countOfUnreadMessages());
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

    case Qt::FontRole:
      return countOfUnreadMessages() > 0 ? m_boldFont : m_normalFont;

    case Qt::ForegroundRole:
      switch (m_status) {
        case NewMessages:
          return QColor(Qt::blue);

        case NetworkError:
          return QColor(Qt::red);

        default:
          return QVariant();
      }

    default:
      return QVariant();
  }
}

void FeedsModelFeed::update() {
  QByteArray feed_contents;
  int download_timeout = qApp->settings()->value(APP_CFG_FEEDS, "feed_update_timeout", DOWNLOAD_TIMEOUT).toInt();
  m_networkError = NetworkFactory::downloadFile(url(), download_timeout,
                                                feed_contents, passwordProtected(),
                                                username(), password());

  if (m_networkError != QNetworkReply::NoError) {
    qWarning("Error during fetching of new messages for feed '%s' (id %d).", qPrintable(url()), id());
    m_status = NetworkError;

    return;
  }
  else {
    m_status = Normal;
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
    case FeedsModelFeed::Rss0X:
    case FeedsModelFeed::Rss2X:
      messages = ParsingFactory::parseAsRSS20(formatted_feed_contents);
      break;

    case FeedsModelFeed::Rdf:
      messages = ParsingFactory::parseAsRDF(formatted_feed_contents);
      break;

    case FeedsModelFeed::Atom10:
      messages = ParsingFactory::parseAsATOM10(formatted_feed_contents);

    default:
      break;
  }

  updateMessages(messages);
}

bool FeedsModelFeed::removeItself() {
  QSqlDatabase database = qApp->database()->connection("FeedsModelFeed",
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

void FeedsModelFeed::updateMessages(const QList<Message> &messages) {
  int feed_id = id();
  QSqlDatabase database = qApp->database()->connection("FeedsModelFeed",
                                                       DatabaseFactory::FromSettings);

  // Prepare queries.
  QSqlQuery query_select(database);
  QSqlQuery query_insert(database);

  // Used to check if give feed contains with message with given
  // title, url and date_created.
  // WARNING: One feed CANNOT contain two (or more) messages with same
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

      if (query_insert.exec() && query_insert.numRowsAffected() == 1) {
        setStatus(NewMessages);
      }

      query_insert.finish();
    }
  }

  if (!database.commit()) {
    database.rollback();

    qDebug("Transaction commit for message downloader failed.");
  }
}

QNetworkReply::NetworkError FeedsModelFeed::networkError() const {
  return m_networkError;
}
