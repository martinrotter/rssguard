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

#include "core/messagesmodel.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iconfactory.h"

#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>


MessagesModel::MessagesModel(QObject *parent)
  : QSqlTableModel(parent, qApp->database()->connection("MessagesModel", DatabaseFactory::FromSettings)),
    m_messageMode(MessagesFromFeeds), m_messageFilter(NoHighlighting) {
  setObjectName("MessagesModel");
  setupFonts();
  setupIcons();
  setupHeaderData();

  // Set desired table and edit strategy.
  // NOTE: Changes to the database are actually NOT submitted
  // via model, but via DIRECT SQL calls are used to do persistent messages.
  setEditStrategy(QSqlTableModel::OnManualSubmit);
  setTable("Messages");
  loadMessages(QList<int>());
}

MessagesModel::~MessagesModel() {
  qDebug("Destroying MessagesModel instance.");
}

void MessagesModel::setupIcons() {
  m_favoriteIcon = qApp->icons()->fromTheme("mail-mark-favorite");
  m_readIcon = qApp->icons()->fromTheme("mail-mark-read");
  m_unreadIcon = qApp->icons()->fromTheme("mail-mark-unread");
}

void MessagesModel::fetchAll() {
  while (canFetchMore()) {
    fetchMore();
  }
}

void MessagesModel::setupFonts() {
  m_normalFont = Application::font("MessagesView");
  m_boldFont = m_normalFont;
  m_boldFont.setBold(true);
}

void MessagesModel::loadMessages(const QList<int> feed_ids) { 
  m_currentFeeds = feed_ids;

  if (feed_ids.size() == 1 && feed_ids[0] == ID_RECYCLE_BIN) {
    m_messageMode = MessagesFromRecycleBin;
    setFilter("is_deleted = 1");
  }
  else {
    m_messageMode = MessagesFromFeeds;
    QString assembled_ids = textualFeeds().join(", ");

    setFilter(QString("feed IN (%1) AND is_deleted = 0").arg(assembled_ids));
    qDebug("Loading messages from feeds: %s.", qPrintable(assembled_ids));
  }

  select();
  fetchAll();
}

