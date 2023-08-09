// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagesforfiltersmodel.h"

#include "core/messagefilter.h"
#include "definitions/definitions.h"
#include "exceptions/filteringexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/skinfactory.h"

MessagesForFiltersModel::MessagesForFiltersModel(QObject* parent) : QAbstractTableModel(parent) {
  m_headerData << tr("Read") << tr("Important") << tr("In recycle bin") << tr("Title") << tr("URL") << tr("Author")
               << tr("Date") << tr("Score");
}

void MessagesForFiltersModel::setMessages(const QList<Message>& messages) {
  m_filteringDecisions.clear();
  m_messages = messages;

  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

int MessagesForFiltersModel::rowCount(const QModelIndex& parent) const {
  Q_UNUSED(parent)
  return m_messages.size();
}

int MessagesForFiltersModel::columnCount(const QModelIndex& parent) const {
  Q_UNUSED(parent)
  return m_headerData.size();
}

QVariant MessagesForFiltersModel::data(const QModelIndex& index, int role) const {
  auto msg = messageForRow(index.row());
  QString bool_true = tr("true");
  QString bool_false = tr("false");

  switch (role) {
    case Qt::ItemDataRole::BackgroundRole: {
      if (m_filteringDecisions.contains(index.row())) {
        switch (m_filteringDecisions.value(index.row())) {
          case MessageObject::FilteringAction::Accept:
            return qApp->skins()->colorForModel(SkinEnums::PaletteColors::Allright);

          case MessageObject::FilteringAction::Ignore:
          case MessageObject::FilteringAction::Purge:
            return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgError);

          default:
            break;
        }
      }

      break;
    }

    case Qt::ItemDataRole::DisplayRole: {
      switch (index.column()) {
        case MFM_MODEL_ISREAD:
          return msg.m_isRead ? bool_true : bool_false;

        case MFM_MODEL_ISIMPORTANT:
          return msg.m_isImportant ? bool_true : bool_false;

        case MFM_MODEL_ISDELETED:
          return msg.m_isDeleted ? bool_true : bool_false;

        case MFM_MODEL_TITLE:
          return msg.m_title;

        case MFM_MODEL_URL:
          return msg.m_url;

        case MFM_MODEL_AUTHOR:
          return msg.m_author;

        case MFM_MODEL_CREATED:
          return msg.m_created;

        case MFM_MODEL_SCORE:
          return msg.m_score;
      }

      break;
    }
  }

  return QVariant();
}

QVariant MessagesForFiltersModel::headerData(int section, Qt::Orientation orientation, int role) const {
  Q_UNUSED(orientation)

  switch (role) {
    case Qt::ItemDataRole::DisplayRole:
      if (section >= 0 && section < m_headerData.size()) {
        return m_headerData.at(section);
      }

      break;
  }

  return QVariant();
}

Qt::ItemFlags MessagesForFiltersModel::flags(const QModelIndex& index) const {
  Q_UNUSED(index)
  return Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable;
}

int MessagesForFiltersModel::messagesCount() const {
  return m_messages.size();
}

void MessagesForFiltersModel::testFilter(MessageFilter* filter, QJSEngine* engine, MessageObject* msg_proxy) {
  m_filteringDecisions.clear();

  for (int i = 0; i < m_messages.size(); i++) {
    Message* msg = messageForRow(i);

    msg->m_rawContents = Message::generateRawAtomContents(*msg);
    msg_proxy->setMessage(msg);

    try {
      MessageObject::FilteringAction decision = filter->filterMessage(engine);

      m_filteringDecisions.insert(i, decision);
    }
    catch (const FilteringException& ex) {
      throw ex;
    }
  }

  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

Message* MessagesForFiltersModel::messageForRow(int row) {
  if (row >= 0 && row < m_messages.size()) {
    return &m_messages[row];
  }
  else {
    return nullptr;
  }
}

Message MessagesForFiltersModel::messageForRow(int row) const {
  if (row >= 0 && row < m_messages.size()) {
    return m_messages[row];
  }
  else {
    return Message();
  }
}
