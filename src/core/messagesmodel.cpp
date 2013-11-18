#include <QSqlRecord>
#include <QSqlError>
#include <QSqlQuery>

#include "qtsingleapplication/qtsingleapplication.h"

#include "core/defs.h"
#include "core/messagesmodel.h"
#include "core/databasefactory.h"
#include "gui/iconthemefactory.h"


MessagesModel::MessagesModel(QObject *parent)
  : QSqlTableModel(parent, DatabaseFactory::getInstance()->addConnection("MessagesModel")) {
  setObjectName("MessagesModel");

  setupFonts();
  setupIcons();
  setupHeaderData();

  // Set desired table and edit strategy.
  setEditStrategy(QSqlTableModel::OnFieldChange);
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
  // TODO: Doplnit "AND deleted = 0"

  // Conversion of parameter.
  QStringList stringy_ids;
  stringy_ids.reserve(feed_ids.count());

  foreach (int feed_id, feed_ids) {
    stringy_ids.append(QString::number(feed_id));
  }

  // TODO: časem povolit.
  //setFilter(QString("feed IN (%1) ").arg(stringy_ids.join(',')));
  select();
  fetchAll();
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

QVariant MessagesModel::data(const QModelIndex &idx, int role) const {
  switch (role) {
    case Qt::DisplayRole:
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
      else {
        return QVariant();
      }
    }


    default:
      return QVariant();
  }
  /*
  if (role == Qt::FontRole && idx.column() == 1) {
    return record(idx.row()).value(1).toInt() == 1 ? m_normalFont : m_boldFont;
  }
  else if (role == Qt::DecorationRole && idx.column() == 3) {
    if (record(idx.row()).value(1).toInt() == 1) {
      return IconThemeFactory::getInstance()->fromTheme("mail-mark-read");
    }
    else {
      return IconThemeFactory::getInstance()->fromTheme("mail-mark-unread");
    }
  }
  else {
    return QSqlTableModel::data(idx, role);
  }*/
}

bool MessagesModel::setData(const QModelIndex &idx, const QVariant &value, int role) {
  if (!idx.isValid()) {
    return false;
  }

  m_isInEditingMode = true;
  bool set_data_result = QSqlTableModel::setData(idx, value, role);
  m_isInEditingMode = false;

  return set_data_result;
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
