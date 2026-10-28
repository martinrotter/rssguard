// For license of this file, see <project-root-folder>/LICENSE.md.

#include "filtering/messagesforfiltersmodel.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/filteringexception.h"
#include "filtering/filteringsystem.h"
#include "filtering/filterobjects.h"
#include "filtering/messagefilter.h"
#include "miscellaneous/application.h"
#include "miscellaneous/skinfactory.h"
#include "services/abstract/feed.h"
#include "services/abstract/labelsnode.h"

#include <QSet>

MessagesForFiltersModel::MessagesForFiltersModel(Application* app, QObject* parent)
  : QAbstractTableModel(parent), m_application(app) {
  m_headerData << tr("Filter decision") << tr("Database outcome") << tr("Read") << tr("Important") << tr("Trash")
               << tr("Title") << tr("Date") << tr("Received") << tr("Score");

  m_txtTrue = tr("true");
  m_txtFalse = tr("false");

  m_colorOk = m_application->skins()->colorForModel(SkinEnums::PaletteColors::Allright).value<QColor>();
  m_colorError = m_application->skins()->colorForModel(SkinEnums::PaletteColors::FgError).value<QColor>();
}

Message MessagesForFiltersModel::snapshotForFiltering(const Message& message) {
  Message snapshot(message);
  snapshot.m_enclosures.clear();
  for (const auto& enclosure : message.m_enclosures) {
    snapshot.m_enclosures.append(QSharedPointer<MessageEnclosure>::create(*enclosure));
  }
  return snapshot;
}

void MessagesForFiltersModel::setMessages(const QList<Message>& messages) {
  beginResetModel();
  m_messages.clear();
  m_messages.reserve(messages.size());
  m_summary.clear();
  m_error.clear();
  m_preview = false;
  m_showStoredValues = false;
  for (const Message& msg : messages) {
    MessageBackupAndOriginal row;
    row.m_original = snapshotForFiltering(msg);
    row.m_filtered = snapshotForFiltering(msg);
    row.m_expectedStored = snapshotForFiltering(msg);
    m_messages.append(row);
  }
  endResetModel();
  emit summaryChanged(m_summary);
  emit resultsChanged(false);
}

QString MessagesForFiltersModel::decisionToText(FilterMessage::FilteringAction dec) const {
  switch (dec) {
    case FilterMessage::FilteringAction::Accept:
      return tr("Accept");
    case FilterMessage::FilteringAction::Ignore:
      return tr("Ignore");
    case FilterMessage::FilteringAction::Purge:
      return tr("Purge");
    default:
      return tr("Not processed");
  }
}

