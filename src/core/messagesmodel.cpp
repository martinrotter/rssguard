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

#include "core/messagesmodel.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iconfactory.h"
#include "gui/dialogs/formmain.h"
#include "services/abstract/serviceroot.h"

#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>


MessagesModel::MessagesModel(QObject *parent)
  : QSqlTableModel(parent, qApp->database()->connection(QSL("MessagesModel"), DatabaseFactory::FromSettings)),
    m_messageHighlighter(NoHighlighting), m_customDateFormat(QString()) {
  setObjectName(QSL("MessagesModel"));
  setupFonts();
  setupIcons();
  setupHeaderData();
  updateDateFormat();

  // Set desired table and edit strategy.
  // NOTE: Changes to the database are actually NOT submitted
  // via model, but via DIRECT SQL calls are used to do persistent messages.
  setEditStrategy(QSqlTableModel::OnManualSubmit);
  setTable(QSL("Messages"));
  loadMessages(NULL);
}

MessagesModel::~MessagesModel() {
  qDebug("Destroying MessagesModel instance.");
}

void MessagesModel::setupIcons() {
  m_favoriteIcon = qApp->icons()->fromTheme(QSL("mail-mark-favorite"));
  m_readIcon = qApp->icons()->fromTheme(QSL("mail-mark-read"));
  m_unreadIcon = qApp->icons()->fromTheme(QSL("mail-mark-unread"));
}

void MessagesModel::fetchAllData() {
  select();

  while (canFetchMore()) {
    fetchMore();
  }
}

void MessagesModel::setupFonts() {
  m_normalFont = Application::font("MessagesView");
  m_boldFont = m_normalFont;
  m_boldFont.setBold(true);
}

void MessagesModel::loadMessages(RootItem *item) {
  m_selectedItem = item;

  if (item == NULL) {
    setFilter("true != true");
  }
  else {
    if (!item->getParentServiceRoot()->loadMessagesForItem(item, this)) {
      qWarning("Loading of messages from item '%s' failed.", qPrintable(item->title()));
      qApp->showGuiMessage(tr("Loading of messages from item '%s' failed.").arg(item->title()),
                           tr("Loading of messages failed, maybe messages could not be downloaded."),
                           QSystemTrayIcon::Critical,
                           qApp->mainForm(),
                           true);
    }
  }

  fetchAllData();
}

bool MessagesModel::submitAll() {
  qFatal("Submitting changes via model is not allowed.");
  return false;
}

void MessagesModel::highlightMessages(MessagesModel::MessageHighlighter highlight) {
  m_messageHighlighter = highlight;
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

int MessagesModel::messageId(int row_index) const {
  return data(row_index, MSG_DB_ID_INDEX, Qt::EditRole).toInt();
}

void MessagesModel::updateDateFormat() {
  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::UseCustomDate)).toBool()) {
    m_customDateFormat = qApp->settings()->value(GROUP(Messages), SETTING(Messages::CustomDateFormat)).toString();
  }
  else {
    m_customDateFormat = QString();
  }
}

