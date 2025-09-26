// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagesmodel.h"

#include "3rd-party/boolinq/boolinq.h"
#include "core/messagesmodelcache.h"
#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "gui/messagesview.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/skinfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/recyclebin.h"
#include "services/abstract/serviceroot.h"

#include <cmath>

#include <QPainter>
#include <QPainterPath>
#include <QSqlError>
#include <QSqlField>

#define RAD_COLOR 0, 180, 0

MessagesModel::MessagesModel(QObject* parent)
  : QSqlQueryModel(parent), m_view(nullptr), m_cache(new MessagesModelCache(this)),
    m_messageHighlighter(MessageHighlighter::NoHighlighting), m_customDateFormat(QString()),
    m_customTimeFormat(QString()), m_customFormatForDatesOnly(QString()), m_newerArticlesRelativeTime(-1),
    m_selectedItem(nullptr), m_unreadIconType(MessageUnreadIcon::Dot),
    m_multilineListItems(qApp->settings()->value(GROUP(Messages), SETTING(Messages::MultilineArticleList)).toBool()) {
  updateFeedIconsDisplay();
  updateDateFormat();

  setupFonts();
  setupIcons();
  setupHeaderData();
  loadMessages(nullptr);
}

MessagesModel::~MessagesModel() {
  qDebugNN << LOGSEC_MESSAGEMODEL << "Destroying MessagesModel instance.";
}

void MessagesModel::setupIcons() {
  m_favoriteIcon = qApp->icons()->fromTheme(QSL("mail-mark-important"));
  m_readIcon = qApp->icons()->fromTheme(QSL("mail-mark-read"));
  m_unreadIcon = m_unreadIconType == MessageUnreadIcon::Dot ? generateUnreadIcon()
                                                            : qApp->icons()->fromTheme(QSL("mail-mark-unread"));
  m_enclosuresIcon = qApp->icons()->fromTheme(QSL("mail-attachment"));

  for (int i = int(MSG_SCORE_MIN); i <= int(MSG_SCORE_MAX); i += 10) {
    m_scoreIcons.append(generateIconForScore(double(i)));
  }
}

QIcon MessagesModel::generateIconForScore(double score) {
  QPixmap pix(64, 64);
  QPainter paint(&pix);

  paint.setRenderHint(QPainter::RenderHint::Antialiasing);

  int level = std::min(MSG_SCORE_MAX, std::max(MSG_SCORE_MIN, std::floor(score / 10.0)));
  QPainterPath path;

  path.addRoundedRect(QRectF(2, 2, 60, 60), 5, 5);

  QPen pen(Qt::GlobalColor::black, 2);

  paint.setPen(pen);
  paint.fillPath(path, Qt::GlobalColor::white);
  paint.drawPath(path);

#if QT_VERSION >= 0x050D00 // Qt >= 5.13.0
  path.clear();
#else
  path = QPainterPath();
#endif

  paint.setPen(Qt::GlobalColor::transparent);

  int bar_height = 6 * level;

  path.addRoundedRect(QRectF(2, 64 - bar_height - 2, 60, bar_height), 5, 5);
  paint.fillPath(path, QColor::fromHsv(int(score), 200, 230));

  return pix;
}

QIcon MessagesModel::generateUnreadIcon() {
  QPointF center(64, 64);
  qreal radius = 32;

  QColor custom_clr = qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgInteresting).value<QColor>();

  if (!custom_clr.isValid()) {
    custom_clr = QColor(RAD_COLOR);
  }

  QRadialGradient grad(center, radius);
  grad.setColorAt(0.0, QColor(custom_clr.red(), custom_clr.green(), custom_clr.blue(), 255));
  grad.setColorAt(0.8, QColor(custom_clr.red(), custom_clr.green(), custom_clr.blue(), 0.8 * 255));
  grad.setColorAt(1.0, QColor(custom_clr.red(), custom_clr.green(), custom_clr.blue(), 0.0));

  QPen pen;
  pen.setWidth(96);
  pen.setBrush(grad);

  QPixmap pix(128, 128);
  pix.fill(Qt::GlobalColor::transparent);

  QPainter painter(&pix);
  painter.setRenderHint(QPainter::RenderHint::Antialiasing);
  painter.setPen(pen);
  painter.drawPoint(center);

  return QIcon(pix);
}