void MessagesModel::filterMessages(MessagesModel::MessageFilter filter) {
  m_messageFilter = filter;
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

QStringList MessagesModel::textualFeeds() const {
  QStringList stringy_ids;
  stringy_ids.reserve(m_currentFeeds.size());

  foreach (int feed_id, m_currentFeeds) {
    stringy_ids.append(QString::number(feed_id));
  }

  return stringy_ids;
}

int MessagesModel::messageId(int row_index) const {
  return data(row_index, MSG_DB_ID_INDEX, Qt::EditRole).toInt();
}

Message MessagesModel::messageAt(int row_index) const {
  QSqlRecord rec = record(row_index);
  Message message;

  // Fill Message object with details.
  message.m_author = rec.value(MSG_DB_AUTHOR_INDEX).toString();
  message.m_contents = rec.value(MSG_DB_CONTENTS_INDEX).toString();
  message.m_title = rec.value(MSG_DB_TITLE_INDEX).toString();
  message.m_url = rec.value(MSG_DB_URL_INDEX).toString();
  message.m_created = TextFactory::parseDateTime(rec.value(MSG_DB_DCREATED_INDEX).value<qint64>()).toLocalTime();

  return message;
}

void MessagesModel::setupHeaderData() {
  m_headerData << /*: Tooltip for ID of message.*/ tr("Id") <<
                  /*: Tooltip for "read" column in msg list.*/ tr("Read") <<
                  /*: Tooltip for "deleted" column in msg list.*/ tr("Deleted") <<
                  /*: Tooltip for "important" column in msg list.*/ tr("Important") <<
                  /*: Tooltip for name of feed for message.*/ tr("Feed") <<
                  /*: Tooltip for title of message.*/ tr("Title") <<
                  /*: Tooltip for url of message.*/ tr("Url") <<
                  /*: Tooltip for author of message.*/ tr("Author") <<
                  /*: Tooltip for creation date of message.*/ tr("Created on") <<
                  /*: Tooltip for contents of message.*/ tr("Contents");

  m_tooltipData << tr("Id of the message.") << tr("Is message read?") <<
                   tr("Is message deleted?") << tr("Is message important?") <<
                   tr("Id of feed which this message belongs to.") <<
                   tr("Title of the message.") << tr("Url of the message.") <<
                   tr("Author of the message.") << tr("Creation date of the message.") <<
                   tr("Contents of the message.");
}

Qt::ItemFlags MessagesModel::flags(const QModelIndex &index) const {
  Q_UNUSED(index)

  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
}

QVariant MessagesModel::data(int row, int column, int role) const {
  return data(index(row, column), role);
}

QVariant MessagesModel::data(const QModelIndex &idx, int role) const {
  switch (role) {
    // Human readable data for viewing.
    case Qt::DisplayRole: {
      int index_column = idx.column();

      if (index_column == MSG_DB_DCREATED_INDEX) {
        return TextFactory::parseDateTime(QSqlTableModel::data(idx,
                                                               role).value<qint64>()).toLocalTime().toString(Qt::DefaultLocaleShortDate);
      }
      else if (index_column == MSG_DB_AUTHOR_INDEX) {
        QString author_name = QSqlTableModel::data(idx, role).toString();

        return author_name.isEmpty() ? "-" : author_name;
      }
      /*
      else if (index_column == MSG_DB_ID_INDEX) {
        return QSqlTableModel::data(index(idx.row(), MSG_DB_TITLE_INDEX, idx.parent()));
      }
      */
      else if (index_column != MSG_DB_IMPORTANT_INDEX && index_column != MSG_DB_READ_INDEX) {
        return QSqlTableModel::data(idx, role);
      }
      else {
        return QVariant();
      }
    }

    case Qt::EditRole:
      return QSqlTableModel::data(idx, role);

    case Qt::FontRole:
      return QSqlTableModel::data(index(idx.row(), MSG_DB_READ_INDEX)).toInt() == 1 ? m_normalFont : m_boldFont;

    case Qt::ForegroundRole:
      switch (m_messageFilter) {
        case HighlightImportant:
          return QSqlTableModel::data(index(idx.row(), MSG_DB_IMPORTANT_INDEX)).toInt() == 1 ? QColor(Qt::blue) : QVariant();

        case HighlightUnread:
          return QSqlTableModel::data(index(idx.row(), MSG_DB_READ_INDEX)).toInt() == 0 ? QColor(Qt::blue) : QVariant();

        case NoHighlighting:
        default:
          return QVariant();
      }

    case Qt::DecorationRole: {
      int index_column = idx.column();

      if (index_column == MSG_DB_READ_INDEX) {
        return QSqlTableModel::data(idx).toInt() == 1 ? m_readIcon : m_unreadIcon;
      }
      else if (index_column == MSG_DB_IMPORTANT_INDEX) {
        return QSqlTableModel::data(idx).toInt() == 1 ? m_favoriteIcon : QVariant();
      }
      else {
        return QVariant();
      }
    }

    default:
      return QVariant();
  }
}

bool MessagesModel::setMessageRead(int row_index, int read) {  
  if (data(row_index, MSG_DB_READ_INDEX, Qt::EditRole).toInt() == read) {
    // Read status is the same is the one currently set.
    // In that case, no extra work is needed.
    return true;
  }

  QSqlDatabase db_handle = database();

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for message read change.");
    return false;
  }

  // Rewrite "visible" data in the model.
  bool working_change = setData(index(row_index, MSG_DB_READ_INDEX), read);

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebug("Setting of new data to the model failed for message read change.");

    db_handle.rollback();
    return false;
  }

  int message_id;
  QSqlQuery query_read_msg(db_handle);
  query_read_msg.setForwardOnly(true);

  if (!query_read_msg.prepare("UPDATE Messages SET is_read = :read WHERE id = :id;")) {
    qWarning("Query preparation failed for message read change.");

    db_handle.rollback();
    return false;
  }

  // Rewrite the actual data in the database itself.
  message_id = messageId(row_index);
  query_read_msg.bindValue(":id", message_id);
  query_read_msg.bindValue(":read", read);
  query_read_msg.exec();

  // Commit changes.
  if (db_handle.commit()) {
    // If commit succeeded, then emit changes, so that view
    // can reflect.
    emit dataChanged(index(row_index, 0),
                     index(row_index, columnCount() - 1));
    emit feedCountsChanged(false);
    return true;
  }
  else {
    return db_handle.rollback();;
  }
}