void MessagesModel::reloadWholeLayout() {
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

Message MessagesModel::messageAt(int row_index) const {
  QSqlRecord rec = record(row_index);
  Message message;

  // Fill Message object with details.
  message.m_author = rec.value(MSG_DB_AUTHOR_INDEX).toString();
  message.m_contents = rec.value(MSG_DB_CONTENTS_INDEX).toString();
  message.m_enclosures = Enclosures::decodeEnclosuresFromString(rec.value(MSG_DB_ENCLOSURES_INDEX).toString());
  message.m_title = rec.value(MSG_DB_TITLE_INDEX).toString();
  message.m_url = rec.value(MSG_DB_URL_INDEX).toString();
  message.m_feedId = rec.value(MSG_DB_FEED_INDEX).toInt();
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
                  /*: Tooltip for contents of message.*/ tr("Contents") <<
                  /*: Tooltip for "pdeleted" column in msg list.*/ tr("Permanently deleted") <<
                  /*: Tooltip for attachments of message.*/ tr("Attachments");

  m_tooltipData << tr("Id of the message.") << tr("Is message read?") <<
                   tr("Is message deleted?") << tr("Is message important?") <<
                   tr("Id of feed which this message belongs to.") <<
                   tr("Title of the message.") << tr("Url of the message.") <<
                   tr("Author of the message.") << tr("Creation date of the message.") <<
                   tr("Contents of the message.") << tr("Is message permanently deleted from recycle bin?") <<
                   tr("List of attachments.");
}

Qt::ItemFlags MessagesModel::flags(const QModelIndex &index) const {
  Q_UNUSED(index)

#if QT_VERSION >= 0x050000
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren;
#else

  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
#endif
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
        if (m_customDateFormat.isEmpty()) {
          return TextFactory::parseDateTime(QSqlTableModel::data(idx,
                                                                 role).value<qint64>()).toLocalTime().toString(Qt::DefaultLocaleShortDate);
        }
        else {
          return TextFactory::parseDateTime(QSqlTableModel::data(idx, role).value<qint64>()).toLocalTime().toString(m_customDateFormat);
        }
      }
      else if (index_column == MSG_DB_AUTHOR_INDEX) {
        QString author_name = QSqlTableModel::data(idx, role).toString();

        return author_name.isEmpty() ? "-" : author_name;
      }
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
      switch (m_messageHighlighter) {
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

  if (!query_read_msg.prepare(QSL("UPDATE Messages SET is_read = :read WHERE id = :id;"))) {
    qWarning("Query preparation failed for message read change.");

    db_handle.rollback();
    return false;
  }

  // Rewrite the actual data in the database itself.
  message_id = messageId(row_index);
  query_read_msg.bindValue(QSL(":id"), message_id);
  query_read_msg.bindValue(QSL(":read"), read);
  query_read_msg.exec();

  // Commit changes.
  if (db_handle.commit()) {
    // If commit succeeded, then emit changes, so that view
    // can reflect.
    emit dataChanged(index(row_index, 0), index(row_index, columnCount() - 1));
    emit messageCountsChanged();

    // TODO: counts changed
    //emit messageCountsChanged(m_selectedItem.mode(), false, false);
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
  bool working_change = current_importance == 1 ? setData(target_index, 0) : setData(target_index, 1);

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebug("Setting of new data to the model failed for message importance change.");

    db_handle.rollback();
    return false;
  }

  int message_id;
  QSqlQuery query_importance_msg(db_handle);
  query_importance_msg.setForwardOnly(true);

  if (!query_importance_msg.prepare(QSL("UPDATE Messages SET is_important = :important WHERE id = :id;"))) {
    qWarning("Query preparation failed for message importance switch.");

    db_handle.rollback();
    return false;
  }

  message_id = messageId(row_index);
  query_importance_msg.bindValue(QSL(":id"), message_id);
  query_importance_msg.bindValue(QSL(":important"), current_importance == 1 ? 0 : 1);
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

  if (query_read_msg.exec(QString(QSL("UPDATE Messages SET is_important = NOT is_important WHERE id IN (%1);"))
                          .arg(message_ids.join(QSL(", "))))) {
    fetchAllData();

    //emit messageCountsChanged(false);
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

  // TODO: todo
  /*
  if (m_selectedItem.mode() == FeedsSelection::MessagesFromFeeds) {
    sql_delete_query = QString(QSL("UPDATE Messages SET is_deleted = %2 WHERE id IN (%1);")).arg(message_ids.join(QSL(", ")),
                                                                                                 QString::number(deleted));
  }
  else {
    sql_delete_query = QString(QSL("UPDATE Messages SET is_pdeleted = %2 WHERE id IN (%1);")).arg(message_ids.join(QSL(", ")),
                                                                                                  QString::number(deleted));
  }
  */

  if (query_read_msg.exec(sql_delete_query)) {
    fetchAllData();
    emit messageCountsChanged();

    // TODO: counts changed
    //emit messageCountsChanged(m_selectedItem.mode(), true, false);
    return true;
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesRead(const QModelIndexList &messages, RootItem::ReadStatus read) {
  QSqlDatabase db_handle = database();
  QSqlQuery query_read_msg(db_handle);
  QStringList message_ids;

  query_read_msg.setForwardOnly(true);

  // Obtain IDs of all desired messages.
  foreach (const QModelIndex &message, messages) {
    message_ids.append(QString::number(messageId(message.row())));
  }

  if (query_read_msg.exec(QString(QSL("UPDATE Messages SET is_read = %2 WHERE id IN (%1);"))
                          .arg(message_ids.join(QSL(", ")),
                               read == RootItem::Read ? QSL("1") : QSL("0")))) {
    fetchAllData();

    emit messageCountsChanged();

    // TODO: counts changed
    //emit messageCountsChanged(m_selectedItem.mode(), false, false);
    return true;
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesRestored(const QModelIndexList &messages) {
  QSqlDatabase db_handle = database();
  QSqlQuery query_read_msg(db_handle);
  QStringList message_ids;

  query_read_msg.setForwardOnly(true);

  // Obtain IDs of all desired messages.
  foreach (const QModelIndex &message, messages) {
    message_ids.append(QString::number(messageId(message.row())));
  }

  QString sql_delete_query = QString(QSL("UPDATE Messages SET is_deleted = 0 WHERE id IN (%1);")).arg(message_ids.join(QSL(", ")));

  if (query_read_msg.exec(sql_delete_query)) {
    fetchAllData();

    emit messageCountsChanged();

    // TODO: counts changed
    //emit messageCountsChanged(m_selectedItem.mode(), true, true);
    return true;
  }
  else {
    return false;
  }
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