QString MessagesModel::descriptionOfUnreadIcon(MessageUnreadIcon type) {
  switch (type) {
    case MessagesModel::MessageUnreadIcon::Dot:
      return tr("dot");

    case MessagesModel::MessageUnreadIcon::Envelope:
      return tr("envelope");

    case MessagesModel::MessageUnreadIcon::FeedIcon:
      return tr("feed icon");

    default:
      return QString();
  }
}

MessagesView* MessagesModel::view() const {
  return m_view;
}

void MessagesModel::setView(MessagesView* new_view) {
  m_view = new_view;
}

MessagesModelCache* MessagesModel::cache() const {
  return m_cache;
}

void MessagesModel::repopulate(int additional_article_id) {
  m_cache->clear();

  QString statemnt = selectStatement(additional_article_id);

  setQuery(statemnt, m_db);

  if (lastError().isValid()) {
    qCriticalNN << LOGSEC_MESSAGEMODEL
                << "Error when setting new msg view query:" << QUOTE_W_SPACE_DOT(lastError().text());
    qCriticalNN << LOGSEC_MESSAGEMODEL << "Used SQL select statement:" << QUOTE_W_SPACE_DOT(statemnt);
  }

  /*
  while (canFetchMore()) {
    fetchMore();
  }
  */

  qDebugNN << LOGSEC_MESSAGEMODEL << "Repopulated model, SQL statement is now:\n" << QUOTE_W_SPACE_DOT(statemnt);
}

bool MessagesModel::setData(const QModelIndex& idx, const QVariant& value, int role) {
  Q_UNUSED(role)
  m_cache->setData(idx, value);

  emit dataChanged(index(idx.row(), 0), index(idx.row(), MSG_DB_LABELS_IDS));
  return true;
}

void MessagesModel::setupFonts() {
  QFont fon;

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::CustomizeListFont)).toBool()) {
    fon.fromString(qApp->settings()
                     ->value(GROUP(Messages), Messages::ListFont, Application::font("MessagesView").toString())
                     .toString());
  }
  else {
    fon = QApplication::font("MessagesView");
  }

  m_normalFont = fon;
  m_boldFont = m_normalFont;
  m_boldFont.setBold(true);
  m_normalStrikedFont = m_normalFont;
  m_boldStrikedFont = m_boldFont;
  m_normalStrikedFont.setStrikeOut(true);
  m_boldStrikedFont.setStrikeOut(true);
}

void MessagesModel::loadMessages(RootItem* item) {
  m_selectedItem = item;

  if (item == nullptr) {
    setFilter(QSL(DEFAULT_SQL_MESSAGES_FILTER));
  }
  else {
    if (!item->getParentServiceRoot()->loadMessagesForItem(item, this)) {
      setFilter(QSL(DEFAULT_SQL_MESSAGES_FILTER));
      qCriticalNN << LOGSEC_MESSAGEMODEL << "Loading of messages from item '" << item->title() << "' failed.";
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Loading of articles from item '%1' failed").arg(item->title()),
                            tr("Loading of articles failed, maybe messages could not be downloaded."),
                            QSystemTrayIcon::MessageIcon::Critical});
    }
  }

  repopulate();
}

bool MessagesModel::setMessageImportantById(int id, RootItem::Importance important) {
  for (int i = 0; i < rowCount(); i++) {
    int found_id = data(i, MSG_DB_ID_INDEX, Qt::EditRole).toInt();

    if (found_id == id) {
      bool set = setData(index(i, MSG_DB_IMPORTANT_INDEX), int(important));

      if (set) {
        emit dataChanged(index(i, 0), index(i, MSG_DB_LABELS_IDS));
      }

      return set;
    }
  }

  return false;
}

void MessagesModel::highlightMessages(MessagesModel::MessageHighlighter highlighter) {
  m_messageHighlighter = highlighter;
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

int MessagesModel::messageId(int row_index) const {
  return data(row_index, MSG_DB_ID_INDEX, Qt::EditRole).toInt();
}

RootItem::Importance MessagesModel::messageImportance(int row_index) const {
  return RootItem::Importance(data(row_index, MSG_DB_IMPORTANT_INDEX, Qt::EditRole).toInt());
}

RootItem* MessagesModel::loadedItem() const {
  return m_selectedItem;
}

void MessagesModel::updateDateFormat() {
  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::UseCustomDate)).toBool()) {
    m_customDateFormat = qApp->settings()->value(GROUP(Messages), SETTING(Messages::CustomDateFormat)).toString();
  }
  else {
    m_customDateFormat = QString();
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::UseCustomTime)).toBool()) {
    m_customTimeFormat = qApp->settings()->value(GROUP(Messages), SETTING(Messages::CustomTimeFormat)).toString();
  }
  else {
    m_customTimeFormat = QString();
  }

  if (qApp->settings()->value(GROUP(Messages), SETTING(Messages::UseCustomFormatForDatesOnly)).toBool()) {
    m_customFormatForDatesOnly =
      qApp->settings()->value(GROUP(Messages), SETTING(Messages::CustomFormatForDatesOnly)).toString();
  }
  else {
    m_customFormatForDatesOnly = QString();
  }

  m_newerArticlesRelativeTime =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::RelativeTimeForNewerArticles)).toInt();
}

