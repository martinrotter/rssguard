#include "core/feedsmodelstandardfeed.h"

#include "core/defs.h"
#include "core/settings.h"
#include "core/parsingfactory.h"
#include "core/databasefactory.h"
#include "core/networkfactory.h"
#include "gui/iconfactory.h"
#include "gui/iconthemefactory.h"

#include <QVariant>
#include <QTextCodec>
#include <QSqlQuery>


FeedsModelStandardFeed::FeedsModelStandardFeed(FeedsModelRootItem *parent_item)
  : FeedsModelFeed(parent_item) {
}

FeedsModelStandardFeed::~FeedsModelStandardFeed() {
  qDebug("Destroying FeedsModelStandardFeed instance.");
}

void FeedsModelStandardFeed::setDescription(const QString &description) {
  m_description = description;
}

FeedsModelStandardFeed *FeedsModelStandardFeed::loadFromRecord(const QSqlRecord &record) {
  FeedsModelStandardFeed *feed = new FeedsModelStandardFeed(NULL);

  feed->setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  feed->setId(record.value(FDS_DB_ID_INDEX).toInt());
  feed->setDescription(record.value(FDS_DB_DESCRIPTION_INDEX).toString());
  feed->setCreationDate(QDateTime::fromString(record.value(FDS_DB_DCREATED_INDEX).toString(),
                                              Qt::ISODate));
  feed->setIcon(IconFactory::fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  feed->setEncoding(record.value(FDS_DB_ENCODING_INDEX).toString());
  feed->setUrl(record.value(FDS_DB_URL_INDEX).toString());
  feed->setLanguage(record.value(FDS_DB_LANGUAGE_INDEX).toString());
  feed->updateCounts();

  return feed;
}

QDateTime FeedsModelStandardFeed::creationDate() const {
  return m_creationDate;
}

void FeedsModelStandardFeed::setCreationDate(const QDateTime &creation_date) {
  m_creationDate = creation_date;
}

QString FeedsModelStandardFeed::encoding() const {
  return m_encoding;
}

void FeedsModelStandardFeed::setEncoding(const QString &encoding) {
  m_encoding = encoding;
}

QString FeedsModelStandardFeed::url() const {
  return m_url;
}

void FeedsModelStandardFeed::setUrl(const QString &url) {
  m_url = url;
}

QString FeedsModelStandardFeed::language() const {
  return m_language;
}

void FeedsModelStandardFeed::setLanguage(const QString &language) {
  m_language = language;
}

QString FeedsModelStandardFeed::description() const {
  return m_description;
}

QVariant FeedsModelStandardFeed::data(int column, int role) const {
  switch (role) {
    case Qt::DisplayRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return m_title;
      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return QString("(%1)").arg(QString::number(countOfUnreadMessages()));
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
        return m_icon.isNull() ?
              IconThemeFactory::getInstance()->fromTheme("application-rss+xml") :
              m_icon;
      }
      else {
        return QVariant();
      }

    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return QObject::tr("%1\n\n"
                           "Feed type: %2\n"
                           "URL: %3\n"
                           "Encoding: %4\n"
                           "Language: %5").arg(m_title,
                                               FeedsModelFeed::typeToString(m_type),
                                               m_url,
                                               m_encoding,
                                               m_language.isEmpty() ?
                                                 "-" :
                                                 m_language);      }
      else if (column == FDS_MODEL_COUNTS_INDEX) {
        return QObject::tr("%n unread message(s).", "", countOfUnreadMessages());
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
  int download_timeout =  Settings::getInstance()->value(APP_CFG_FEEDS,
                                                         "download_timeout",
                                                         5000).toInt();

  QNetworkReply::NetworkError download_result = NetworkFactory::downloadFile(url(),
                                                                             download_timeout,
                                                                             feed_contents);

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
    case FeedsModelFeed::StandardRss2X:
      messages = ParsingFactory::parseAsRSS20(formatted_feed_contents);
      break;

      // TODO: Add support for other standard formats.

    default:
      break;
  }

  updateMessages(messages);
}

void FeedsModelStandardFeed::updateMessages(const QList<Message> &messages) {
  int feed_id = id(), message_id;
  QSqlDatabase database = DatabaseFactory::getInstance()->addConnection("FeedsModelStandardFeed");

  // Prepare queries.
  QSqlQuery query_select(database);
  QSqlQuery query_insert(database);

  query_select.prepare("SELECT id, feed, date_created FROM Messages "
                       "WHERE feed = :feed AND title = :title AND url = :url;");

  query_insert.prepare("INSERT INTO Messages "
                       "(feed, title, url, author, date_created, contents) "
                       "VALUES (:feed, :title, :url, :author, :date_created, :contents);");

  foreach (const Message &message, messages) {
    query_select.bindValue(":feed", feed_id);
    query_select.bindValue(":title", message.m_title);
    query_select.bindValue(":url", message.m_url);
    query_select.exec();

    if (query_select.next()) {
      // Message with this title & url probably exists in current feed.
      message_id = query_select.value(0).toInt();
    }
    else {
      message_id = -1;
    }

    query_select.finish();

    if (message_id == -1) {
      // Message is not fetched in this feed yet. Add it.
      query_insert.bindValue(":feed", feed_id);
      query_insert.bindValue(":title", message.m_title);
      query_insert.bindValue(":url", message.m_url);
      query_insert.bindValue(":author", message.m_author);
      query_insert.bindValue(":date_created", message.m_created.toString(Qt::ISODate));
      query_insert.bindValue(":contents", message.m_contents);

      query_insert.exec();
      query_insert.finish();
    }
    else {
      // Message is already persistently stored.
      // TODO: Update message if it got updated in the
      // online feed.
    }
  }
}
