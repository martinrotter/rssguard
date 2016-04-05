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

#include "core/messagesmodel.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iconfactory.h"
#include "gui/dialogs/formmain.h"
#include "services/abstract/serviceroot.h"
#include "miscellaneous/databasequeries.h"


MessagesModel::MessagesModel(QObject *parent)
  : QSqlTableModel(parent, qApp->database()->connection(QSL("MessagesModel"), DatabaseFactory::FromSettings)),
    m_messageHighlighter(NoHighlighting), m_customDateFormat(QString()) {
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
      setFilter("true != true");
      qWarning("Loading of messages from item '%s' failed.", qPrintable(item->title()));
      qApp->showGuiMessage(tr("Loading of messages from item '%1' failed.").arg(item->title()),
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

RootItem::Importance MessagesModel::messageImportance(int row_index) const {
  return (RootItem::Importance) data(row_index, MSG_DB_IMPORTANT_INDEX, Qt::EditRole).toInt();
}

RootItem *MessagesModel::loadedItem() const {
  return m_selectedItem;
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
  return Message::fromSqlRecord(record(row_index));
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
                  /*: Tooltip for attachments of message.*/ tr("Attachments") <<
                  /*: Tooltip for account ID of message.*/ tr("Account ID") <<
                  /*: Tooltip for custom ID of message.*/ tr("Custom ID") <<
                  /*: Tooltip for custom hash string of message.*/ tr("Custom hash");

  m_tooltipData << tr("Id of the message.") << tr("Is message read?") <<
                   tr("Is message deleted?") << tr("Is message important?") <<
                   tr("Id of feed which this message belongs to.") <<
                   tr("Title of the message.") << tr("Url of the message.") <<
                   tr("Author of the message.") << tr("Creation date of the message.") <<
                   tr("Contents of the message.") << tr("Is message permanently deleted from recycle bin?") <<
                   tr("List of attachments.") << tr("Account ID of the message.") << tr("Custom ID of the message") <<
                   tr("Custom hash of the message.");
}

Qt::ItemFlags MessagesModel::flags(const QModelIndex &index) const {
  Q_UNUSED(index)

  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren;
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
        const QString author_name = QSqlTableModel::data(idx, role).toString();

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
      const int index_column = idx.column();

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

bool MessagesModel::setMessageRead(int row_index, RootItem::ReadStatus read) {
  if (data(row_index, MSG_DB_READ_INDEX, Qt::EditRole).toInt() == read) {
    // Read status is the same is the one currently set.
    // In that case, no extra work is needed.
    return true;
  }

  Message message = messageAt(row_index);

  if (!m_selectedItem->getParentServiceRoot()->onBeforeSetMessagesRead(m_selectedItem, QList<Message>() << message, read)) {
    // Cannot change read status of the item. Abort.
    return false;
  }

  // Rewrite "visible" data in the model.
  bool working_change = setData(index(row_index, MSG_DB_READ_INDEX), read);

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebug("Setting of new data to the model failed for message read change.");
    return false;
  }

  if (DatabaseQueries::markMessagesReadUnread(database(), QStringList() << QString::number(message.m_id), read)) {
    return m_selectedItem->getParentServiceRoot()->onAfterSetMessagesRead(m_selectedItem, QList<Message>() << message, read);
  }
  else {
    return false;
  }
}

bool MessagesModel::switchMessageImportance(int row_index) {
  const QModelIndex target_index = index(row_index, MSG_DB_IMPORTANT_INDEX);
  const RootItem::Importance current_importance = (RootItem::Importance) data(target_index, Qt::EditRole).toInt();
  const RootItem::Importance next_importance = current_importance == RootItem::Important ?
                                                 RootItem::NotImportant : RootItem::Important;
  const Message message = messageAt(row_index);
  const QPair<Message,RootItem::Importance> pair(message, next_importance);

  if (!m_selectedItem->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_selectedItem,
                                                                               QList<QPair<Message,RootItem::Importance> >() << pair)) {
    return false;
  }

  // Rewrite "visible" data in the model.
  const bool working_change = setData(target_index, next_importance);

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebug("Setting of new data to the model failed for message importance change.");
    return false;
  }

  // Commit changes.
  if (DatabaseQueries::markMessageImportant(database(), message.m_id, next_importance)) {
    return m_selectedItem->getParentServiceRoot()->onAfterSwitchMessageImportance(m_selectedItem,
                                                                                  QList<QPair<Message,RootItem::Importance> >() << pair);
  }
  else {
    return false;
  }
}

