#include "core/messagesmodel.h"

#include "core/defs.h"
#include "core/textfactory.h"
#include "core/databasefactory.h"
#include "gui/iconthemefactory.h"
#include "qtsingleapplication/qtsingleapplication.h"

#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>


MessagesModel::MessagesModel(QObject *parent)
  : QSqlTableModel(parent,
                   DatabaseFactory::instance()->connection("MessagesModel",
                                                           DatabaseFactory::FromSettings)) {
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
  m_favoriteIcon = IconThemeFactory::instance()->fromTheme("favorites");
  m_readIcon = IconThemeFactory::instance()->fromTheme("mail-mark-not-junk");
  m_unreadIcon = IconThemeFactory::instance()->fromTheme("mail-mark-important");
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
  m_currentFeeds = feed_ids;

  QString assembled_ids = textualFeeds().join(", ");

  setFilter(QString("feed IN (%1) AND deleted = 0").arg(assembled_ids));
  select();
  fetchAll();

  qDebug("Loading messages from feeds: %s.", qPrintable(assembled_ids));
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
  message.m_created = TextFactory::parseDateTime(rec.value(MSG_DB_DCREATED_INDEX).value<qint64>());

  return message;
}

void MessagesModel::setupHeaderData() {
  m_headerData << tr("Id") << tr("Read") << tr("Deleted") << tr("Important") <<
                  tr("Feed") << tr("Title") << tr("Url") << tr("Author") <<
                  tr("Created on") << tr("Contents");
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

QVariant MessagesModel::data(const QModelIndex &index, int role) const {
  switch (role) {
    // Human readable data for viewing.
    case Qt::DisplayRole: {
      int index_column = index.column();

      if (index_column == MSG_DB_DCREATED_INDEX) {
        return TextFactory::parseDateTime(QSqlTableModel::data(index,
                                                               role).value<qint64>()).toLocalTime().toString(Qt::DefaultLocaleShortDate);
      }
      else if (index_column == MSG_DB_AUTHOR_INDEX) {
        QString author_name = QSqlTableModel::data(index, role).toString();

        return author_name.isEmpty() ? "-" : author_name;
      }
      else if (index_column != MSG_DB_IMPORTANT_INDEX &&
               index_column != MSG_DB_READ_INDEX) {
        return QSqlTableModel::data(index, role);
      }
      else {
        return QVariant();
      }
    }

    case Qt::EditRole:
      return QSqlTableModel::data(index, role);

    case Qt::FontRole:
      return record(index.row()).value(MSG_DB_READ_INDEX).toInt() == 1 ?
            m_normalFont :
            m_boldFont;

    case Qt::DecorationRole: {
      int index_column = index.column();

      if (index_column == MSG_DB_READ_INDEX) {
        return record(index.row()).value(MSG_DB_READ_INDEX).toInt() == 1 ?
              m_readIcon :
              m_unreadIcon;
      }
      else if (index_column == MSG_DB_IMPORTANT_INDEX) {
        return record(index.row()).value(MSG_DB_IMPORTANT_INDEX).toInt() == 1 ?
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
  bool working_change = setData(index(row_index, MSG_DB_READ_INDEX),
                                read);

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebug("Setting of new data to the model failed for message read change.");

    db_handle.rollback();
    return false;
  }

  int message_id;
  QSqlQuery query_read_msg(db_handle);
  query_read_msg.setForwardOnly(true);

  if (!query_read_msg.prepare("UPDATE messages SET read = :read "
                              "WHERE id = :id")) {
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
    emit feedCountsChanged();
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

  if (!query_importance_msg.prepare("UPDATE messages SET important = :important "
                                    "WHERE id = :id")) {
    qWarning("Query preparation failed for message importance switch.");

    db_handle.rollback();
    return false;
  }

  message_id = messageId(row_index);
  query_importance_msg.bindValue(":id", message_id);
  query_importance_msg.bindValue(":important",
                                 current_importance == 1 ? 0 : 1);
  query_importance_msg.exec();

  // Commit changes.
  if (db_handle.commit()) {
    // If commit succeeded, then emit changes, so that view
    // can reflect.
    emit dataChanged(index(row_index, 0),
                     index(row_index, columnCount() - 1));
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

bool MessagesModel::switchBatchMessageImportance(const QModelIndexList &messages) {
  QSqlDatabase db_handle = database();

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for batch message importance switch failed.");
    return false;
  }

  int message_id, importance;
  QSqlQuery query_importance_msg(db_handle);
  query_importance_msg.setForwardOnly(true);

  if (!query_importance_msg.prepare("UPDATE messages SET important = :important "
                                    "WHERE id = :id")) {
    qWarning("Query preparation failed for message importance switch.");

    db_handle.rollback();
    return false;
  }

  foreach (const QModelIndex &message, messages) {
    message_id = messageId(message.row());
    importance = data(message.row(), MSG_DB_IMPORTANT_INDEX, Qt::EditRole).toInt();

    query_importance_msg.bindValue(":id", message_id);
    query_importance_msg.bindValue(":important",
                                   importance == 1 ? 0 : 1);
    query_importance_msg.exec();
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
  QSqlDatabase db_handle = database();

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for batch message deletion.");
    return false;
  }

  int message_id;
  QSqlQuery query_delete_msg(db_handle);
  query_delete_msg.setForwardOnly(true);

  if (!query_delete_msg.prepare("UPDATE messages SET deleted = :deleted "
                                "WHERE id = :id")) {
    qWarning("Query preparation failed for message deletion.");

    db_handle.rollback();
    return false;
  }

  foreach (const QModelIndex &message, messages) {
    message_id = messageId(message.row());
    query_delete_msg.bindValue(":id", message_id);
    query_delete_msg.bindValue(":deleted", deleted);
    query_delete_msg.exec();
  }

  // Commit changes.
  if (db_handle.commit()) {
    // FULLY reload the model if underlying data is changed.
    select();
    fetchAll();

    emit feedCountsChanged();
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

bool MessagesModel::setBatchMessagesRead(const QModelIndexList &messages, int read) {
  QSqlDatabase db_handle = database();

  if (!db_handle.transaction()) {
    qWarning("Starting transaction for batch message read change.");
    return false;
  }

  int message_id;
  QSqlQuery query_read_msg(db_handle);
  query_read_msg.setForwardOnly(true);

  if (!query_read_msg.prepare("UPDATE messages SET read = :read "
                              "WHERE id = :id")) {
    qWarning("Query preparation failed for message read change.");

    db_handle.rollback();
    return false;
  }

  foreach (const QModelIndex &message, messages) {
    message_id = messageId(message.row());
    query_read_msg.bindValue(":id", message_id);
    query_read_msg.bindValue(":read", read);
    query_read_msg.exec();
  }

  // Commit changes.
  if (db_handle.commit()) {
    // FULLY reload the model if underlying data is changed.
    select();
    fetchAll();

    emit feedCountsChanged();
    return true;
  }
  else {
    return db_handle.rollback();
  }
}

QVariant MessagesModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const {
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
