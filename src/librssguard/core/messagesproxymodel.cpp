// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagesproxymodel.h"

#include "core/messagesmodel.h"
#include "core/messagesmodelcache.h"
#include "definitions/globals.h"
#include "miscellaneous/regexfactory.h"
#include "miscellaneous/textfactory.h"

#include <QTimer>

MessagesProxyModel::MessagesProxyModel(MessagesModel* source_model, QObject* parent)
  : QSortFilterProxyModel(parent), m_sourceModel(source_model), m_filter(MessageListFilter::NoFiltering),
    m_additionalArticleId(0) {
  setObjectName(QSL("MessagesProxyModel"));

  initializeFilters();

  setSortRole(Qt::ItemDataRole::EditRole);
  setSortCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);

  setFilterKeyColumn(-1);
  setFilterRole(Qt::ItemDataRole::EditRole);

  setDynamicSortFilter(false);
  setSourceModel(m_sourceModel);
}

MessagesProxyModel::~MessagesProxyModel() {
  qDebugNN << LOGSEC_MESSAGEMODEL << "Destroying MessagesProxyModel instance.";
}

void MessagesProxyModel::initializeFilters() {
  m_filters[MessageListFilter::ShowUnread] = [this](int msg_row_index) {
    return !m_sourceModel->data(msg_row_index, MSG_DB_READ_INDEX, Qt::ItemDataRole::EditRole).toBool();
  };

  m_filters[MessageListFilter::ShowImportant] = [this](int msg_row_index) {
    return m_sourceModel->data(msg_row_index, MSG_DB_IMPORTANT_INDEX, Qt::ItemDataRole::EditRole).toBool();
  };

  m_filters[MessageListFilter::ShowToday] = [this](int msg_row_index) {
    const QDateTime current_dt = QDateTime::currentDateTime();
    const QDate current_d = current_dt.date();
    const QDateTime msg_created =
      TextFactory::parseDateTime(m_sourceModel->data(msg_row_index, MSG_DB_DCREATED_INDEX, Qt::ItemDataRole::EditRole)
                                   .value<qint64>());

    return current_d.startOfDay() <= msg_created && msg_created <= current_d.endOfDay();
  };

  m_filters[MessageListFilter::ShowYesterday] = [this](int msg_row_index) {
    const QDateTime current_dt = QDateTime::currentDateTime();
    const QDate current_d = current_dt.date();
    const QDateTime msg_created =
      TextFactory::parseDateTime(m_sourceModel->data(msg_row_index, MSG_DB_DCREATED_INDEX, Qt::ItemDataRole::EditRole)
                                   .value<qint64>());

    return current_d.addDays(-1).startOfDay() <= msg_created && msg_created <= current_d.addDays(-1).endOfDay();
  };

  m_filters[MessageListFilter::ShowLast24Hours] = [this](int msg_row_index) {
    const QDateTime current_dt = QDateTime::currentDateTime();
    const QDateTime msg_created =
      TextFactory::parseDateTime(m_sourceModel->data(msg_row_index, MSG_DB_DCREATED_INDEX, Qt::ItemDataRole::EditRole)
                                   .value<qint64>());

    return current_dt.addSecs(-24 * 60 * 60) <= msg_created && msg_created <= current_dt;
  };

  m_filters[MessageListFilter::ShowLast48Hours] = [this](int msg_row_index) {
    const QDateTime current_dt = QDateTime::currentDateTime();
    const QDateTime msg_created =
      TextFactory::parseDateTime(m_sourceModel->data(msg_row_index, MSG_DB_DCREATED_INDEX, Qt::ItemDataRole::EditRole)
                                   .value<qint64>());

    return current_dt.addSecs(-48 * 60 * 60) <= msg_created && msg_created <= current_dt;
  };

  m_filters[MessageListFilter::ShowThisWeek] = [this](int msg_row_index) {
    const QDateTime current_dt = QDateTime::currentDateTime();
    const QDate current_d = current_dt.date();
    const QDateTime msg_created =
      TextFactory::parseDateTime(m_sourceModel->data(msg_row_index, MSG_DB_DCREATED_INDEX, Qt::ItemDataRole::EditRole)
                                   .value<qint64>());

    return current_d.year() == msg_created.date().year() && current_d.weekNumber() == msg_created.date().weekNumber();
  };

  m_filters[MessageListFilter::ShowLastWeek] = [this](int msg_row_index) {
    const QDateTime current_dt = QDateTime::currentDateTime();
    const QDate current_d = current_dt.date();
    const QDateTime msg_created =
      TextFactory::parseDateTime(m_sourceModel->data(msg_row_index, MSG_DB_DCREATED_INDEX, Qt::ItemDataRole::EditRole)
                                   .value<qint64>());

    return current_d.addDays(-7).year() == msg_created.date().year() &&
           current_d.addDays(-7).weekNumber() == msg_created.date().weekNumber();
  };

  m_filters[MessageListFilter::ShowOnlyWithAttachments] = [this](int msg_row_index) {
    const bool msg_has_enclosures =
      m_sourceModel->data(msg_row_index, MSG_DB_HAS_ENCLOSURES, Qt::ItemDataRole::EditRole).toBool();

    return msg_has_enclosures;
  };

  m_filters[MessageListFilter::ShowOnlyWithScore] = [this](int msg_row_index) {
    const int msg_score = m_sourceModel->data(msg_row_index, MSG_DB_SCORE_INDEX, Qt::ItemDataRole::EditRole).toDouble();

    return msg_score > MSG_SCORE_MIN;
  };

  m_filterKeys = m_filters.keys();
}

