// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/standard/standardfeed.h"

#include "definitions/definitions.h"
#include "core/parsingfactory.h"
#include "core/feedsmodel.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/simplecrypt/simplecrypt.h"
#include "network-web/networkfactory.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "services/standard/standardserviceroot.h"
#include "services/standard/gui/formstandardfeeddetails.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QTextCodec>
#include <QPointer>
#include <QDomDocument>
#include <QDomNode>
#include <QDomElement>
#include <QXmlStreamReader>


StandardFeed::StandardFeed(RootItem *parent_item)
  : Feed(parent_item) {
  m_passwordProtected = false;
  m_username = QString();
  m_password = QString();
  m_networkError = QNetworkReply::NoError;
  m_type = Rss0X;
  m_totalCount = 0;
  m_unreadCount = 0;
  m_encoding = QString();
  m_url = QString();

  setKind(RootItemKind::Feed);
}

StandardFeed::StandardFeed(const StandardFeed &other)
  : Feed(NULL) {
  m_passwordProtected = other.passwordProtected();
  m_username = other.username();
  m_password = other.password();
  m_networkError = other.networkError();
  m_type = other.type();
  m_totalCount = other.countOfAllMessages();
  m_unreadCount = other.countOfUnreadMessages();
  m_encoding = other.encoding();
  m_url = other.url();

  setStatus(other.status());
  setAutoUpdateType(other.autoUpdateType());
  setAutoUpdateInitialInterval(other.autoUpdateInitialInterval());
  setAutoUpdateRemainingInterval(other.autoUpdateRemainingInterval());

  setKind(RootItemKind::Feed);
  setTitle(other.title());
  setId(other.id());
  setIcon(other.icon());
  setChildItems(other.childItems());
  setParent(other.parent());
  setCreationDate(other.creationDate());
  setDescription(other.description());
}

StandardFeed::~StandardFeed() {
  qDebug("Destroying Feed instance.");
}

int StandardFeed::countOfAllMessages() const {
  return m_totalCount;
}

int StandardFeed::countOfUnreadMessages() const {
  return m_unreadCount;
}

QList<QAction*> StandardFeed::contextMenuActions() {
  return serviceRoot()->getContextMenuForFeed(this);
}

StandardServiceRoot *StandardFeed::serviceRoot() {
  return static_cast<StandardServiceRoot*>(getParentServiceRoot());
}

bool StandardFeed::editViaGui() {
  QPointer<FormStandardFeedDetails> form_pointer = new FormStandardFeedDetails(serviceRoot(), qApp->mainForm());

  form_pointer.data()->exec(this, NULL);
  delete form_pointer.data();
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

bool StandardFeed::markAsReadUnread(ReadStatus status) {
  return serviceRoot()->markFeedsReadUnread(QList<Feed*>() << this, status);
}

bool StandardFeed::cleanMessages(bool clean_read_only) {
  return serviceRoot()->cleanFeeds(QList<Feed*>() << this, clean_read_only);
}

QList<Message> StandardFeed::undeletedMessages() const {
  QList<Message> messages;

  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_read_msg(database);
  query_read_msg.setForwardOnly(true);
  query_read_msg.prepare("SELECT title, url, author, date_created, contents "
                         "FROM Messages "
                         "WHERE is_deleted = 0 AND feed = :feed AND account_id = :account_id;");

  query_read_msg.bindValue(QSL(":feed"), id());
  query_read_msg.bindValue(QSL(":account_id"), const_cast<StandardFeed*>(this)->serviceRoot()->accountId());

  // FIXME: Fix those const functions, this is fucking ugly.

  if (query_read_msg.exec()) {
    while (query_read_msg.next()) {
      Message message;

      message.m_feedId = id();
      message.m_title = query_read_msg.value(0).toString();
      message.m_url = query_read_msg.value(1).toString();
      message.m_author = query_read_msg.value(2).toString();
      message.m_created = TextFactory::parseDateTime(query_read_msg.value(3).value<qint64>());
      message.m_contents = query_read_msg.value(4).toString();

      messages.append(message);
    }
  }

  return messages;
}

QString StandardFeed::typeToString(StandardFeed::Type type) {
  switch (type) {
    case Atom10:
      return QSL("ATOM 1.0");

    case Rdf:
      return QSL("RDF (RSS 1.0)");

    case Rss0X:
      return QSL("RSS 0.91/0.92/0.93");

    case Rss2X:
    default:
      return QSL("RSS 2.0/2.0.1");
  }
}

void StandardFeed::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_all(database);

  query_all.setForwardOnly(true);

  if (including_total_count) {
    if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE feed = %1 AND is_deleted = 0 AND account_id = %2;").arg(QString::number(id()),
                                                                                                                            QString::number(const_cast<StandardFeed*>(this)->serviceRoot()->accountId()))) && query_all.next()) {
      m_totalCount = query_all.value(0).toInt();
    }
  }

  // Obtain count of unread messages.
  if (query_all.exec(QString("SELECT count(*) FROM Messages WHERE feed = %1 AND is_deleted = 0 AND is_read = 0 AND account_id = %2;").arg(QString::number(id()),
                                                                                                                                          QString::number(const_cast<StandardFeed*>(this)->serviceRoot()->accountId()))) && query_all.next()) {
    int new_unread_count = query_all.value(0).toInt();

    if (status() == NewMessages && new_unread_count < m_unreadCount) {
      setStatus(Normal);
    }

    m_unreadCount = new_unread_count;
  }
}

