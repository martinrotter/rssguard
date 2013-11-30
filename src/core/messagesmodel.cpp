#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>

#include "qtsingleapplication/qtsingleapplication.h"

#include "core/defs.h"
#include "core/textfactory.h"
#include "core/messagesmodel.h"
#include "core/databasefactory.h"
#include "gui/iconthemefactory.h"


MessagesModel::MessagesModel(QObject *parent)
  : QSqlTableModel(parent,
                   DatabaseFactory::getInstance()->addConnection("MessagesModel")) {
  setObjectName("MessagesModel");

  setupFonts();
  setupIcons();
  setupHeaderData();

  // Set desired table and edit strategy.
  setEditStrategy(QSqlTableModel::OnManualSubmit);
  setTable("Messages");

  loadMessages(QList<int>());
}

MessagesModel::~MessagesModel() {
  qDebug("Destroying MessagesModel instance.");
}

void MessagesModel::setupIcons() {
  m_favoriteIcon = IconThemeFactory::getInstance()->fromTheme("mail-mark-important");
  m_readIcon = IconThemeFactory::getInstance()->fromTheme("mail-mark-read");
  m_unreadIcon = IconThemeFactory::getInstance()->fromTheme("mail-mark-unread");
}

void MessagesModel::fetchAll() {
  while (canFetchMore()) {
    fetchMore();
  }
}

void MessagesModel::setupFonts() {
  m_normalFont = QtSingleApplication::font("MessagesView");

  m_boldFont = m_normalFont;
  m_boldFont.setBold(true);
}

void MessagesModel::loadMessages(const QList<int> feed_ids) {
  // Submit changes first.
  submitAll();

  // Conversion of parameter.
  m_currentFeeds = feed_ids;
  QStringList stringy_ids;
  stringy_ids.reserve(feed_ids.count());

  foreach (int feed_id, feed_ids) {
    stringy_ids.append(QString::number(feed_id));
  }

  // TODO: Enable when time is right.
  //setFilter(QString("feed IN (%1) AND deleted = 0").arg(stringy_ids.join(',')));
  select();
  fetchAll();
}

void MessagesModel::endInsertColumns() {

  qDebug("end insert cols");

  QSqlTableModel::endInsertColumns();
}

int MessagesModel::messageId(int row_index) const {
  return record(row_index).value(MSG_DB_ID_INDEX).toInt();
}

Message MessagesModel::messageAt(int row_index) const {
  QSqlRecord rec = record(row_index);
  Message message;

  // Fill Message object with details.
  message.m_author = rec.value(MSG_DB_AUTHOR_INDEX).toString();
  message.m_contents = rec.value(MSG_DB_CONTENTS_INDEX).toString();
  message.m_title = rec.value(MSG_DB_TITLE_INDEX).toString();
  message.m_url = rec.value(MSG_DB_URL_INDEX).toString();
  message.m_updated = TextFactory::parseDateTime(rec.value(MSG_DB_DUPDATED_INDEX).toString());

  return message;
}

void MessagesModel::setupHeaderData() {
  m_headerData << tr("Id") << tr("Read") << tr("Deleted") << tr("Important") <<
                  tr("Feed") << tr("Title") << tr("Url") << tr("Author") <<
                  tr("Created on") << tr("Updated on") << tr("Contents");
}

Qt::ItemFlags MessagesModel::flags(const QModelIndex &idx) const {
  Q_UNUSED(idx);

  if (m_isInEditingMode) {
    // NOTE: Editing of model must be temporarily enabled here.
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable;
  }
  else {
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
  }
}

QVariant MessagesModel::data(int row, int column, int role) const {
  return data(index(row, column), role);
}

QVariant MessagesModel::data(const QModelIndex &idx, int role) const {
  switch (role) {
    // Human readable data for viewing.
    case Qt::DisplayRole: {
      int index_column = idx.column();
      if (index_column != MSG_DB_IMPORTANT_INDEX &&
          index_column != MSG_DB_READ_INDEX) {
        return QSqlTableModel::data(idx, role);
      }
      else {
        return QVariant();
      }
    }

      // Return RAW data for EditRole.
    case Qt::EditRole:
      return QSqlTableModel::data(idx, role);

    case Qt::FontRole:
      return record(idx.row()).value(MSG_DB_READ_INDEX).toInt() == 1 ?
            m_normalFont :
            m_boldFont;

    case Qt::DecorationRole: {
      int index_column = idx.column();

      if (index_column == MSG_DB_READ_INDEX) {
        return record(idx.row()).value(MSG_DB_READ_INDEX).toInt() == 1 ?
              m_readIcon :
              m_unreadIcon;
      }
      else if (index_column == MSG_DB_IMPORTANT_INDEX) {
        return record(idx.row()).value(MSG_DB_IMPORTANT_INDEX).toInt() == 1 ?
              m_favoriteIcon :
              QVariant();
      }
      else {
        return QVariant();
      }
    }

    default:
      return QVariant();
  }
}

bool MessagesModel::setData(const QModelIndex &idx, const QVariant &value, int role) {
  m_isInEditingMode = true;
  bool set_data_result = QSqlTableModel::setData(idx, value, role);
  m_isInEditingMode = false;

  return set_data_result;
}

bool MessagesModel::setMessageRead(int row_index, int read) {
  return setData(index(row_index, MSG_DB_READ_INDEX),
                 read);
}

bool MessagesModel::setMessageDeleted(int row_index, int deleted) {
  return setData(index(row_index, MSG_DB_DELETED_INDEX),
                 deleted);
}

bool MessagesModel::switchMessageImportance(int row_index) {
  QModelIndex target_index = index(row_index, MSG_DB_IMPORTANT_INDEX);
  int current_importance = data(target_index).toInt();

  return current_importance == 1 ?
        setData(target_index, 0) :
        setData(target_index, 1);
}

bool MessagesModel::switchBatchMessageImportance(const QModelIndexList &messages) {
  // Submit changes first.
  submitAll();

  if (!database().transaction()) {
    qWarning("Starting transaction for batch message importance switch failed.");
    return false;
  }

  QSqlDatabase db_handle = database();
  int message_id, importance;
  QSqlQuery query_delete_msg(db_handle);
  if (!query_delete_msg.prepare("UPDATE messages SET important = :important "
                                "WHERE id = :id")) {
    qWarning("Query preparation failed for message importance switch.");
    return false;
  }

  foreach (const QModelIndex &message, messages) {
    message_id = messageId(message.row());
    importance = record(message.row()).value(MSG_DB_IMPORTANT_INDEX).toInt();

    query_delete_msg.bindValue(":id", message_id);
    query_delete_msg.bindValue(":important",
                               importance == 1 ? 0 : 1);
    query_delete_msg.exec();
  }

  // Commit changes.
  if (db_handle.commit()) {
    // FULLY reload the model if underlying data is changed.
    select();
    fetchAll();
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

bool MessagesModel::setBatchMessagesDeleted(const QModelIndexList &messages, int deleted) {

}

bool MessagesModel::setBatchMessagesRead(const QModelIndexList &messages, int read) {

}

bool MessagesModel::switchAllMessageImportance() {
  return false;
}

bool MessagesModel::setAllMessagesDeleted(int deleted) {
  return false;
}

bool MessagesModel::setAllMessagesRead(int read) {
  return false;
}

QVariant MessagesModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const {
  Q_UNUSED(orientation);

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

      // Return RAW data for these roles.
    case Qt::ToolTipRole:
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
