#include "core/messagesmodel.h"


MessagesModel::MessagesModel(QObject *parent) : QSqlTableModel(parent) {
  setObjectName("MessagesModel");
  setupHeaderData();
}

void MessagesModel::setupHeaderData() {
  // TODO: Enhance this.
  m_headerData << tr("aaa") <<
                  tr("bbb");
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