QString MessagesForFiltersModel::databaseOutcome(const MessageBackupAndOriginal& row, bool detailed) const {
  QString text;
  if (row.m_saveResult) {
    const auto& saved = *row.m_saveResult;
    switch (saved.m_outcome) {
      case MessageWriteOutcome::Inserted:
        text = m_preview ? tr("Would insert") : tr("Inserted");
        break;
      case MessageWriteOutcome::Updated:
        text = m_preview ? tr("Would update") : tr("Updated");
        break;
      case MessageWriteOutcome::Skipped:
        switch (saved.m_decision.m_reason) {
          case MessageSaveReason::PermanentlyDeleted:
            text = tr("Skipped: permanently deleted");
            break;
          case MessageSaveReason::MissingArticle:
            text = tr("Skipped: article no longer exists");
            break;
          case MessageSaveReason::InvalidArticle:
            text = tr("Skipped: invalid article");
            break;
          default:
            text = tr("Skipped: no update trigger");
            break;
        }
        break;
      case MessageWriteOutcome::Failed:
        text = tr("Write failed or unconfirmed");
        break;
      case MessageWriteOutcome::NotAttempted:
        text = tr("Not attempted");
        break;
    }
    if (saved.m_labelsChanged) {
      text += m_preview ? tr("; labels would change") : tr("; labels changed");
    }
    if (saved.m_removedByCleanup) {
      text += tr("; removed by feed cleanup");
    }
    if (detailed) {
      if (saved.m_decision.m_reason == MessageSaveReason::ExplicitOverwrite) {
        text += tr("\nThe filter explicitly requested a full article overwrite.");
      }
      if (saved.m_decision.m_preserveReadState) {
        text += tr("\nThe stored read state is preserved.");
      }
      if (saved.m_decision.m_preserveImportantState) {
        text += tr("\nThe stored starred state is preserved.");
      }
      if (!saved.m_error.isEmpty()) {
        text += QL1C('\n') + saved.m_error;
      }
    }
  }
  else if (row.m_purged) {
    text = tr("Purged");
  }
  else if (!row.m_error.isEmpty()) {
    text = tr("Not saved: processing failed");
  }
  else if (row.m_decision == FilterMessage::FilteringAction::Ignore) {
    text = tr("No article write: ignored");
  }
  else if (m_preview && row.m_decision == FilterMessage::FilteringAction::Purge) {
    text = tr("Would purge");
  }
  else {
    text = tr("Not attempted");
  }
  if (detailed && !row.m_error.isEmpty()) {
    text += QL1C('\n') + row.m_error;
  }
  if (detailed && m_preview) {
    text += tr("\nPrediction against the current database. Later changes, script side effects and feed cleanup can "
               "affect the actual result.");
  }
  return text;
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
  const auto* msg = constMessageForRow(index.row());

  if (msg == nullptr) {
    return {};
  }

  switch (role) {
    case Qt::ItemDataRole::BackgroundRole: {
      if (index.column() == MFM_MODEL_RESULT) {
        if (m_messages[index.row()].m_decision) {
          switch (*m_messages[index.row()].m_decision) {
            case FilterMessage::FilteringAction::Accept:
              return m_colorOk;

            case FilterMessage::FilteringAction::Ignore:
            case FilterMessage::FilteringAction::Purge:
              return m_colorError;

            default:
              break;
          }
        }
      }

      else if (index.column() == MFM_MODEL_DATABASE) {
        const auto& row = m_messages[index.row()];
        if (!row.m_error.isEmpty() || row.m_purged || row.m_decision == FilterMessage::FilteringAction::Purge) {
          return m_colorError;
        }
        if (row.m_saveResult) {
          const auto& saved = *row.m_saveResult;
          if (!saved.m_error.isEmpty() || saved.m_removedByCleanup) {
            return m_colorError;
          }
          switch (saved.m_outcome) {
            case MessageWriteOutcome::Inserted:
            case MessageWriteOutcome::Updated:
              return m_colorOk;
            case MessageWriteOutcome::Skipped:
              return saved.m_labelsChanged ? m_colorOk : m_colorError;
            case MessageWriteOutcome::Failed:
              return m_colorError;
            case MessageWriteOutcome::NotAttempted:
              break;
          }
        }
        else if (row.m_decision == FilterMessage::FilteringAction::Ignore) {
          return m_colorError;
        }
      }

      break;
    }

    case Qt::ItemDataRole::ForegroundRole: {
      if (index.column() != MFM_MODEL_RESULT) {
        QVariant interest = m_colorError;
        const Message& msg_original = m_messages[index.row()].m_original;

        switch (index.column()) {
          case MFM_MODEL_ISREAD:
            return msg->m_isRead != msg_original.m_isRead ? interest : QVariant();

          case MFM_MODEL_ISIMPORTANT:
            return msg->m_isImportant != msg_original.m_isImportant ? interest : QVariant();

          case MFM_MODEL_ISDELETED:
            return msg->m_isDeleted != msg_original.m_isDeleted ? interest : QVariant();

          case MFM_MODEL_TITLE:
            return msg->m_title != msg_original.m_title ? interest : QVariant();

          case MFM_MODEL_CREATED:
            return msg->m_created != msg_original.m_created ? interest : QVariant();

          case MFM_MODEL_RETRIEVED:
            return msg->m_retrieved != msg_original.m_retrieved ? interest : QVariant();

          case MFM_MODEL_SCORE:
            return msg->m_score != msg_original.m_score ? interest : QVariant();
        }
      }

      break;
    }

    case SortRole:
    case Qt::ItemDataRole::DisplayRole: {
      if (m_showStoredValues && !m_messages[index.row()].m_hasStoredValues && index.column() != MFM_MODEL_RESULT &&
          index.column() != MFM_MODEL_DATABASE && index.column() != MFM_MODEL_TITLE) {
        return role == SortRole ? QVariant() : QVariant(QSL("—"));
      }
      switch (index.column()) {
        case MFM_MODEL_RESULT:
          return m_messages[index.row()].m_decision ? decisionToText(*m_messages[index.row()].m_decision)
                                                    : tr("Not processed");

        case MFM_MODEL_DATABASE:
          return databaseOutcome(m_messages[index.row()], false);

        case MFM_MODEL_ISREAD:
          return role == SortRole ? QVariant(msg->m_isRead)
                                  : QVariant(msg->m_isRead ? m_txtTrue : m_txtFalse);

        case MFM_MODEL_ISIMPORTANT:
          return role == SortRole ? QVariant(msg->m_isImportant)
                                  : QVariant(msg->m_isImportant ? m_txtTrue : m_txtFalse);

        case MFM_MODEL_ISDELETED:
          return role == SortRole ? QVariant(msg->m_isDeleted)
                                  : QVariant(msg->m_isDeleted ? m_txtTrue : m_txtFalse);

        case MFM_MODEL_TITLE:
          return msg->m_title;

        case MFM_MODEL_CREATED:
          return msg->m_created;

        case MFM_MODEL_RETRIEVED:
          return msg->m_retrieved;

        case MFM_MODEL_SCORE:
          return msg->m_score;
      }

      break;
    }

    case Qt::ItemDataRole::ToolTipRole: {
      if (m_showStoredValues && !m_messages[index.row()].m_hasStoredValues && index.column() != MFM_MODEL_RESULT &&
          index.column() != MFM_MODEL_DATABASE) {
        return m_preview ? tr("There would be no stored article.") : tr("The article is no longer stored.");
      }
      const Message& msg_original = m_messages[index.row()].m_original;

      switch (index.column()) {
        case MFM_MODEL_RESULT:
          return m_messages[index.row()].m_decision ? decisionToText(*m_messages[index.row()].m_decision)
                                                    : tr("Not processed");

        case MFM_MODEL_DATABASE:
          return databaseOutcome(m_messages[index.row()], true);

        case MFM_MODEL_ISREAD:
          return QSL(VALUE_COMPARISON_FORMAT)
            .arg(msg->m_isRead ? m_txtTrue : m_txtFalse, msg_original.m_isRead ? m_txtTrue : m_txtFalse);

        case MFM_MODEL_ISIMPORTANT:
          return QSL(VALUE_COMPARISON_FORMAT)
            .arg(msg->m_isImportant ? m_txtTrue : m_txtFalse, msg_original.m_isImportant ? m_txtTrue : m_txtFalse);

        case MFM_MODEL_ISDELETED:
          return QSL(VALUE_COMPARISON_FORMAT)
            .arg(msg->m_isDeleted ? m_txtTrue : m_txtFalse, msg_original.m_isDeleted ? m_txtTrue : m_txtFalse);

        case MFM_MODEL_TITLE:
          return QSL(VALUE_COMPARISON_FORMAT).arg(msg->m_title, msg_original.m_title);

        case MFM_MODEL_CREATED:
          return QSL(VALUE_COMPARISON_FORMAT)
            .arg(msg->m_created.toString(Qt::DateFormat::ISODate),
                 msg_original.m_created.toString(Qt::DateFormat::ISODate));

        case MFM_MODEL_RETRIEVED:
          return QSL(VALUE_COMPARISON_FORMAT)
            .arg(msg->m_retrieved.toString(Qt::DateFormat::ISODate),
                 msg_original.m_retrieved.toString(Qt::DateFormat::ISODate));

        case MFM_MODEL_SCORE:
          return QSL(VALUE_COMPARISON_FORMAT).arg(QString::number(msg->m_score), QString::number(msg_original.m_score));
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

namespace {

  // Processing results retain metadata; bodies are loaded only when selected.
  Message compactMessage(const Message& message) {
    Message result(message);
    result.m_contents.clear();
    result.m_rawContents.clear();
    result.m_customData.clear();
    result.m_enclosures.clear();
    result.m_categories.clear();
    return result;
  }

  int storedIdentity(const MessageBackupAndOriginal& row) {
    if (row.m_saveResult && row.m_saveResult->m_id > 0 &&
        (row.m_saveResult->m_outcome == MessageWriteOutcome::Inserted ||
         row.m_saveResult->m_outcome == MessageWriteOutcome::Updated ||
         row.m_saveResult->m_outcome == MessageWriteOutcome::Skipped)) {
      return row.m_saveResult->m_id;
    }
    return row.m_original.m_id;
  }

} // namespace

std::unique_ptr<FilteringSystem> MessagesForFiltersModel::createFilteringSystem(Feed* feed, ServiceRoot* account) {
  auto filtering = std::make_unique<FilteringSystem>(FilteringSystem::FiteringUseCase::ExistingArticles,
                                                     feed,
                                                     account,
                                                     m_application->applicationPaths(),
                                                     m_application->database(),
                                                     m_application->icons(),
                                                     m_application->localization(),
                                                     m_application->guiNotifications());
  filtering->filterRun().setTotalCountOfFilters(1);
  filtering->filterRun().setIndexOfCurrentFilter(0);
  connect(&filtering->filterApp(), &FilterApp::logged, this, &MessagesForFiltersModel::scriptLogged);
  return filtering;
}

QHash<int, Message> MessagesForFiltersModel::storedMessages(const QList<int>& ids, bool full) const {
  const auto labels = m_resultAccount != nullptr && m_resultAccount->labelsNode() != nullptr
                        ? m_resultAccount->labelsNode()->getHashedLabels()
                        : QHash<QString, Label*>();
  return m_application->database()->worker()->read<QHash<int, Message>>([&](const QSqlDatabase& db) {
    QHash<int, Message> messages;
    constexpr int batch_size = 200;
    for (int start = 0; start < ids.size(); start += batch_size) {
      QStringList keys;
      for (int i = start; i < std::min(start + batch_size, int(ids.size())); ++i) {
        keys.append(QString::number(ids[i]));
      }
      const QString columns = full ? DatabaseQueries::messageTableAttributes().join(QSL(", "))
                                   : QSL("id, title, is_read, is_important, is_deleted, date_created, date_retrieved, "
                                         "score, url, author, custom_id, is_pdeleted, feed, account_id");
      SqlQuery query(db);
      query.exec(QSL("SELECT %1 FROM Messages WHERE id IN (%2);").arg(columns, keys.join(QL1C(','))));
      while (query.next()) {
        Message message;
        if (full) {
          message = Message::fromSqlQuery(query, labels);
        }
        else {
          message.m_id = query.value(0).toInt();
          message.m_title = query.value(1).toString();
          message.m_isRead = query.value(2).toBool();
          message.m_isImportant = query.value(3).toBool();
          message.m_isDeleted = query.value(4).toBool();
          message.m_created = QDateTime::fromMSecsSinceEpoch(query.value(5).toLongLong(), Qt::UTC);
          message.m_retrieved = QDateTime::fromMSecsSinceEpoch(query.value(6).toLongLong(), Qt::UTC);
          message.m_score = query.value(7).toDouble();
          message.m_url = query.value(8).toString();
          message.m_author = query.value(9).toString();
          message.m_customId = query.value(10).toString();
          message.m_isPdeleted = query.value(11).toBool();
          message.m_feedId = query.value(12).toInt();
          message.m_accountId = query.value(13).toInt();
        }
        messages.insert(message.m_id, message);
      }
    }
    return messages;
  });
}

std::optional<Message> MessagesForFiltersModel::storedMessage(int id) const {
  const auto messages = storedMessages({id}, true);
  return messages.contains(id) ? std::optional<Message>(messages.value(id)) : std::nullopt;
}

void MessagesForFiltersModel::refreshStoredRows(int first_row, int count) {
  QList<int> ids;
  for (int i = first_row; i < first_row + count; ++i) {
    ids.append(storedIdentity(m_messages[i]));
  }
  const auto stored = storedMessages(ids, false);
  for (int i = first_row; i < first_row + count; ++i) {
    auto& row = m_messages[i];
    const int stored_id = storedIdentity(row);
    row.m_hasStoredValues = stored.contains(stored_id);
    if (row.m_hasStoredValues) {
      row.m_expectedStored = stored.value(stored_id);
    }
  }
}

void MessagesForFiltersModel::processFeeds(MessageFilter* filter,
                                           ServiceRoot* account,
                                           const QList<RootItem*>& checked) {
  if (filter == nullptr || account == nullptr) {
    return;
  }
  beginResetModel();
  m_messages.clear();
  m_error.clear();
  m_preview = false;
  m_showStoredValues = true;
  m_resultAccount = account;
  int completed_feeds = 0;
  int requested_feeds = 0;
  int active_row = -1;
  int active_feed_start = 0;
  int active_feed_count = 0;
  for (const auto* item : checked) {
    requested_feeds += item->kind() == RootItem::Kind::Feed;
  }

  try {
    for (RootItem* item : checked) {
      if (item->kind() != RootItem::Kind::Feed) {
        continue;
      }
      auto filtering = createFilteringSystem(item->toFeed(), account);
      const auto labels =
        account->labelsNode() != nullptr ? account->labelsNode()->getHashedLabels() : QHash<QString, Label*>();
      QList<Message> messages = m_application->database()->worker()->read<QList<Message>>([&](const QSqlDatabase& db) {
        return DatabaseQueries::getUndeletedMessagesForFeed(db, item->id(), labels);
      });
      const int first_row = m_messages.size();
      active_feed_start = first_row;
      active_feed_count = messages.size();
      for (const auto& message : messages) {
        MessageBackupAndOriginal row;
        row.m_original = compactMessage(message);
        row.m_filtered = row.m_original;
        row.m_expectedStored = row.m_original;
        row.m_compact = true;
        m_messages.append(row);
      }

      QList<Message> read_messages, important_messages, accepted;
      QList<int> accepted_rows;
      for (int i = 0; i < messages.size(); ++i) {
        active_row = first_row + i;
        auto& row = m_messages[active_row];
        Message& message = messages[i];
        Message original = snapshotForFiltering(message);
        filtering->setMessage(&message);
        row.m_decision = filtering->filterMessage(*filter);
        row.m_filtered = compactMessage(message);

        if (*row.m_decision == FilterMessage::FilteringAction::Purge) {
          m_application->database()->worker()->write([&](const QSqlDatabase& db) {
            DatabaseQueries::purgeMessage(db, message.m_id);
          });
          row.m_purged = true;
          row.m_hasStoredValues = false;
          filtering->removeDuplicateCandidate(message.m_id);
        }
        else if (*row.m_decision == FilterMessage::FilteringAction::Accept) {
          filtering->filterRun().incrementNumberOfAcceptedMessages();
        }

        // Retain existing automatic service-state handling, including rejected results.
        filtering->compareAndWriteArticleStates(&original, &message, read_messages, important_messages);
        if (*row.m_decision == FilterMessage::FilteringAction::Accept) {
          accepted_rows.append(active_row);
          accepted.append(message);
        }
      }
      active_row = -1;
      filtering->setMessage(nullptr);
      filtering->pushMessageStatesToServices(read_messages, important_messages, item, account);

      const auto report = account->updateMessages(accepted, item->toFeed(), true, MessageUpdateDetail::Detailed);
      QHash<int, Message> saved_messages;
      for (const auto& saved : report.m_all) {
        saved_messages.insert(saved.m_id, compactMessage(saved));
      }
      for (const auto& receipt : report.m_results) {
        if (receipt.m_inputIndex < 0 || receipt.m_inputIndex >= accepted_rows.size()) {
          continue;
        }
        auto& row = m_messages[accepted_rows[receipt.m_inputIndex]];
        row.m_saveResult = receipt;
        if (saved_messages.contains(receipt.m_id)) {
          row.m_expectedStored = saved_messages.value(receipt.m_id);
        }
      }
      m_error = report.m_error;
      refreshStoredRows(first_row, messages.size());
      if (!report.succeeded()) {
        break;
      }
      ++completed_feeds;
    }
  }
  catch (...) {
    MessageUpdateReport failure;
    failure.captureFailure();
    if (m_error.isEmpty()) {
      m_error = failure.m_error;
    }
    if (active_row >= 0) {
      m_messages[active_row].m_error = m_error;
    }
    try {
      refreshStoredRows(active_feed_start, active_feed_count);
    }
    catch (...) {
      emit scriptLogged(tr("Could not refresh stored values after the incomplete run."));
    }
  }
  finishRun(completed_feeds, requested_feeds);
}

void MessagesForFiltersModel::testFilter(MessageFilter* filter, ServiceRoot* account) {
  if (filter == nullptr || account == nullptr) {
    return;
  }
  beginResetModel();
  m_error.clear();
  m_preview = true;
  m_showStoredValues = false;
  m_resultAccount = account;
  QHash<int, Feed*> feeds;
  for (Feed* feed : account->getSubTreeFeeds()) {
    feeds.insert(feed->id(), feed);
  }
  QList<int> feed_order;
  QHash<int, QList<int>> rows_by_feed;
  int active_row = -1;
  int completed_feeds = 0;

  try {
    QList<int> compact_ids;
    QHash<int, int> compact_id_by_row;
    // Clear the entire previous result before any reload or feed lookup can fail.
    for (int i = 0; i < m_messages.size(); ++i) {
      auto& row = m_messages[i];
      if (row.m_compact) {
        const int stored_id = storedIdentity(row);
        compact_ids.append(stored_id);
        compact_id_by_row.insert(i, stored_id);
      }
      row.m_decision.reset();
      row.m_saveResult.reset();
      row.m_error.clear();
      row.m_purged = false;
      row.m_hasStoredValues = !row.m_compact;
      row.m_filtered = snapshotForFiltering(row.m_original);
      row.m_expectedStored = snapshotForFiltering(row.m_original);
    }
    const auto restored = storedMessages(compact_ids, true);
    for (int i = 0; i < m_messages.size(); ++i) {
      active_row = i;
      auto& row = m_messages[i];
      if (row.m_compact) {
        const int stored_id = compact_id_by_row.value(i);
        if (!restored.contains(stored_id)) {
          MessageWriteResult missing;
          missing.m_id = stored_id;
          missing.m_outcome = MessageWriteOutcome::Skipped;
          missing.m_labelsChecked = true;
          row.m_hasStoredValues = false;
          missing.m_decision.m_reason = MessageSaveReason::MissingArticle;
          row.m_saveResult = missing;
          continue;
        }
        row.m_original = snapshotForFiltering(restored.value(stored_id));
        row.m_compact = false;
        // Ordinary rows were reset above; only reloaded rows need new snapshots.
        row.m_filtered = snapshotForFiltering(row.m_original);
        row.m_expectedStored = snapshotForFiltering(row.m_original);
      }
      row.m_hasStoredValues = true;
      const int feed_id = row.m_original.m_feedId;
      if (!feeds.contains(feed_id)) {
        throw ApplicationException(tr("The feed for article '%1' is no longer available.").arg(row.m_original.m_title));
      }
      if (!rows_by_feed.contains(feed_id)) {
        feed_order.append(feed_id);
      }
      rows_by_feed[feed_id].append(i);
    }
    active_row = -1;
    for (int feed_id : feed_order) {
      auto filtering = createFilteringSystem(feeds.value(feed_id), account);
      QList<Message> accepted;
      QList<int> accepted_rows;
      QList<Message> read_messages, important_messages;
      for (int row_index : rows_by_feed.value(feed_id)) {
        active_row = row_index;
        auto& row = m_messages[row_index];
        filtering->setMessage(&row.m_filtered);
        row.m_decision = filtering->filterMessage(*filter);
        if (*row.m_decision == FilterMessage::FilteringAction::Purge) {
          row.m_hasStoredValues = false;
        }
        if (*row.m_decision == FilterMessage::FilteringAction::Accept) {
          filtering->filterRun().incrementNumberOfAcceptedMessages();
        }
        filtering->compareAndWriteArticleStates(&row.m_original, &row.m_filtered, read_messages, important_messages);
        if (*row.m_decision == FilterMessage::FilteringAction::Accept) {
          accepted_rows.append(row_index);
          accepted.append(row.m_filtered);
        }
      }
      active_row = -1;
      filtering->setMessage(nullptr);
      const auto previews = DatabaseQueries::previewMessageUpdates(m_application->database(),
                                                                   m_application->settings(),
                                                                   accepted,
                                                                   feeds.value(feed_id));
      for (const auto& preview : previews) {
        if (preview.m_inputIndex < 0 || preview.m_inputIndex >= accepted_rows.size()) {
          continue;
        }
        auto& row = m_messages[accepted_rows[preview.m_inputIndex]];
        row.m_hasStoredValues = preview.m_hasStoredMessage || preview.m_decision.m_action != MessageSaveAction::Skip;
        row.m_expectedStored = snapshotForFiltering(preview.m_expectedStored);
        MessageWriteResult result;
        result.m_inputIndex = preview.m_inputIndex;
        result.m_id = preview.m_id;
        result.m_decision = preview.m_decision;
        result.m_outcome = preview.m_decision.m_action == MessageSaveAction::Insert   ? MessageWriteOutcome::Inserted
                           : preview.m_decision.m_action == MessageSaveAction::Update ? MessageWriteOutcome::Updated
                                                                                      : MessageWriteOutcome::Skipped;
        result.m_labelsChecked = true;
        result.m_labelsChanged = preview.m_labelsChanged;
        row.m_saveResult = result;
      }
      ++completed_feeds;
    }
  }
  catch (...) {
    MessageUpdateReport failure;
    failure.captureFailure();
    m_error = failure.m_error;
    if (active_row >= 0) {
      m_messages[active_row].m_error = m_error;
    }
  }
  finishRun(completed_feeds, feed_order.size());
}

void MessagesForFiltersModel::finishRun(int completed_feeds, int requested_feeds) {
  int updated = 0, inserted = 0, skipped = 0, ignored = 0, purged = 0, labels = 0, failed = 0, pending = 0;
  int unconfirmed_labels = 0;
  for (const auto& row : m_messages) {
    if (row.m_saveResult) {
      const auto& saved = *row.m_saveResult;
      updated += saved.m_outcome == MessageWriteOutcome::Updated;
      inserted += saved.m_outcome == MessageWriteOutcome::Inserted;
      skipped += saved.m_outcome == MessageWriteOutcome::Skipped;
      failed += saved.m_outcome == MessageWriteOutcome::Failed;
      pending += saved.m_outcome == MessageWriteOutcome::NotAttempted;
      labels += saved.m_labelsChecked && saved.m_labelsChanged;
      unconfirmed_labels += !saved.m_labelsChecked;
    }
    else if (row.m_purged || (m_preview && row.m_decision == FilterMessage::FilteringAction::Purge)) {
      ++purged;
    }
    else if (!row.m_error.isEmpty()) {
      ++failed;
    }
    else if (row.m_decision == FilterMessage::FilteringAction::Ignore) {
      ++ignored;
    }
    else {
      ++pending;
    }
  }
  m_summary = m_preview ? tr("Preview of %1 loaded articles in %2 of %3 feeds: %4 would update, %5 would insert, %6 "
                             "skipped, %7 ignored, %8 would purge.")
                        : tr("Processing results for %1 articles in %2 of %3 feeds: %4 updated, %5 inserted, %6 "
                             "skipped, %7 ignored, %8 purged.");
  m_summary = m_summary.arg(m_messages.size())
                .arg(completed_feeds)
                .arg(requested_feeds)
                .arg(updated)
                .arg(inserted)
                .arg(skipped)
                .arg(ignored)
                .arg(purged);
  m_summary += m_preview ? tr(" Labels would change on %1 articles.").arg(labels)
                         : tr(" Labels changed on %1 articles.").arg(labels);
  if (unconfirmed_labels > 0) {
    m_summary += tr(" Label results remain unconfirmed for %1 articles.").arg(unconfirmed_labels);
  }
  if (!m_error.isEmpty()) {
    m_summary +=
      tr(" Run incomplete: %1 failed or unconfirmed, %2 not attempted. %3").arg(failed).arg(pending).arg(m_error);
  }
  endResetModel();
  emit summaryChanged(m_summary);
  emit resultsChanged(m_preview);
}

void MessagesForFiltersModel::setShowStoredValues(bool show_stored) {
  m_showStoredValues = show_stored;
  if (!m_messages.isEmpty()) {
    emit dataChanged(index(0, 0), index(m_messages.size() - 1, m_headerData.size() - 1));
  }
}

QString MessagesForFiltersModel::summaryText() const {
  return m_summary;
}

QString MessagesForFiltersModel::lastError() const {
  return m_error;
}

bool MessagesForFiltersModel::isPreview() const {
  return m_preview;
}

bool MessagesForFiltersModel::hasStoredValuesForRow(int row) const {
  return row >= 0 && row < m_messages.size() && m_messages[row].m_hasStoredValues;
}

Message* MessagesForFiltersModel::messageForRow(int row) {
  if (row < 0 || row >= m_messages.size()) {
    return nullptr;
  }
  return m_showStoredValues ? &m_messages[row].m_expectedStored : &m_messages[row].m_filtered;
}

const Message* MessagesForFiltersModel::constMessageForRow(int row) const {
  if (row < 0 || row >= m_messages.size()) {
    return nullptr;
  }
  return m_showStoredValues ? &m_messages[row].m_expectedStored : &m_messages[row].m_filtered;
}

const MessageBackupAndOriginal* MessagesForFiltersModel::resultForRow(int row) const {
  return row >= 0 && row < m_messages.size() ? &m_messages[row] : nullptr;
}

Message MessagesForFiltersModel::messageDetailsForRow(int row) const {
  const auto* selected = constMessageForRow(row);
  if (selected == nullptr || (m_showStoredValues && !hasStoredValuesForRow(row))) {
    return Message();
  }
  if (m_messages[row].m_compact && m_showStoredValues && !m_messages[row].m_purged) {
    const auto stored = storedMessage(selected->m_id);
    return stored ? *stored : Message();
  }
  return *selected;
}