void StandardFeed::fetchMetadataForItself() {
  QPair<StandardFeed*,QNetworkReply::NetworkError> metadata = guessFeed(url(), username(), password());

  if (metadata.first != NULL && metadata.second == QNetworkReply::NoError) {
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

QPair<StandardFeed*,QNetworkReply::NetworkError> StandardFeed::guessFeed(const QString &url, const QString &username, const QString &password) {
  QPair<StandardFeed*,QNetworkReply::NetworkError> result; result.first = NULL;

  QByteArray feed_contents;
  NetworkResult network_result = NetworkFactory::downloadFeedFile(url,
                                                                  qApp->settings()->value(GROUP(Feeds),
                                                                                          SETTING(Feeds::UpdateTimeout)).toInt(),
                                                                  feed_contents,
                                                                  !username.isEmpty(),
                                                                  username,
                                                                  password);
  result.second = network_result.first;

  if (result.second == QNetworkReply::NoError) {
    // Feed XML was obtained, now we need to try to guess
    // its encoding before we can read further data.
    QString xml_schema_encoding;
    QString xml_contents_encoded;
    QRegExp encoding_rexp(QSL("encoding=\"[^\"]\\S+\""));

    if (encoding_rexp.indexIn(feed_contents) != -1 &&
        !(xml_schema_encoding = encoding_rexp.cap(0)).isEmpty()) {
      // Some "encoding" attribute was found get the encoding
      // out of it.
      encoding_rexp.setPattern(QSL("[^\"]\\S+[^\"]"));
      encoding_rexp.indexIn(xml_schema_encoding, 9);
      xml_schema_encoding = encoding_rexp.cap(0);
    }

    if (result.first == NULL) {
      result.first = new StandardFeed();
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
    QList<QString> icon_possible_locations;

    icon_possible_locations.append(url);

    if (root_tag_name == QL1S("rdf:RDF")) {
      // We found RDF feed.
      QDomElement channel_element = root_element.namedItem(QSL("channel")).toElement();

      result.first->setType(Rdf);
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
        result.first->setType(Rss0X);
      }
      else {
        result.first->setType(Rss2X);
      }

      QDomElement channel_element = root_element.namedItem(QSL("channel")).toElement();

      result.first->setTitle(channel_element.namedItem(QSL("title")).toElement().text());
      result.first->setDescription(channel_element.namedItem(QSL("description")).toElement().text());

      QString source_link = channel_element.namedItem(QSL("link")).toElement().text();

      if (!source_link.isEmpty()) {
        icon_possible_locations.prepend(source_link);
      }
    }
    else if (root_tag_name == QL1S("feed")) {
      // We found ATOM feed.
      result.first->setType(Atom10);
      result.first->setTitle(root_element.namedItem(QSL("title")).toElement().text());
      result.first->setDescription(root_element.namedItem(QSL("subtitle")).toElement().text());

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

QVariant StandardFeed::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        QString auto_update_string;

        switch (autoUpdateType()) {
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
                                    autoUpdateRemainingInterval());
            break;
        }

        //: Tooltip for feed.
        return tr("%1 (%2)"
                  "%3\n\n"
                  "Network status: %6\n"
                  "Encoding: %4\n"
                  "Auto-update status: %5").arg(title(),
                                                StandardFeed::typeToString(type()),
                                                description().isEmpty() ? QString() : QString('\n') + description(),
                                                encoding(),
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

    case Qt::ForegroundRole:
      switch (status()) {
        case NewMessages:
          return QColor(Qt::blue);

        case NetworkError:
          return QColor(Qt::red);

        default:
          return QVariant();
      }

    default:
      return Feed::data(column, role);
  }
}

Qt::ItemFlags StandardFeed::additionalFlags() const {
  return Qt::ItemIsDragEnabled;
}

bool StandardFeed::performDragDropChange(RootItem *target_item) {
  StandardFeed *feed_new = new StandardFeed(*this);
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

int StandardFeed::update() {
  QByteArray feed_contents;
  int download_timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  m_networkError = NetworkFactory::downloadFeedFile(url(), download_timeout, feed_contents,
                                                    passwordProtected(), username(), password()).first;

  if (m_networkError != QNetworkReply::NoError) {
    qWarning("Error during fetching of new messages for feed '%s' (id %d).", qPrintable(url()), id());
    setStatus(NetworkError);
    return 0;
  }
  else if (status() != NewMessages) {
    setStatus(Normal);
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
    case StandardFeed::Rss0X:
    case StandardFeed::Rss2X:
      messages = ParsingFactory::parseAsRSS20(formatted_feed_contents);
      break;

    case StandardFeed::Rdf:
      messages = ParsingFactory::parseAsRDF(formatted_feed_contents);
      break;

    case StandardFeed::Atom10:
      messages = ParsingFactory::parseAsATOM10(formatted_feed_contents);

    default:
      break;
  }

  return updateMessages(messages);
}

bool StandardFeed::removeItself() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_remove(database);

  query_remove.setForwardOnly(true);

  // Remove all messages from this standard feed.
  query_remove.prepare(QSL("DELETE FROM Messages WHERE feed = :feed AND account_id = :account_id;"));
  query_remove.bindValue(QSL(":feed"), id());
  query_remove.bindValue(QSL(":account_id"), const_cast<StandardFeed*>(this)->serviceRoot()->accountId());

  if (!query_remove.exec()) {
    return false;
  }

  // Remove feed itself.
  query_remove.prepare(QSL("DELETE FROM Feeds WHERE id = :feed;"));
  query_remove.bindValue(QSL(":feed"), id());

  return query_remove.exec();
}

bool StandardFeed::addItself(RootItem *parent) {
  // Now, add feed to persistent storage.
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_add_feed(database);

  query_add_feed.setForwardOnly(true);
  query_add_feed.prepare("INSERT INTO Feeds "
                         "(title, description, date_created, icon, category, encoding, url, protected, username, password, update_type, update_interval, type) "
                         "VALUES (:title, :description, :date_created, :icon, :category, :encoding, :url, :protected, :username, :password, :update_type, :update_interval, :type);");
  query_add_feed.bindValue(QSL(":title"), title());
  query_add_feed.bindValue(QSL(":description"), description());
  query_add_feed.bindValue(QSL(":date_created"), creationDate().toMSecsSinceEpoch());
  query_add_feed.bindValue(QSL(":icon"), qApp->icons()->toByteArray(icon()));
  query_add_feed.bindValue(QSL(":category"), parent->id());
  query_add_feed.bindValue(QSL(":encoding"), encoding());
  query_add_feed.bindValue(QSL(":url"), url());
  query_add_feed.bindValue(QSL(":protected"), (int) passwordProtected());
  query_add_feed.bindValue(QSL(":username"), username());

  if (password().isEmpty()) {
    query_add_feed.bindValue(QSL(":password"), password());
  }
  else {
    query_add_feed.bindValue(QSL(":password"), TextFactory::encrypt(password()));
  }

  query_add_feed.bindValue(QSL(":update_type"), (int) autoUpdateType());
  query_add_feed.bindValue(QSL(":update_interval"), autoUpdateInitialInterval());
  query_add_feed.bindValue(QSL(":type"), (int) type());

  if (!query_add_feed.exec()) {
    qDebug("Failed to add feed to database: %s.", qPrintable(query_add_feed.lastError().text()));

    // Query failed.
    return false;
  }

  query_add_feed.prepare(QSL("SELECT id FROM Feeds WHERE url = :url;"));
  query_add_feed.bindValue(QSL(":url"), url());
  if (query_add_feed.exec() && query_add_feed.next()) {
    // New feed was added, fetch is primary id from the database.
    setId(query_add_feed.value(0).toInt());
  }
  else {
    // Something failed.
    return false;
  }

  return true;
}

bool StandardFeed::editItself(StandardFeed *new_feed_data) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  QSqlQuery query_update_feed(database);
  StandardFeed *original_feed = this;
  RootItem *new_parent = new_feed_data->parent();

  query_update_feed.setForwardOnly(true);
  query_update_feed.prepare("UPDATE Feeds "
                            "SET title = :title, description = :description, icon = :icon, category = :category, encoding = :encoding, url = :url, protected = :protected, username = :username, password = :password, update_type = :update_type, update_interval = :update_interval, type = :type "
                            "WHERE id = :id;");
  query_update_feed.bindValue(QSL(":title"), new_feed_data->title());
  query_update_feed.bindValue(QSL(":description"), new_feed_data->description());
  query_update_feed.bindValue(QSL(":icon"), qApp->icons()->toByteArray(new_feed_data->icon()));
  query_update_feed.bindValue(QSL(":category"), new_parent->id());
  query_update_feed.bindValue(QSL(":encoding"), new_feed_data->encoding());
  query_update_feed.bindValue(QSL(":url"), new_feed_data->url());
  query_update_feed.bindValue(QSL(":protected"), (int) new_feed_data->passwordProtected());
  query_update_feed.bindValue(QSL(":username"), new_feed_data->username());

  if (password().isEmpty()) {
    query_update_feed.bindValue(QSL(":password"), new_feed_data->password());
  }
  else {
    query_update_feed.bindValue(QSL(":password"), TextFactory::encrypt(new_feed_data->password()));
  }

  query_update_feed.bindValue(QSL(":update_type"), (int) new_feed_data->autoUpdateType());
  query_update_feed.bindValue(QSL(":update_interval"), new_feed_data->autoUpdateInitialInterval());
  query_update_feed.bindValue(QSL(":type"), new_feed_data->type());
  query_update_feed.bindValue(QSL(":id"), original_feed->id());

  if (!query_update_feed.exec()) {
    // Persistent storage update failed, no way to continue now.
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

int StandardFeed::updateMessages(const QList<Message> &messages) {
  int feed_id = id();
  int updated_messages = 0;
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  bool remove_duplicates = qApp->settings()->value(GROUP(Messages), SETTING(Messages::RemoveDuplicates)).toBool();
  int account_id = serviceRoot()->accountId();

  // Prepare queries.
  QSqlQuery query_select(database);
  QSqlQuery query_update(database);
  QSqlQuery query_insert(database);

  // Used to check if given feed contains any message with given title, url and date_created.
  // WARNING: One feed CANNOT contain two (or more) messages with same AUTHOR AND TITLE AND URL AND DATE_CREATED.
  query_select.setForwardOnly(true);
  query_select.prepare("SELECT id, feed, date_created FROM Messages "
                       "WHERE feed = :feed AND title = :title AND url = :url AND author = :author AND account_id = :account_id;");

  // Used to insert new messages.
  query_insert.setForwardOnly(true);
  query_insert.prepare("INSERT INTO Messages "
                       "(feed, title, url, author, date_created, contents, enclosures, account_id) "
                       "VALUES (:feed, :title, :url, :author, :date_created, :contents, :enclosures, :account_id);");

  if (remove_duplicates) {
    query_update.setForwardOnly(true);
    query_update.prepare(QSL("UPDATE Messages SET contents = :contents enclosures = :enclosures WHERE id = :id;"));
  }

  if (!database.transaction()) {
    database.rollback();
    qDebug("Transaction start for message downloader failed.");
    return updated_messages;
  }

  foreach (Message message, messages) {
    // Check if messages contain relative URLs and if they do, then replace them.
    if (message.m_url.startsWith(QL1S("/"))) {
      QString new_message_url = url();
      int last_slash = new_message_url.lastIndexOf(QL1S("/"));

      if (last_slash >= 0) {
        new_message_url = new_message_url.left(last_slash);
      }

      new_message_url += message.m_url;
      message.m_url = new_message_url;
    }

    query_select.bindValue(QSL(":feed"), feed_id);
    query_select.bindValue(QSL(":title"), message.m_title);
    query_select.bindValue(QSL(":url"), message.m_url);
    query_select.bindValue(QSL(":author"), message.m_author);
    query_select.bindValue(QSL(":account_id"), account_id);
    query_select.exec();

    QList<qint64> datetime_stamps;
    QList<int> ids;

    while (query_select.next()) {
      ids << query_select.value(0).toInt();
      datetime_stamps << query_select.value(2).value<qint64>();
    }

    query_select.finish();

    if (datetime_stamps.isEmpty()) {
      // Message is not fetched in this feed yet.
      query_insert.bindValue(QSL(":feed"), feed_id);
      query_insert.bindValue(QSL(":title"), message.m_title);
      query_insert.bindValue(QSL(":url"), message.m_url);
      query_insert.bindValue(QSL(":author"), message.m_author);
      query_insert.bindValue(QSL(":date_created"), message.m_created.toMSecsSinceEpoch());
      query_insert.bindValue(QSL(":contents"), message.m_contents);
      query_insert.bindValue(QSL(":enclosures"), Enclosures::encodeEnclosuresToString(message.m_enclosures));
      query_insert.bindValue(QSL(":account_id"), account_id);

      if (query_insert.exec() && query_insert.numRowsAffected() == 1) {
        setStatus(NewMessages);
        updated_messages++;
      }

      query_insert.finish();

      qDebug("Adding new message '%s' to DB.", qPrintable(message.m_title));
    }
    else if (message.m_createdFromFeed && !datetime_stamps.contains(message.m_created.toMSecsSinceEpoch())) {
      if (remove_duplicates && datetime_stamps.size() == 1) {
        // Message is already in feed and new message has new unique time but user wishes to update existing
        // messages and there is exactly ONE existing duplicate.
        query_update.bindValue(QSL(":id"), ids.at(0));
        query_update.bindValue(QSL(":contents"), message.m_contents);
        query_update.bindValue(QSL(":enclosures"), Enclosures::encodeEnclosuresToString(message.m_enclosures));
        query_update.exec();
        query_update.finish();

        qDebug("Updating contents of duplicate message '%s'.", qPrintable(message.m_title));
      }
      else {
        // Message with same title, author and url exists, but new message has new unique time and
        // user does not wish to update duplicates.
        query_insert.bindValue(QSL(":feed"), feed_id);
        query_insert.bindValue(QSL(":title"), message.m_title);
        query_insert.bindValue(QSL(":url"), message.m_url);
        query_insert.bindValue(QSL(":author"), message.m_author);
        query_insert.bindValue(QSL(":date_created"), message.m_created.toMSecsSinceEpoch());
        query_insert.bindValue(QSL(":contents"), message.m_contents);
        query_insert.bindValue(QSL(":enclosures"), Enclosures::encodeEnclosuresToString(message.m_enclosures));

        if (query_insert.exec() && query_insert.numRowsAffected() == 1) {
          setStatus(NewMessages);
          updated_messages++;
        }

        query_insert.finish();

        qDebug("Adding new duplicate (with potentially updated contents) message '%s' to DB.", qPrintable(message.m_title));
      }
    }
  }

  if (!database.commit()) {
    database.rollback();

    qDebug("Transaction commit for message downloader failed.");
  }
  else {
    updateCounts(true);
    serviceRoot()->itemChanged(QList<RootItem*>() << this);
  }

  return updated_messages;
}

QNetworkReply::NetworkError StandardFeed::networkError() const {
  return m_networkError;
}

StandardFeed::StandardFeed(const QSqlRecord &record) : Feed(NULL) {
  setKind(RootItemKind::Feed);
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setDescription(record.value(FDS_DB_DESCRIPTION_INDEX).toString());
  setCreationDate(TextFactory::parseDateTime(record.value(FDS_DB_DCREATED_INDEX).value<qint64>()).toLocalTime());
  setIcon(qApp->icons()->fromByteArray(record.value(FDS_DB_ICON_INDEX).toByteArray()));
  setEncoding(record.value(FDS_DB_ENCODING_INDEX).toString());
  setUrl(record.value(FDS_DB_URL_INDEX).toString());
  setPasswordProtected(record.value(FDS_DB_PROTECTED_INDEX).toBool());
  setUsername(record.value(FDS_DB_USERNAME_INDEX).toString());

  if (record.value(FDS_DB_PASSWORD_INDEX).toString().isEmpty()) {
    setPassword(record.value(FDS_DB_PASSWORD_INDEX).toString());
  }
  else {
    setPassword(TextFactory::decrypt(record.value(FDS_DB_PASSWORD_INDEX).toString()));
  }


  setAutoUpdateType(static_cast<StandardFeed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
}