void MessagesModel::updateFeedIconsDisplay() {
  m_unreadIconType =
    MessageUnreadIcon(qApp->settings()->value(GROUP(Messages), SETTING(Messages::UnreadIconType)).toInt());
}

void MessagesModel::reloadWholeLayout() {
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

Message MessagesModel::messageAt(int row_index) const {
  return Message::fromSqlRecord(m_cache->containsData(row_index) ? m_cache->record(row_index) : record(row_index));
}

void MessagesModel::setupHeaderData() {
  m_headerData <<

    /*: Tooltip for ID of message.*/ tr("Id") <<
    /*: Tooltip for "read" column in msg list.*/ tr("Read") <<
    /*: Tooltip for "important" column in msg list.*/ tr("Important") <<
    /*: Tooltip for "deleted" column in msg list.*/ tr("Deleted") <<
    /*: Tooltip for "pdeleted" column in msg list.*/ tr("Permanently deleted") <<
    /*: Tooltip for custom ID of feed of message.*/ tr("Feed ID") <<
    /*: Tooltip for title of message.*/ tr("Title") <<
    /*: Tooltip for url of message.*/ tr("URL") <<
    /*: Tooltip for author of message.*/ tr("Author") <<
    /*: Tooltip for creation date of message.*/ tr("Date") <<
    /*: Tooltip for contents of message.*/ tr("Contents") <<
    /*: Tooltip for attachments of message.*/ tr("Attachments") <<
    /*: Tooltip for score of message.*/ tr("Score") <<
    /*: Tooltip for account ID of message.*/ tr("Account ID") <<
    /*: Tooltip for custom ID of message.*/ tr("Custom ID") <<
    /*: Tooltip for custom hash string of message.*/ tr("Custom hash") <<
    /*: Tooltip for name of feed for message.*/ tr("Feed") <<
    /*: Tooltip for indication whether article is RTL or not.*/ tr("RTL") <<
    /*: Tooltip for indication of presence of enclosures.*/ tr("Has enclosures") <<
    /*: Tooltip for indication of labels of message.*/ tr("Assigned labels") <<
    /*: Tooltip for indication of label IDs of message.*/ tr("Assigned label IDs");

  m_tooltipData << tr("ID of the article.") << tr("Is article read?") << tr("Is article important?")
                << tr("Is article deleted?") << tr("Is article permanently deleted from recycle bin?")
                << tr("ID of feed which this article belongs to.") << tr("Title of the article.")
                << tr("Url of the article.") << tr("Author of the article.") << tr("Creation date of the article.")
                << tr("Contents of the article.") << tr("List of attachments.") << tr("Score of the article.")
                << tr("Account ID of the article.") << tr("Custom ID of the article.")
                << tr("Custom hash of the article.") << tr("Name of feed of the article.")
                << tr("Layout direction of the article") << tr("Indication of enclosures presence within the article.")
                << tr("Labels assigned to the article.") << tr("Label IDs assigned to the article.");
}

Qt::ItemFlags MessagesModel::flags(const QModelIndex& index) const {
  Q_UNUSED(index)
  return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren;
}

QList<Message> MessagesModel::messagesAt(const QList<int>& row_indices) const {
  QList<Message> msgs;
  msgs.reserve(row_indices.size());

  for (int idx : row_indices) {
    msgs << messageAt(idx);
  }

  return msgs;
}

QVariant MessagesModel::data(int row, int column, int role) const {
  return data(index(row, column), role);
}

QVariant MessagesModel::data(const QModelIndex& idx, int role) const {
  // This message is not in cache, return real data from live query.
  switch (role) {
    // Human readable data for viewing.
    case Qt::ItemDataRole::DisplayRole: {
      int index_column = idx.column();

      if (index_column == MSG_DB_DCREATED_INDEX) {
        QDateTime utc_dt =
          TextFactory::parseDateTime(QSqlQueryModel::data(idx, Qt::ItemDataRole::EditRole).value<qint64>());
        QDateTime dt = utc_dt.toLocalTime();

        if (dt.date() == QDate::currentDate() && !m_customTimeFormat.isEmpty()) {
          return dt.toString(m_customTimeFormat);
        }
        else if (!m_customFormatForDatesOnly.isEmpty() && utc_dt.time().hour() == 0 && utc_dt.time().minute() == 0 &&
                 utc_dt.time().second() == 0) {
          return dt.toString(m_customFormatForDatesOnly);
        }
        else if (m_newerArticlesRelativeTime > 0 &&
                 dt.daysTo(QDateTime::currentDateTime()) <= m_newerArticlesRelativeTime) {
          auto secs_difference = dt.secsTo(QDateTime::currentDateTime());

          if (secs_difference >= 2419200) {
            // More than 1 week.
            return tr("%n months ago", nullptr, secs_difference / 2419200);
          }
          else if (secs_difference >= 604800) {
            // More than 1 week.
            return tr("%n weeks ago", nullptr, secs_difference / 604800);
          }
          else if (secs_difference >= 172800) {
            // At least 2 days.
            return tr("%n days ago", nullptr, secs_difference / 86400);
          }
          else if (secs_difference >= 86400) {
            // 1 day.
            return tr("yesterday");
          }
          else if (secs_difference >= 3600) {
            // Less than a day.
            return tr("%n hours ago", nullptr, secs_difference / 3600);
          }
          else if (secs_difference >= 120) {
            // Less then 1 hour ago.
            return tr("%n minutes ago", nullptr, secs_difference / 60);
          }
          else {
            return tr("just now");
          }
        }
        else if (m_customDateFormat.isEmpty()) {
          return QLocale().toString(dt, QLocale::FormatType::ShortFormat);
        }
        else {
          return dt.toString(m_customDateFormat);
        }
      }
      else if (index_column == MSG_DB_FEED_TITLE_INDEX) {
        // Trim feed title.
        return data(idx, Qt::ItemDataRole::EditRole).toString().simplified();
      }
      else if (index_column == MSG_DB_CONTENTS_INDEX) {
        // Do not display full contents here.
        QString contents = data(idx, Qt::ItemDataRole::EditRole).toString().mid(0, 64).simplified() + QL1S("...");

        return contents;
      }
      else if (index_column == MSG_DB_LABELS_IDS) {
        return m_cache->containsData(idx.row()) ? m_cache->data(idx) : QSqlQueryModel::data(idx, role);
      }
      else if (index_column == MSG_DB_AUTHOR_INDEX) {
        const QString author_name = QSqlQueryModel::data(idx, role).toString();

        return author_name.isEmpty() ? QSL("-") : author_name;
      }
      else if (index_column != MSG_DB_IMPORTANT_INDEX && index_column != MSG_DB_READ_INDEX &&
               index_column != MSG_DB_HAS_ENCLOSURES && index_column != MSG_DB_SCORE_INDEX) {
        return QSqlQueryModel::data(idx, role);
      }
      else {
        return QVariant();
      }
    }

    case TEXT_DIRECTION_ROLE: {
      int index_column = idx.column();

      if (index_column != MSG_DB_TITLE_INDEX && index_column != MSG_DB_FEED_TITLE_INDEX &&
          index_column != MSG_DB_AUTHOR_INDEX) {
        return Qt::LayoutDirection::LayoutDirectionAuto;
      }
      else {
        RtlBehavior rtl_mode =
          (m_cache->containsData(idx.row())
             ? m_cache->data(index(idx.row(), MSG_DB_FEED_IS_RTL_INDEX))
             : QSqlQueryModel::data(index(idx.row(), MSG_DB_FEED_IS_RTL_INDEX), Qt::ItemDataRole::EditRole))
            .value<RtlBehavior>();

        return (rtl_mode == RtlBehavior::Everywhere || rtl_mode == RtlBehavior::EverywhereExceptFeedList)
                 ? Qt::LayoutDirection::RightToLeft
                 : Qt::LayoutDirection::LayoutDirectionAuto;
      }
    }

    case LOWER_TITLE_ROLE:
      return m_cache->containsData(idx.row())
               ? m_cache->data(idx).toString().toLower()
               : QSqlQueryModel::data(idx, Qt::ItemDataRole::EditRole).toString().toLower();

    case Qt::ItemDataRole::EditRole:
      return m_cache->containsData(idx.row()) ? m_cache->data(idx) : QSqlQueryModel::data(idx, role);

    case Qt::ItemDataRole::ToolTipRole: {
      if (!qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::EnableTooltipsFeedsMessages)).toBool()) {
        return QVariant();
      }
      else {
        if (idx.column() == MSG_DB_SCORE_INDEX) {
          return data(idx, Qt::ItemDataRole::EditRole);
        }
        else if (idx.column() == MSG_DB_URL_INDEX) {
          return TextFactory::shorten(data(idx, Qt::ItemDataRole::DisplayRole).toString(), TEXT_TOOLTIP_LIMIT);
        }
        else if (idx.column() == MSG_DB_DCREATED_INDEX) {
          return qApp->localization()
            ->loadedLocale()
            .toString(QDateTime::fromMSecsSinceEpoch(data(idx, Qt::ItemDataRole::EditRole).value<qint64>())
                        .toLocalTime(),
                      QLocale::FormatType::LongFormat);
        }
        else if (idx.column() == MSG_DB_READ_INDEX && m_unreadIconType == MessageUnreadIcon::FeedIcon) {
          return data(idx.row(), MSG_DB_FEED_TITLE_INDEX, Qt::ItemDataRole::EditRole);
        }
        else {
          return data(idx, Qt::ItemDataRole::DisplayRole);
        }
      }
    }

    case Qt::ItemDataRole::FontRole: {
      QModelIndex idx_read = index(idx.row(), MSG_DB_READ_INDEX);
      QVariant data_read = data(idx_read, Qt::ItemDataRole::EditRole);
      const bool is_bin = qobject_cast<RecycleBin*>(loadedItem()) != nullptr;
      bool is_deleted;

      if (is_bin) {
        QModelIndex idx_del = index(idx.row(), MSG_DB_PDELETED_INDEX);

        is_deleted = data(idx_del, Qt::ItemDataRole::EditRole).toBool();
      }
      else {
        QModelIndex idx_del = index(idx.row(), MSG_DB_DELETED_INDEX);

        is_deleted = data(idx_del, Qt::ItemDataRole::EditRole).toBool();
      }

      const bool striked = is_deleted;

      if (data_read.toBool()) {
        return striked ? m_normalStrikedFont : m_normalFont;
      }
      else {
        return striked ? m_boldStrikedFont : m_boldFont;
      }
    }

    case Qt::ItemDataRole::ForegroundRole:
    case HIGHLIGHTED_FOREGROUND_TITLE_ROLE: {
      if (Globals::hasFlag(m_messageHighlighter, MessageHighlighter::HighlightImportant)) {
        QModelIndex idx_important = index(idx.row(), MSG_DB_IMPORTANT_INDEX);
        QVariant dta = m_cache->containsData(idx_important.row()) ? m_cache->data(idx_important)
                                                                  : QSqlQueryModel::data(idx_important);

        if (dta.toInt() == 1) {
          return qApp->skins()->colorForModel(role == Qt::ItemDataRole::ForegroundRole
                                                ? SkinEnums::PaletteColors::FgInteresting
                                                : SkinEnums::PaletteColors::FgSelectedInteresting);
        }
      }

      if (Globals::hasFlag(m_messageHighlighter, MessageHighlighter::HighlightUnread)) {
        QModelIndex idx_read = index(idx.row(), MSG_DB_READ_INDEX);
        QVariant dta = m_cache->containsData(idx_read.row()) ? m_cache->data(idx_read) : QSqlQueryModel::data(idx_read);

        if (dta.toInt() == 0) {
          return qApp->skins()->colorForModel(role == Qt::ItemDataRole::ForegroundRole
                                                ? SkinEnums::PaletteColors::FgInteresting
                                                : SkinEnums::PaletteColors::FgSelectedInteresting);
        }
      }

      return QVariant();
    }

    case Qt::ItemDataRole::SizeHintRole: {
      if (!m_multilineListItems || m_view == nullptr || m_view->isColumnHidden(idx.column()) ||
          idx.column() != MSG_DB_TITLE_INDEX) {
        return {};
      }
      else {
        auto wd = m_view->columnWidth(idx.column());
        QString str = data(idx, Qt::ItemDataRole::DisplayRole).toString();

        if (str.simplified().isEmpty()) {
          return {};
        }

        QFontMetrics fm(data(idx, Qt::ItemDataRole::FontRole).value<QFont>());
        auto rct =
          fm.boundingRect(QRect(QPoint(0, 0), QPoint(wd - 5, 100000)),
                          Qt::TextFlag::TextWordWrap | Qt::AlignmentFlag::AlignLeft | Qt::AlignmentFlag::AlignVCenter,
                          str)
            .size();

        return rct;
      }
    }

    case Qt::ItemDataRole::DecorationRole: {
      const int index_column = idx.column();

      if (index_column == MSG_DB_READ_INDEX) {
        if (m_unreadIconType == MessageUnreadIcon::FeedIcon && m_selectedItem != nullptr) {
          QModelIndex idx_feedid = index(idx.row(), MSG_DB_FEED_CUSTOM_ID_INDEX);
          QVariant dta =
            m_cache->containsData(idx_feedid.row()) ? m_cache->data(idx_feedid) : QSqlQueryModel::data(idx_feedid);
          QString feed_custom_id = dta.toString();

          // TODO: Very slow and repeats itself.
          auto acc = m_selectedItem->getParentServiceRoot()->feedIconForMessage(feed_custom_id);

          if (acc.isNull()) {
            return qApp->icons()->fromTheme(QSL("application-rss+xml"));
          }
          else {
            return acc;
          }
        }
        else {
          QModelIndex idx_read = index(idx.row(), MSG_DB_READ_INDEX);
          QVariant dta =
            m_cache->containsData(idx_read.row()) ? m_cache->data(idx_read) : QSqlQueryModel::data(idx_read);

          if (m_unreadIconType == MessageUnreadIcon::Dot) {
            return dta.toInt() == 1 ? QVariant() : m_unreadIcon;
          }
          else {
            return dta.toInt() == 1 ? m_readIcon : m_unreadIcon;
          }
        }
      }
      else if (index_column == MSG_DB_IMPORTANT_INDEX) {
        QModelIndex idx_important = index(idx.row(), MSG_DB_IMPORTANT_INDEX);
        QVariant dta = m_cache->containsData(idx_important.row()) ? m_cache->data(idx_important)
                                                                  : QSqlQueryModel::data(idx_important);

        return dta.toInt() == 1 ? m_favoriteIcon : QVariant();
      }
      else if (index_column == MSG_DB_HAS_ENCLOSURES) {
        QModelIndex idx_enc = index(idx.row(), MSG_DB_HAS_ENCLOSURES);
        QVariant dta = QSqlQueryModel::data(idx_enc);

        return dta.toBool() ? m_enclosuresIcon : QVariant();
      }
      else if (index_column == MSG_DB_SCORE_INDEX) {
        QVariant dta = QSqlQueryModel::data(idx);
        int level = std::min(MSG_SCORE_MAX, std::max(MSG_SCORE_MIN, std::floor(dta.toDouble() / 10.0)));

        return m_scoreIcons.at(level);
      }
      else {
        return QVariant();
      }
    }

    default:
      return QVariant();
  }
}

