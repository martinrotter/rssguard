// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/simplecrypt/simplecrypt.h"
#include "network-web/networkfactory.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "services/abstract/recyclebin.h"
#include "services/standard/gui/formstandardfeeddetails.h"
#include "services/standard/standardserviceroot.h"

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
  m_encoding = QString();
}

StandardFeed::StandardFeed(const StandardFeed &other)
  : Feed(NULL) {
  m_passwordProtected = other.passwordProtected();
  m_username = other.username();
  m_password = other.password();
  m_networkError = other.networkError();
  m_type = other.type();
  m_encoding = other.encoding();

  setCountOfAllMessages(other.countOfAllMessages());
  setCountOfUnreadMessages(other.countOfUnreadMessages());

  setUrl(other.url());
  setStatus(other.status());
  setAutoUpdateType(other.autoUpdateType());
  setAutoUpdateInitialInterval(other.autoUpdateInitialInterval());
  setAutoUpdateRemainingInterval(other.autoUpdateRemainingInterval());

  setTitle(other.title());
  setId(other.id());
  setCustomId(other.customId());
  setIcon(other.icon());
  setChildItems(other.childItems());
  setParent(other.parent());
  setCreationDate(other.creationDate());
  setDescription(other.description());
}

StandardFeed::~StandardFeed() {
  qDebug("Destroying Feed instance.");
}

QList<QAction*> StandardFeed::contextMenu() {
  return serviceRoot()->getContextMenuForFeed(this);
}

StandardServiceRoot *StandardFeed::serviceRoot() const {
  return qobject_cast<StandardServiceRoot*>(getParentServiceRoot());
}

bool StandardFeed::editViaGui() {
  QScopedPointer<FormStandardFeedDetails> form_pointer(new FormStandardFeedDetails(serviceRoot(), qApp->mainForm()));
  form_pointer.data()->exec(this, NULL);
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
      else {
        return Feed::data(column, role);
      }

    default:
      return Feed::data(column, role);
  }
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

QPair<StandardFeed*,QNetworkReply::NetworkError> StandardFeed::guessFeed(const QString &url,
                                                                         const QString &username,
                                                                         const QString &password) {
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

bool StandardFeed::removeItself() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  return DatabaseQueries::deleteFeed(database, customId(), getParentServiceRoot()->accountId());
}

bool StandardFeed::addItself(RootItem *parent) {
  // Now, add feed to persistent storage.
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  bool ok;
  int new_id = DatabaseQueries::addFeed(database, parent->id(), parent->getParentServiceRoot()->accountId(), title(),
                                        description(), creationDate(), icon(), encoding(), url(), passwordProtected(),
                                        username(), password(), autoUpdateType(), autoUpdateInitialInterval(), type(), &ok);

  if (!ok) {
    // Query failed.
    return false;
  }
  else {
    // New feed was added, fetch is primary id from the database.
    setId(new_id);
    setCustomId(new_id);

    return true;
  }
}

bool StandardFeed::editItself(StandardFeed *new_feed_data) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  StandardFeed *original_feed = this;
  RootItem *new_parent = new_feed_data->parent();

  if (!DatabaseQueries::editFeed(database, new_parent->id(), original_feed->id(), new_feed_data->title(),
                                 new_feed_data->description(), new_feed_data->icon(),
                                 new_feed_data->encoding(), new_feed_data->url(), new_feed_data->passwordProtected(),
                                 new_feed_data->username(), new_feed_data->password(),
                                 new_feed_data->autoUpdateType(), new_feed_data->autoUpdateInitialInterval(),
                                 new_feed_data->type())) {
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

QList<Message> StandardFeed::obtainNewMessages() {
  QByteArray feed_contents;
  int download_timeout = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateTimeout)).toInt();
  m_networkError = NetworkFactory::downloadFeedFile(url(), download_timeout, feed_contents,
                                                    passwordProtected(), username(), password()).first;

  if (m_networkError != QNetworkReply::NoError) {
    qWarning("Error during fetching of new messages for feed '%s' (id %d).", qPrintable(url()), id());
    setStatus(Error);
    return QList<Message>();
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

  return messages;
}

QNetworkReply::NetworkError StandardFeed::networkError() const {
  return m_networkError;
}

StandardFeed::StandardFeed(const QSqlRecord &record) : Feed(NULL) {
  setTitle(record.value(FDS_DB_TITLE_INDEX).toString());
  setId(record.value(FDS_DB_ID_INDEX).toInt());
  setCustomId(id());
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

  setAutoUpdateType(static_cast<Feed::AutoUpdateType>(record.value(FDS_DB_UPDATE_TYPE_INDEX).toInt()));
  setAutoUpdateInitialInterval(record.value(FDS_DB_UPDATE_INTERVAL_INDEX).toInt());
}
