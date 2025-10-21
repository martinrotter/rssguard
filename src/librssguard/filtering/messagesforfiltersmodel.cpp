// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/messagesforfiltersmodel.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/filteringexception.h"
#include "filtering/filteringsystem.h"
#include "filtering/filterobjects.h"
#include "filtering/messagefilter.h"
#include "miscellaneous/application.h"
#include "miscellaneous/skinfactory.h"
#include "services/abstract/labelsnode.h"

MessagesForFiltersModel::MessagesForFiltersModel(QObject* parent) : QAbstractTableModel(parent) {
  m_headerData << tr("Result") << tr("Read") << tr("Important") << tr("Trash") << tr("Title") << tr("Date")
               << tr("Score");
}

void MessagesForFiltersModel::setMessages(const QList<Message>& messages) {
  m_filteringDecisions.clear();
  m_messages.clear();

  for (const Message& msg : messages) {
    MessageBackupAndOriginal msg_tuple;
    msg_tuple.m_original = msg;
    msg_tuple.m_filtered = msg;

    m_messages.append(msg_tuple);
  }

  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

QString MessagesForFiltersModel::decisionToText(FilterMessage::FilteringAction dec) const {
  switch (dec) {
    case FilterMessage::FilteringAction::Accept:
      return QSL("✓");

    case FilterMessage::FilteringAction::Ignore:
      return QSL("❌");

    case FilterMessage::FilteringAction::Purge:
      return QSL("❌❌");

    default:
      return QSL("?");
  }
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
    case Qt::ItemDataRole::ForegroundRole: {
      if (index.column() == MFM_MODEL_RESULT) {
        if (m_filteringDecisions.contains(index.row())) {
          switch (m_filteringDecisions.value(index.row())) {
            case FilterMessage::FilteringAction::Accept:
              return qApp->skins()->colorForModel(SkinEnums::PaletteColors::Allright);

            case FilterMessage::FilteringAction::Ignore:
            case FilterMessage::FilteringAction::Purge:
              return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgError);

            default:
              break;
          }
        }
      }
      else {
        QVariant interest = qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgError);
        Message msg_original = m_messages[index.row()].m_original;

        switch (index.column()) {
          case MFM_MODEL_ISREAD:
            return msg.m_isRead != msg_original.m_isRead ? interest : QVariant();

          case MFM_MODEL_ISIMPORTANT:
            return msg.m_isImportant != msg_original.m_isImportant ? interest : QVariant();

          case MFM_MODEL_ISDELETED:
            return msg.m_isDeleted != msg_original.m_isDeleted ? interest : QVariant();

          case MFM_MODEL_TITLE:
            return msg.m_title != msg_original.m_title ? interest : QVariant();

          case MFM_MODEL_CREATED:
            return msg.m_created != msg_original.m_created ? interest : QVariant();

          case MFM_MODEL_SCORE:
            return msg.m_score != msg_original.m_score ? interest : QVariant();
        }
      }

      break;
    }

    case Qt::ItemDataRole::DisplayRole: {
      switch (index.column()) {
        case MFM_MODEL_RESULT:
          return m_filteringDecisions.contains(index.row()) ? decisionToText(m_filteringDecisions.value(index.row()))
                                                            : QSL("?");

        case MFM_MODEL_ISREAD:
          return msg.m_isRead ? bool_true : bool_false;

        case MFM_MODEL_ISIMPORTANT:
          return msg.m_isImportant ? bool_true : bool_false;

        case MFM_MODEL_ISDELETED:
          return msg.m_isDeleted ? bool_true : bool_false;

        case MFM_MODEL_TITLE:
          return msg.m_title;

        case MFM_MODEL_CREATED:
          return msg.m_created;

        case MFM_MODEL_SCORE:
          return msg.m_score;
      }

      break;
    }

    case Qt::ItemDataRole::ToolTipRole: {
      Message msg_original = m_messages[index.row()].m_original;

      switch (index.column()) {
        case MFM_MODEL_RESULT:
          return m_filteringDecisions.contains(index.row()) ? decisionToText(m_filteringDecisions.value(index.row()))
                                                            : QSL("?");

        case MFM_MODEL_ISREAD:
          return QSL(VALUE_COMPARISON_FORMAT)
            .arg(msg.m_isRead ? bool_true : bool_false, msg_original.m_isRead ? bool_true : bool_false);

        case MFM_MODEL_ISIMPORTANT:
          return QSL(VALUE_COMPARISON_FORMAT)
            .arg(msg.m_isImportant ? bool_true : bool_false, msg_original.m_isImportant ? bool_true : bool_false);

        case MFM_MODEL_ISDELETED:
          return QSL(VALUE_COMPARISON_FORMAT)
            .arg(msg.m_isDeleted ? bool_true : bool_false, msg_original.m_isDeleted ? bool_true : bool_false);

        case MFM_MODEL_TITLE:
          return QSL(VALUE_COMPARISON_FORMAT).arg(msg.m_title, msg_original.m_title);

        case MFM_MODEL_CREATED:
          return QSL(VALUE_COMPARISON_FORMAT)
            .arg(msg.m_created.toString(Qt::DateFormat::ISODate),
                 msg_original.m_created.toString(Qt::DateFormat::ISODate));

        case MFM_MODEL_SCORE:
          return QSL(VALUE_COMPARISON_FORMAT).arg(QString::number(msg.m_score), QString::number(msg_original.m_score));
      }
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

void MessagesForFiltersModel::processFeeds(MessageFilter* fltr, ServiceRoot* account, const QList<RootItem*>& checked) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  for (RootItem* it : checked) {
    if (it->kind() == RootItem::Kind::Feed) {
      FilteringSystem filtering(FilteringSystem::FiteringUseCase::ExistingArticles, database, it->toFeed(), account);

      filtering.filterRun().setTotalCountOfFilters(1);
      filtering.filterRun().setIndexOfCurrentFilter(0);

      // We process messages of the feed.
      QList<Message> msgs = DatabaseQueries::getUndeletedMessagesForFeed(database,
                                                                         it->id(),
                                                                         account->labelsNode()->getHashedLabels(),
                                                                         account->accountId());
      QList<Message> read_msgs, important_msgs;

      for (int i = 0; i < msgs.size(); i++) {
        Message* msg_filtered = &msgs[i];
        Message msg_original(*msg_filtered);
        bool remove_msg = false;

        filtering.setMessage(msg_filtered);

        FilterMessage::FilteringAction result = filtering.filterMessage(*fltr);

        if (result == FilterMessage::FilteringAction::Purge) {
          remove_msg = true;

          // Purge the message completely and remove leftovers.
          DatabaseQueries::purgeMessage(database, msg_filtered->m_id);
        }
        else if (result == FilterMessage::FilteringAction::Ignore) {
          remove_msg = true;
        }
        else {
          // Article was accepted.
          filtering.filterRun().incrementNumberOfAcceptedMessages();
        }

        filtering.compareAndWriteArticleStates(&msg_original, msg_filtered, read_msgs, important_msgs);

        if (remove_msg) {
          // Do not update message.
          msgs.removeAt(i--);
        }
      }

      filtering.pushMessageStatesToServices(read_msgs, important_msgs, it, account);

      // Update messages in DB and reload selection.
      it->account()->updateMessages(msgs, it->toFeed(), true, true, nullptr);
    }
  }

  DatabaseQueries::purgeLeftoverLabelAssignments(database, account->accountId());
}

void MessagesForFiltersModel::testFilter(MessageFilter* filter, FilteringSystem* engine) {
  m_filteringDecisions.clear();

  for (int i = 0; i < m_messages.size(); i++) {
    auto& msg_orig_backup = m_messages[i];

    msg_orig_backup.m_filtered = Message(msg_orig_backup.m_original);

    Message* msg = &msg_orig_backup.m_filtered;
    engine->setMessage(msg);

    try {
      FilterMessage::FilteringAction decision = engine->filterMessage(*filter);

      switch (decision) {
        case FilterMessage::FilteringAction::Accept:
          // Message is normally accepted, it could be tweaked by the filter.
          engine->filterRun().incrementNumberOfAcceptedMessages();
          break;

        case FilterMessage::FilteringAction::Ignore:
        case FilterMessage::FilteringAction::Purge:
        default:
          // Remove the message, we do not want it.
          break;
      }

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
    return &m_messages[row].m_filtered;
  }
  else {
    return nullptr;
  }
}

Message MessagesForFiltersModel::messageForRow(int row) const {
  if (row >= 0 && row < m_messages.size()) {
    return m_messages[row].m_filtered;
  }
  else {
    return Message();
  }
}