bool MessagesModel::switchMessageReadUnread(int row_index) {
  RootItem::ReadStatus current_read =
    RootItem::ReadStatus(data(row_index, MSG_DB_READ_INDEX, Qt::ItemDataRole::EditRole).toInt());

  return setMessageRead(row_index,
                        current_read == RootItem::ReadStatus::Read ? RootItem::ReadStatus::Unread
                                                                   : RootItem::ReadStatus::Read);
}

bool MessagesModel::setMessageRead(int row_index, RootItem::ReadStatus read) {
  if (data(row_index, MSG_DB_READ_INDEX, Qt::ItemDataRole::EditRole).toInt() == int(read)) {
    // Read status is the same is the one currently set.
    // In that case, no extra work is needed.
    return true;
  }

  Message message = messageAt(row_index);

  if (!m_selectedItem->getParentServiceRoot()->onBeforeSetMessagesRead(m_selectedItem, {message}, read)) {
    // Cannot change read status of the item. Abort.
    return false;
  }

  // Rewrite "visible" data in the model.
  bool working_change = setData(index(row_index, MSG_DB_READ_INDEX), int(read));

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebugNN << "Setting of new data to the model failed for message read change.";
    return false;
  }

  if (DatabaseQueries::markMessagesReadUnread(m_db, QStringList() << QString::number(message.m_id), read)) {
    return m_selectedItem->getParentServiceRoot()->onAfterSetMessagesRead(m_selectedItem,
                                                                          QList<Message>() << message,
                                                                          read);
  }
  else {
    return false;
  }
}