bool MessagesModel::switchMessageImportance(int row_index) {
  QSqlDatabase db_handle = database();

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for message importance switch failed.");
    return false;
  }

  QModelIndex target_index = index(row_index, MSG_DB_IMPORTANT_INDEX);
  int current_importance = data(target_index, Qt::EditRole).toInt();

  // Rewrite "visible" data in the model.
  bool working_change = current_importance == 1 ?
                          setData(target_index, 0) :
                          setData(target_index, 1);

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebug("Setting of new data to the model failed for message importance change.");

    db_handle.rollback();
    return false;
  }

  int message_id;
  QSqlQuery query_importance_msg(db_handle);
  query_importance_msg.setForwardOnly(true);

  if (!query_importance_msg.prepare("UPDATE Messages SET is_important = :important WHERE id = :id;")) {
    qWarning("Query preparation failed for message importance switch.");

    db_handle.rollback();
    return false;
  }

  message_id = messageId(row_index);
  query_importance_msg.bindValue(":id", message_id);
  query_importance_msg.bindValue(":important", current_importance == 1 ? 0 : 1);
  query_importance_msg.exec();

  // Commit changes.
  if (db_handle.commit()) {
    // If commit succeeded, then emit changes, so that view
    // can reflect.
    emit dataChanged(index(row_index, 0), index(row_index, columnCount() - 1));
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

bool MessagesModel::switchBatchMessageImportance(const QModelIndexList &messages) {
  QSqlDatabase db_handle = database();
  QSqlQuery query_read_msg(db_handle);
  QStringList message_ids;

  query_read_msg.setForwardOnly(true);

  // Obtain IDs of all desired messages.
  foreach (const QModelIndex &message, messages) {
    message_ids.append(QString::number(messageId(message.row())));
  }

  if (query_read_msg.exec(QString("UPDATE Messages SET is_important = NOT is_important "
                                  "WHERE id IN (%1);").arg(message_ids.join(", ")))) {
    select();
    fetchAll();

    //emit feedCountsChanged(false);
    return true;
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesDeleted(const QModelIndexList &messages, int deleted) {
  QSqlDatabase db_handle = database();
  QSqlQuery query_read_msg(db_handle);
  QStringList message_ids;

  query_read_msg.setForwardOnly(true);

  // Obtain IDs of all desired messages.
  foreach (const QModelIndex &message, messages) {
    message_ids.append(QString::number(messageId(message.row())));
  }

  QString sql_delete_query;

  if (m_messageMode == MessagesFromFeeds) {
    sql_delete_query = QString("UPDATE Messages SET is_deleted = %2 WHERE id IN (%1);").arg(message_ids.join(", "),
                                                                                            QString::number(deleted));
  }
  else {
    sql_delete_query = QString("DELETE FROM Messages WHERE id in (%1);").arg(message_ids.join(", "));
  }

  if (query_read_msg.exec(sql_delete_query)) {
    select();
    fetchAll();

    emit feedCountsChanged(true);
    return true;
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesRead(const QModelIndexList &messages, int read) {
  QSqlDatabase db_handle = database();
  QSqlQuery query_read_msg(db_handle);
  QStringList message_ids;

  query_read_msg.setForwardOnly(true);

  // Obtain IDs of all desired messages.
  foreach (const QModelIndex &message, messages) {
    message_ids.append(QString::number(messageId(message.row())));
  }

  if (query_read_msg.exec(QString("UPDATE Messages SET is_read = %2 "
                                  "WHERE id IN (%1);").arg(message_ids.join(", "), QString::number(read)))) {
    select();
    fetchAll();

    emit feedCountsChanged(true);
    return true;
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesRestored(const QModelIndexList &messages) {
  // TODO: Model -> setBatchMessagesRestored();
  // obnovime zpravy, po obnoveni je treba jako ve funkci setBatchMessagesDeleted
  // pres feedCountsChanged dat informaci ze pocty zprav se zmenily, ale oni
  // se zmenily nejen ve vybranych kanalech (je vybran odkadkovy kos) ale v kanalech do kterych patri
  return true;
}

QVariant MessagesModel::headerData(int section, Qt::Orientation orientation, int role) const {
  Q_UNUSED(orientation)

  switch (role) {
    case Qt::DisplayRole:
      // Display textual headers for all columns except "read" and
      // "important" columns.
      if (section != MSG_DB_READ_INDEX && section != MSG_DB_IMPORTANT_INDEX) {
        return m_headerData.at(section);
      }
      else {
        return QVariant();
      }

    case Qt::ToolTipRole:
      return m_tooltipData.at(section);

    case Qt::EditRole:
      return m_headerData.at(section);

      // Display icons for "read" and "important" columns.
    case Qt::DecorationRole: {
      switch (section) {
        case MSG_DB_READ_INDEX:
          return m_readIcon;

        case MSG_DB_IMPORTANT_INDEX:
          return m_favoriteIcon;

        default:
          return QVariant();
      }
    }

    default:
      return QVariant();
  }
}