bool MessagesProxyModel::filterAcceptsMessage(int msg_row_index) const {
  if (m_filter == MessageListFilter::NoFiltering) {
    return true;
  }
  else if (m_additionalArticleId > 0 &&
           m_sourceModel->data(msg_row_index, MSG_DB_ID_INDEX, Qt::ItemDataRole::EditRole).toInt() ==
             m_additionalArticleId) {
    return true;
  }

  for (MessageListFilter val : m_filterKeys) {
    if (Globals::hasFlag(m_filter, val) && m_filters[val](msg_row_index)) {
      return true;
    }
  }

  return false;
}

QModelIndex MessagesProxyModel::getNextPreviousImportantItemIndex(int default_row) const {
  const bool started_from_zero = default_row == 0;
  QModelIndex next_index = getNextImportantItemIndex(default_row, rowCount() - 1);

  // There is no next message, check previous.
  if (!next_index.isValid() && !started_from_zero) {
    next_index = getNextImportantItemIndex(0, default_row - 1);
  }

  return next_index;
}

QModelIndex MessagesProxyModel::getNextPreviousUnreadItemIndex(int default_row) const {
  const bool started_from_zero = default_row == 0;
  QModelIndex next_index = getNextUnreadItemIndex(default_row, rowCount() - 1);

  // There is no next message, check previous.
  if (!next_index.isValid() && !started_from_zero) {
    next_index = getNextUnreadItemIndex(0, default_row - 1);
  }

  return next_index;
}

QModelIndex MessagesProxyModel::indexFromMessage(const Message& msg) const {
  for (int i = 0; i < rowCount(); i++) {
    auto idx = index(i, 0);
    auto id =
      m_sourceModel->data(m_sourceModel->index(mapToSource(idx).row(), MSG_DB_ID_INDEX), Qt::ItemDataRole::EditRole)
        .toInt();

    if (id == msg.m_id) {
      return idx;
    }
  }

  return QModelIndex();
}

QModelIndex MessagesProxyModel::getNextImportantItemIndex(int default_row, int max_row) const {
  while (default_row <= max_row) {
    // Get info if the message is read or not.
    const QModelIndex proxy_index = index(default_row, MSG_DB_IMPORTANT_INDEX);
    const bool is_important =
      m_sourceModel->data(mapToSource(proxy_index).row(), MSG_DB_IMPORTANT_INDEX, Qt::ItemDataRole::EditRole).toInt() ==
      1;

    if (!is_important) {
      // We found unread message, mark it.
      return proxy_index;
    }
    else {
      default_row++;
    }
  }

  return QModelIndex();
}

QModelIndex MessagesProxyModel::getNextUnreadItemIndex(int default_row, int max_row) const {
  while (default_row <= max_row) {
    // Get info if the message is read or not.
    const QModelIndex proxy_index = index(default_row, MSG_DB_READ_INDEX);
    const bool is_read =
      m_sourceModel->data(mapToSource(proxy_index).row(), MSG_DB_READ_INDEX, Qt::ItemDataRole::EditRole).toInt() == 1;

    if (!is_read) {
      // We found unread message, mark it.
      return proxy_index;
    }
    else {
      default_row++;
    }
  }

  return QModelIndex();
}

bool MessagesProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const {
  Q_UNUSED(left)
  Q_UNUSED(right)

  // NOTE: Comparisons are done by SQL server itself, not client-side.
  return false;
}