bool MessagesModel::switchBatchMessageImportance(const QModelIndexList &messages) {
  QStringList message_ids;
  QList<QPair<Message,RootItem::Importance> > message_states;

  // Obtain IDs of all desired messages.
  foreach (const QModelIndex &message, messages) {
    const Message msg = messageAt(message.row());
    RootItem::Importance message_importance = messageImportance((message.row()));

    message_states.append(QPair<Message,RootItem::Importance>(msg, message_importance == RootItem::Important ?
                                                                RootItem::NotImportant :
                                                                RootItem::Important));
    message_ids.append(QString::number(msg.m_id));
  }

  if (!m_selectedItem->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_selectedItem, message_states)) {
    return false;
  }

  if (DatabaseQueries::switchMessagesImportance(database(), message_ids)) {
    fetchAllData();
    return m_selectedItem->getParentServiceRoot()->onAfterSwitchMessageImportance(m_selectedItem, message_states);
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesDeleted(const QModelIndexList &messages) {
  QStringList message_ids;
  QList<Message> msgs;

  // Obtain IDs of all desired messages.
  foreach (const QModelIndex &message, messages) {
    const Message msg = messageAt(message.row());

    msgs.append(msg);
    message_ids.append(QString::number(msg.m_id));
  }

  if (!m_selectedItem->getParentServiceRoot()->onBeforeMessagesDelete(m_selectedItem, msgs)) {
    return false;
  }

  bool deleted;

  if (m_selectedItem->kind() != RootItemKind::Bin) {
    deleted = DatabaseQueries::deleteOrRestoreMessagesToFromBin(database(), message_ids, true);
  }
  else {
    deleted = DatabaseQueries::permanentlyDeleteMessages(database(), message_ids);
  }

  if (deleted) {
    fetchAllData();
    return m_selectedItem->getParentServiceRoot()->onAfterMessagesDelete(m_selectedItem, msgs);
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesRead(const QModelIndexList &messages, RootItem::ReadStatus read) {
  QStringList message_ids;
  QList<Message> msgs;

  // Obtain IDs of all desired messages.
  foreach (const QModelIndex &message, messages) {
    Message msg = messageAt(message.row());

    msgs.append(msg);
    message_ids.append(QString::number(msg.m_id));
  }

  if (!m_selectedItem->getParentServiceRoot()->onBeforeSetMessagesRead(m_selectedItem, msgs, read)) {
    return false;
  }

  if (DatabaseQueries::markMessagesReadUnread(database(), message_ids, read)) {
    fetchAllData();
    return m_selectedItem->getParentServiceRoot()->onAfterSetMessagesRead(m_selectedItem, msgs, read);
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesRestored(const QModelIndexList &messages) {
  QStringList message_ids;
  QList<Message> msgs;

  // Obtain IDs of all desired messages.
  foreach (const QModelIndex &message, messages) {
    const Message msg = messageAt(message.row());

    msgs.append(msg);
    message_ids.append(QString::number(msg.m_id));
  }

  if (!m_selectedItem->getParentServiceRoot()->onBeforeMessagesRestoredFromBin(m_selectedItem, msgs)) {
    return false;
  }

  if (DatabaseQueries::deleteOrRestoreMessagesToFromBin(database(), message_ids, false)) {
    fetchAllData();
    return m_selectedItem->getParentServiceRoot()->onAfterMessagesRestoredFromBin(m_selectedItem, msgs);
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
