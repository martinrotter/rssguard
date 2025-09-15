// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/messagesforfiltersmodel.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/filteringexception.h"
#include "filtering/filteringsystem.h"
#include "filtering/filterobjects.h"
#include "filtering/messagefilter.h"
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
          case FilterMessage::FilteringAction::Accept:
            return qApp->skins()->colorForModel(SkinEnums::PaletteColors::Allright);

          case FilterMessage::FilteringAction::Ignore:
          case FilterMessage::FilteringAction::Purge:
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

void MessagesForFiltersModel::processFeeds(MessageFilter* fltr, ServiceRoot* account, const QList<RootItem*>& checked) {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  for (RootItem* it : checked) {
    if (it->kind() == RootItem::Kind::Feed) {
      FilteringSystem filtering(FilteringSystem::FiteringUseCase::ExistingArticles, database, it->toFeed(), account);

      filtering.filterRun().setTotalCountOfFilters(1);
      filtering.filterRun().setIndexOfCurrentFilter(0);

      // We process messages of the feed.
      QList<Message> msgs = it->undeletedMessages();
      QList<Message> read_msgs, important_msgs;

      for (int i = 0; i < msgs.size(); i++) {
        auto labels_in_message =
          DatabaseQueries::getLabelsForMessage(database, msgs[i], filtering.filterAccount().availableLabels());

        // Create backup of message.
        Message* msg = &msgs[i];

        msg->m_assignedLabels = labels_in_message;
        msg->m_rawContents = Message::generateRawAtomContents(*msg);

        Message msg_original(*msg);
        bool remove_mgs = false;

        filtering.setMessage(msg);

        try {
          FilterMessage::FilteringAction result = filtering.filterMessage(*fltr);

          if (result == FilterMessage::FilteringAction::Purge) {
            remove_mgs = true;

            // Purge the message completely and remove leftovers.
            DatabaseQueries::purgeMessage(database, msg->m_id);
          }
          else if (result == FilterMessage::FilteringAction::Ignore) {
            remove_mgs = true;
          }
          else {
            // Article was accepted.
            filtering.filterRun().incrementNumberOfAcceptedMessages();
          }
        }
        catch (const FilteringException& ex) {
          qCriticalNN << LOGSEC_CORE << "Error when running script when processing existing messages:"
                      << QUOTE_W_SPACE_DOT(ex.message());

          continue;
        }

        if (!msg_original.m_isRead && msg->m_isRead) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Message with custom ID: '" << msg_original.m_customId
                   << "' was marked as read by message scripts.";

          read_msgs << *msg;
        }

        if (!msg_original.m_isImportant && msg->m_isImportant) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Message with custom ID: '" << msg_original.m_customId
                   << "' was marked as important by message scripts.";

          important_msgs << *msg;
        }

        // Process changed labels.
        for (Label* lbl : std::as_const(msg_original.m_assignedLabels)) {
          if (!msg->m_assignedLabels.contains(lbl)) {
            // Label is not there anymore, it was deassigned.
            msg->m_deassignedLabelsByFilter << lbl;

            qDebugNN << LOGSEC_FEEDDOWNLOADER << "It was detected that label" << QUOTE_W_SPACE(lbl->customId())
                     << "was DEASSIGNED from message" << QUOTE_W_SPACE(msg->m_customId) << "by message filter(s).";
          }
        }

        for (Label* lbl : std::as_const(msg->m_assignedLabels)) {
          if (!msg_original.m_assignedLabels.contains(lbl)) {
            // Label is in new message, but is not in old message, it
            // was newly assigned.
            msg->m_assignedLabelsByFilter << lbl;

            qDebugNN << LOGSEC_FEEDDOWNLOADER << "It was detected that label" << QUOTE_W_SPACE(lbl->customId())
                     << "was ASSIGNED to message" << QUOTE_W_SPACE(msg->m_customId) << "by message filter(s).";
          }
        }

        if (remove_mgs) {
          // Do not update message.
          msgs.removeAt(i--);
        }
      }

      if (!read_msgs.isEmpty()) {
        // Now we push new read states to the service.
        if (it->getParentServiceRoot()->onBeforeSetMessagesRead(it, read_msgs, RootItem::ReadStatus::Read)) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER << "Notified services about messages marked as read by message filters.";
        }
        else {
          qCriticalNN << LOGSEC_FEEDDOWNLOADER
                      << "Notification of services about messages marked as read by message filters FAILED.";
        }
      }

      if (!important_msgs.isEmpty()) {
        // Now we push new read states to the service.
        auto list = boolinq::from(important_msgs)
                      .select([](const Message& msg) {
                        return ImportanceChange(msg, RootItem::Importance::Important);
                      })
                      .toStdList();
        QList<ImportanceChange> chngs = FROM_STD_LIST(QList<ImportanceChange>, list);

        if (it->getParentServiceRoot()->onBeforeSwitchMessageImportance(it, chngs)) {
          qDebugNN << LOGSEC_FEEDDOWNLOADER
                   << "Notified services about messages marked as important by message filters.";
        }
        else {
          qCriticalNN << LOGSEC_FEEDDOWNLOADER
                      << "Notification of services about messages marked as important by message filters FAILED.";
        }
      }

      // Update messages in DB and reload selection.
      it->getParentServiceRoot()->updateMessages(msgs, it->toFeed(), true, nullptr);
    }
  }
}

void MessagesForFiltersModel::testFilter(MessageFilter* filter, FilteringSystem* engine) {
  m_filteringDecisions.clear();

  for (int i = 0; i < m_messages.size(); i++) {
    Message* msg = messageForRow(i);

    msg->m_rawContents = Message::generateRawAtomContents(*msg);
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