bool MessagesProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const {
  // We want to show only regexped messages when "all" should be visible
  // and we want to show only regexped AND unread messages when unread should be visible.
  //
  // But also, we want to see messages which have their dirty states cached, because
  // otherwise they would just disappear from the list for example when batch marked as read
  // which is distracting.
  return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent) &&
         (m_sourceModel->cache()->containsData(source_row) || filterAcceptsMessage(source_row));
}

int MessagesProxyModel::additionalArticleId() const {
  return m_additionalArticleId;
}

void MessagesProxyModel::setAdditionalArticleId(int newAdditionalArticleId) {
  m_additionalArticleId = newAdditionalArticleId;
}

void MessagesProxyModel::setMessageListFilter(MessageListFilter filter) {
  m_filter = filter;
  m_additionalArticleId = 0;
}

QModelIndexList MessagesProxyModel::mapListFromSource(const QModelIndexList& indexes, bool deep) const {
  QModelIndexList mapped_indexes;

  for (const QModelIndex& index : indexes) {
    if (deep) {
      // Construct new source index.
      mapped_indexes << mapFromSource(m_sourceModel->index(index.row(), index.column()));
    }
    else {
      mapped_indexes << mapFromSource(index);
    }
  }

  return mapped_indexes;
}

QModelIndexList MessagesProxyModel::match(const QModelIndex& start,
                                          int role,
                                          const QVariant& entered_value,
                                          int hits,
                                          Qt::MatchFlags flags) const {
  QModelIndexList result;
  const int match_type = flags & 0x0F;
  const Qt::CaseSensitivity case_sensitivity = Qt::CaseSensitivity::CaseInsensitive;
  const bool wrap = Globals::hasFlag(flags, Qt::MatchFlag::MatchWrap);
  ;
  const bool all_hits = (hits == -1);
  QString entered_text;
  int from = start.row();
  int to = rowCount();

  for (int i = 0; (wrap && i < 2) || (!wrap && i < 1); i++) {
    for (int r = from; (r < to) && (all_hits || result.count() < hits); r++) {
      QModelIndex idx = index(r, start.column());

      if (!idx.isValid()) {
        continue;
      }

      QVariant item_value = m_sourceModel->data(mapToSource(idx).row(), MSG_DB_TITLE_INDEX, role);

      // QVariant based matching.
      if (match_type == Qt::MatchExactly) {
        if (entered_value == item_value) {
          result.append(idx);
        }
      }

      // QString based matching.
      else {
        if (entered_text.isEmpty()) {
          entered_text = entered_value.toString();
        }

        QString item_text = item_value.toString();

        switch (match_type) {
          case Qt::MatchFlag::MatchRegularExpression:
            if (QRegularExpression(entered_text,
                                   QRegularExpression::PatternOption::CaseInsensitiveOption |
                                     QRegularExpression::PatternOption::UseUnicodePropertiesOption)
                  .match(item_text)
                  .hasMatch()) {
              result.append(idx);
            }

            break;

          case Qt::MatchWildcard:
            if (QRegularExpression(RegexFactory::wildcardToRegularExpression(entered_text),
                                   QRegularExpression::PatternOption::CaseInsensitiveOption |
                                     QRegularExpression::PatternOption::UseUnicodePropertiesOption)
                  .match(item_text)
                  .hasMatch()) {
              result.append(idx);
            }

            break;

          case Qt::MatchStartsWith:
            if (item_text.startsWith(entered_text, case_sensitivity)) {
              result.append(idx);
            }

            break;

          case Qt::MatchEndsWith:
            if (item_text.endsWith(entered_text, case_sensitivity)) {
              result.append(idx);
            }

            break;

          case Qt::MatchFixedString:
            if (item_text.compare(entered_text, case_sensitivity) == 0) {
              result.append(idx);
            }

            break;

          case Qt::MatchContains:
          default:
            if (item_text.contains(entered_text, case_sensitivity)) {
              result.append(idx);
            }

            break;
        }
      }
    }

    // Prepare for the next iteration.
    from = 0;
    to = start.row();
  }

  return result;
}

void MessagesProxyModel::sort(int column, Qt::SortOrder order) {
  // NOTE: Ignore here, sort is done elsewhere (server-side).
  Q_UNUSED(column)
  Q_UNUSED(order)
}

QModelIndexList MessagesProxyModel::mapListToSource(const QModelIndexList& indexes) const {
  QModelIndexList source_indexes;
  source_indexes.reserve(indexes.size());

  for (const QModelIndex& index : indexes) {
    source_indexes << mapToSource(index);
  }

  return source_indexes;
}