bool MessagesModel::setMessageReadById(int id, RootItem::ReadStatus read) {
  for (int i = 0; i < rowCount(); i++) {
    int found_id = data(i, MSG_DB_ID_INDEX, Qt::ItemDataRole::EditRole).toInt();

    if (found_id == id) {
      bool set = setData(index(i, MSG_DB_READ_INDEX), int(read));

      if (set) {
        emit dataChanged(index(i, 0), index(i, MSG_DB_LABELS_IDS));
      }

      return set;
    }
  }

  return false;
}

bool MessagesModel::setMessageLabelsById(int id, const QStringList& label_ids) {
  for (int i = 0; i < rowCount(); i++) {
    int found_id = data(i, MSG_DB_ID_INDEX, Qt::ItemDataRole::EditRole).toInt();

    if (found_id == id) {
      QString enc_ids = label_ids.isEmpty() ? QSL(".") : QSL(".") + label_ids.join('.') + QSL(".");
      bool set = setData(index(i, MSG_DB_LABELS_IDS), enc_ids);

      if (set) {
        emit dataChanged(index(i, 0), index(i, MSG_DB_LABELS_IDS));
      }

      return set;
    }
  }

  return false;
}

bool MessagesModel::switchMessageImportance(int row_index) {
  const QModelIndex target_index = index(row_index, MSG_DB_IMPORTANT_INDEX);
  const RootItem::Importance current_importance = (RootItem::Importance)data(target_index, Qt::EditRole).toInt();
  const RootItem::Importance next_importance = current_importance == RootItem::Importance::Important
                                                 ? RootItem::Importance::NotImportant
                                                 : RootItem::Importance::Important;
  const Message message = messageAt(row_index);
  const QPair<Message, RootItem::Importance> pair(message, next_importance);

  if (!m_selectedItem->getParentServiceRoot()
         ->onBeforeSwitchMessageImportance(m_selectedItem, QList<QPair<Message, RootItem::Importance>>() << pair)) {
    return false;
  }

  // Rewrite "visible" data in the model.
  const bool working_change = setData(target_index, int(next_importance));

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebugNN << LOGSEC_MESSAGEMODEL << "Setting of new data to the model failed for message importance change.";
    return false;
  }

  // Commit changes.
  if (DatabaseQueries::markMessageImportant(m_db, message.m_id, next_importance)) {
    emit dataChanged(index(row_index, 0),
                     index(row_index, MSG_DB_FEED_CUSTOM_ID_INDEX),
                     QVector<int>() << Qt::FontRole);

    return m_selectedItem->getParentServiceRoot()
      ->onAfterSwitchMessageImportance(m_selectedItem, QList<QPair<Message, RootItem::Importance>>() << pair);
  }
  else {
    return false;
  }
}

