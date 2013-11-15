#include "core/messagesmodel.h"
#include "core/databasefactory.h"


MessagesModel::MessagesModel(QObject *parent)
  : QSqlTableModel(parent,
                   DatabaseFactory::getInstance()->addConnection("MessagesModel")) {
  setObjectName("MessagesModel");
  setEditStrategy(QSqlTableModel::OnFieldChange);

  setupHeaderData();

  setTable("Messages");
  select();
}

MessagesModel::~MessagesModel() {
  qDebug("Destroying MessagesModel instance.");
}

void MessagesModel::setupHeaderData() {
  // TODO: Enhance this.
  m_headerData << tr("Id") << tr("Feed") << tr("Title") <<
                  tr("URL") << tr("Author") << tr("Created on") <<
                  tr("Last updated on") << tr("Read") << tr("Deleted") <<
                  tr("Important") << tr("Contents");
}

Qt::ItemFlags MessagesModel::flags(const QModelIndex &index) const {
  Q_UNUSED(index);

  return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QVariant MessagesModel::headerData(int section,
                                   Qt::Orientation orientation,
                                   int role) const {
  Q_UNUSED(orientation);

  // TODO: Ehance this with graphics and other roles.

  switch (role) {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
      return m_headerData.at(section);

    default:
      return QVariant();
  }
}
