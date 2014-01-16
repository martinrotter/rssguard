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


FeedsModelStandardFeed::FeedsModelStandardFeed(FeedsModelRootItem *parent_item)
  : FeedsModelFeed(parent_item) {
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
  feed->setLanguage(record.value(FDS_DB_LANGUAGE_INDEX).toString());
  feed->updateCounts();

  return feed;
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
              IconThemeFactory::instance()->fromTheme("application-rss+xml") :
              m_icon;
      }
      else {
        return QVariant();
      }

    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return QObject::tr("%1 (%2)\n"
                           "%3\n\n"
                           "Encoding: %4\n"
                           "Language: %5").arg(m_title,
                                               FeedsModelFeed::typeToString(m_type),
                                               m_description,
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
  int download_timeout =  Settings::instance()->value(APP_CFG_FEEDS,
                                                      "download_timeout",
                                                      DOWNLOAD_TIMEOUT).toInt();

  // TODO: Provide download time-measures debugging
  // outputs here.
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
    case FeedsModelFeed::StandardRss0X:
    case FeedsModelFeed::StandardRss2X:
      messages = ParsingFactory::parseAsRSS20(formatted_feed_contents);
      break;

    case FeedsModelFeed::StandardRdf:
      messages = ParsingFactory::parseAsRDF(formatted_feed_contents);
      break;

    case FeedsModelFeed::StandardAtom10:
      messages = ParsingFactory::parseAsATOM10(formatted_feed_contents);

      // TODO: Add support for other standard formats.

    default:
      break;
  }

  updateMessages(messages);
}

bool FeedsModelStandardFeed::removeItself() {
  // TODO: pokracovat, vymazat tento standardni
  // kanal z database a smazat jeho zpravy atp.
  return false;
}

void FeedsModelStandardFeed::updateMessages(const QList<Message> &messages) {
  int feed_id = id(), message_id;
  qint64 message_creation_date;
  QSqlDatabase database = DatabaseFactory::instance()->connection();

  // Prepare queries.
  QSqlQuery query_select(database);
  QSqlQuery query_insert(database);
  QSqlQuery query_update(database);

  // Used to check if give feed contains with message with given
  // title and url.
  query_select.setForwardOnly(true);
  query_select.prepare("SELECT id, feed, date_created FROM Messages "
                       "WHERE feed = :feed AND title = :title AND url = :url;");

  // Used to insert new messages.
  query_insert.setForwardOnly(true);
  query_insert.prepare("INSERT INTO Messages "
                       "(feed, title, url, author, date_created, contents) "
                       "VALUES (:feed, :title, :url, :author, :date_created, :contents);");

  // Used to update existing messages of given feed.
  // NOTE: Messages are updated if its creation date
  // is changed.
  query_update.setForwardOnly(true);
  query_update.prepare("UPDATE Messages "
                       "SET title = :title, url = :url, author = :author, "
                       "date_created = :date_created, contents = :contents, "
                       "read = 0, important = 0, deleted = 0 "
                       "WHERE id = :id");

  if (!database.transaction()) {
    database.rollback();

    qDebug("Transaction start for message downloader failed.");
    return;
  }

  foreach (const Message &message, messages) {
    query_select.bindValue(":feed", feed_id);
    query_select.bindValue(":title", message.m_title);
    query_select.bindValue(":url", message.m_url);
    query_select.exec();

    if (query_select.next()) {
      // Message with this title & url probably exists in current feed.
      message_id = query_select.value(0).toInt();
      message_creation_date = query_select.value(2).value<qint64>();
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
      query_insert.bindValue(":date_created", message.m_created.toMSecsSinceEpoch());
      query_insert.bindValue(":contents", message.m_contents);

      query_insert.exec();
      query_insert.finish();
    }
    else if (message.m_createdFromFeed &&
             message_creation_date != 0 &&
             message_creation_date > message.m_created.toMSecsSinceEpoch()) {
      qDebug("Message '%s' (id %d) was updated in the feed, updating too.",
             qPrintable(message.m_title),
             message_id);

      // TODO: Check if this is actually working.

      // Message with given title/url is already persistently
      // stored in given feed.
      // Creation data of the message was obtained from
      // feed itself. We can update this message.
      query_update.bindValue(":title", message.m_title);
      query_update.bindValue(":url", message.m_url);
      query_update.bindValue(":author", message.m_author);
      query_update.bindValue(":date_created", message.m_created.toMSecsSinceEpoch());
      query_update.bindValue(":contents", message.m_contents);
      query_update.bindValue(":id", message_id);

      query_update.exec();
      query_update.finish();
    }
  }

  if (!database.commit()) {
    database.rollback();

    qDebug("Transaction commit for message downloader failed.");
  }
}