bool MessagesModel::switchBatchMessageImportance(const QModelIndexList& messages) {
  QStringList message_ids;
  message_ids.reserve(messages.size());
  QList<QPair<Message, RootItem::Importance>> message_states;
  message_states.reserve(messages.size());

  // Obtain IDs of all desired messages.
  for (const QModelIndex& message : messages) {
    const Message msg = messageAt(message.row());

    RootItem::Importance message_importance = messageImportance((message.row()));

    message_states.append(QPair<Message, RootItem::Importance>(msg,
                                                               message_importance == RootItem::Importance::Important
                                                                 ? RootItem::Importance::NotImportant
                                                                 : RootItem::Importance::Important));
    message_ids.append(QString::number(msg.m_id));
    QModelIndex idx_msg_imp = index(message.row(), MSG_DB_IMPORTANT_INDEX);

    setData(idx_msg_imp,
            message_importance == RootItem::Importance::Important ? int(RootItem::Importance::NotImportant)
                                                                  : int(RootItem::Importance::Important));
  }

  reloadWholeLayout();

  if (!m_selectedItem->getParentServiceRoot()->onBeforeSwitchMessageImportance(m_selectedItem, message_states)) {
    return false;
  }

  if (DatabaseQueries::switchMessagesImportance(m_db, message_ids)) {
    return m_selectedItem->getParentServiceRoot()->onAfterSwitchMessageImportance(m_selectedItem, message_states);
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesDeleted(const QModelIndexList& messages) {
  QStringList message_ids;
  message_ids.reserve(messages.size());
  QList<Message> msgs;
  msgs.reserve(messages.size());

  // Obtain IDs of all desired messages.
  for (const QModelIndex& message : messages) {
    const Message msg = messageAt(message.row());

    msgs.append(msg);
    message_ids.append(QString::number(msg.m_id));

    if (m_selectedItem->kind() == RootItem::Kind::Bin) {
      setData(index(message.row(), MSG_DB_PDELETED_INDEX), 1);
    }
    else {
      setData(index(message.row(), MSG_DB_DELETED_INDEX), 1);
    }
  }

  reloadWholeLayout();

  if (!m_selectedItem->getParentServiceRoot()->onBeforeMessagesDelete(m_selectedItem, msgs)) {
    return false;
  }

  bool deleted;

  if (m_selectedItem->kind() != RootItem::Kind::Bin) {
    deleted = DatabaseQueries::deleteOrRestoreMessagesToFromBin(m_db, message_ids, true);
  }
  else {
    deleted = DatabaseQueries::permanentlyDeleteMessages(m_db, message_ids);
  }

  if (deleted) {
    return m_selectedItem->getParentServiceRoot()->onAfterMessagesDelete(m_selectedItem, msgs);
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesRead(const QModelIndexList& messages, RootItem::ReadStatus read) {
  QStringList message_ids;
  message_ids.reserve(messages.size());
  QList<Message> msgs;
  msgs.reserve(messages.size());

  // Obtain IDs of all desired messages.
  for (const QModelIndex& message : messages) {
    Message msg = messageAt(message.row());

    msgs.append(msg);
    message_ids.append(QString::number(msg.m_id));
    setData(index(message.row(), MSG_DB_READ_INDEX), int(read));
  }

  reloadWholeLayout();

  if (!m_selectedItem->getParentServiceRoot()->onBeforeSetMessagesRead(m_selectedItem, msgs, read)) {
    return false;
  }

  if (DatabaseQueries::markMessagesReadUnread(m_db, message_ids, read)) {
    return m_selectedItem->getParentServiceRoot()->onAfterSetMessagesRead(m_selectedItem, msgs, read);
  }
  else {
    return false;
  }
}

bool MessagesModel::setBatchMessagesRestored(const QModelIndexList& messages) {
  QStringList message_ids;
  message_ids.reserve(messages.size());
  QList<Message> msgs;
  msgs.reserve(messages.size());

  // Obtain IDs of all desired messages.
  for (const QModelIndex& message : messages) {
    const Message msg = messageAt(message.row());

    msgs.append(msg);
    message_ids.append(QString::number(msg.m_id));
    setData(index(message.row(), MSG_DB_PDELETED_INDEX), 0);
    setData(index(message.row(), MSG_DB_DELETED_INDEX), 0);
  }

  reloadWholeLayout();

  if (!m_selectedItem->getParentServiceRoot()->onBeforeMessagesRestoredFromBin(m_selectedItem, msgs)) {
    return false;
  }

  if (DatabaseQueries::deleteOrRestoreMessagesToFromBin(m_db, message_ids, false)) {
    return m_selectedItem->getParentServiceRoot()->onAfterMessagesRestoredFromBin(m_selectedItem, msgs);
  }
  else {
    return false;
  }
}

QVariant MessagesModel::headerData(int section, Qt::Orientation orientation, int role) const {
  Q_UNUSED(orientation)

  switch (role) {
    case Qt::DisplayRole:
      // Display textual headers for all columns except "read" and
      // "important" and "has enclosures" columns.
      if (section != MSG_DB_READ_INDEX && section != MSG_DB_IMPORTANT_INDEX && section != MSG_DB_SCORE_INDEX &&
          section != MSG_DB_HAS_ENCLOSURES) {
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
        case MSG_DB_HAS_ENCLOSURES:
          return m_enclosuresIcon;

        case MSG_DB_READ_INDEX:
          return m_readIcon;

        case MSG_DB_IMPORTANT_INDEX:
          return m_favoriteIcon;

        case MSG_DB_SCORE_INDEX:
          return m_scoreIcons.at(5);

        default:
          return QVariant();
      }
    }

    default:
      return QVariant();
  }
}
