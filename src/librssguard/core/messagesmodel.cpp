// For license of this file, see <project-root-folder>/LICENSE.md.

#include "core/messagesmodel.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "gui/messagesview.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/skinfactory.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/recyclebin.h"
#include "services/abstract/serviceroot.h"

#include <cmath>

#include <QElapsedTimer>
#include <QPainter>
#include <QPainterPath>
#include <QSqlError>
#include <QSqlField>

#define RAD_COLOR 0, 180, 0

MessagesModel::MessagesModel(QObject* parent)
  : QAbstractTableModel(parent), m_canFetchMoreArticles(false), m_view(nullptr),
    m_messageHighlighter(MessageHighlighter::NoHighlighting), m_customDateFormat(QString()),
    m_customTimeFormat(QString()), m_customFormatForDatesOnly(QString()), m_newerArticlesRelativeTime(-1),
    m_selectedItem(nullptr), m_unreadIconType(MessageUnreadIcon::Dot),
    m_multilineListItems(qApp->settings()->value(GROUP(Messages), SETTING(Messages::MultilineArticleList)).toBool()),
    m_additionalArticleId(0), m_lazyLoading(false) {
  updateFeedIconsDisplay();
  updateDateFormat();
  reloadLazyLoading();
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

int MessagesModel::additionalArticleId() const {
  return m_additionalArticleId;
}

void MessagesModel::setAdditionalArticleId(int additional_article_id) {
  m_additionalArticleId = additional_article_id;

  qDebugNN << LOGSEC_MESSAGEMODEL
           << "Setting additional ID of article to be excluded from filtering or reselected after model reset to"
           << QUOTE_W_SPACE_DOT(additional_article_id);
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

  connect(m_view, &MessagesView::reachedEndOfList, this, [this]() {
    qDebugNN << LOGSEC_MESSAGEMODEL << "Reached end of article list";

    if (m_canFetchMoreArticles) {
      fetchMoreArticles();
    }
  });
}

void MessagesModel::fetchMoreArticles(int batch_size) {
  qDebugNN << LOGSEC_MESSAGEMODEL << "We need to fetch more articles!";

  try {
    QElapsedTimer tmr;
    tmr.start();

    auto more_messages = fetchMessages(m_hashedLabels, batch_size, m_messages.size());

    m_canFetchMoreArticles = more_messages.size() >= batch_size;

    // NOTE: Some message data are NOT fetched from database. Fill them directly into the data here.
    for (Message& msg : more_messages) {
      fillComputedMessageData(&msg);
    }

    if (more_messages.isEmpty()) {
      qWarningNN << LOGSEC_MESSAGEMODEL << "There are no more article to fetch, everything is already loaded!";
    }
    else {
      auto sz = m_messages.size();
      auto sz_new = sz + more_messages.size() - 1;

      beginInsertRows(QModelIndex(), sz, sz_new);
      m_messages.reserve(sz_new + 1);
      m_messages.append(more_messages);
      endInsertRows();

      qApp->showGuiMessage(Notification::Event::NoEvent,
                           GuiMessage(QString(),
                                      tr("Loaded extra %1 articles in %2 miliseconds")
                                        .arg(QString::number(more_messages.size()), QString::number(tmr.elapsed()))),
                           GuiMessageDestination(false, false, true));
    }
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_MESSAGEMODEL << "Error when querying for more articles:" << QUOTE_W_SPACE_DOT(ex.message());
    m_canFetchMoreArticles = false;
  }
}

void MessagesModel::fetchAllArticles() {
  fetchMoreArticles(0);
}

void MessagesModel::fetchInitialArticles(int batch_size) {
  qDebugNN << LOGSEC_MESSAGEMODEL << "Repopulate started.";

  qint64 time_clear, time_fetch, time_fill;

  QElapsedTimer tmr;
  tmr.start();

  beginResetModel();
  m_canFetchMoreArticles = false;
  m_messages.clear();
  time_clear = tmr.elapsed();
  tmr.restart();

  try {
    m_messages = fetchMessages(m_hashedLabels, m_lazyLoading ? batch_size : 0, 0, m_additionalArticleId);
    m_canFetchMoreArticles = m_messages.size() >= batch_size;

    time_fetch = tmr.elapsed();
    tmr.restart();

    // NOTE: Some message data are NOT fetched from database. Fill them directly into the data here.
    for (Message& msg : m_messages) {
      fillComputedMessageData(&msg);
    }

    time_fill = tmr.elapsed();
    tmr.restart();
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_MESSAGEMODEL << "Error when setting new msg view query:" << QUOTE_W_SPACE_DOT(ex.message());
    m_canFetchMoreArticles = false;
    m_messages.clear();
  }

  endResetModel();

  if (!m_messages.isEmpty()) {
    qApp->showGuiMessage(Notification::Event::NoEvent,
                         GuiMessage(QString(),
                                    tr("Loaded %1 articles in %2 ms (%3 ms to clear cache, %4 ms for DB data transfer, "
                                       "%5 "
                                       "ms to fill dynamic data)")
                                      .arg(QString::number(m_messages.size()),
                                           QString::number(time_clear + time_fill + time_fetch),
                                           QString::number(time_clear),
                                           QString::number(time_fetch),
                                           QString::number(time_fill))),
                         GuiMessageDestination(false, false, true));
  }

  qDebugNN << LOGSEC_MESSAGEMODEL << "Repopulated model!";
}

int MessagesModel::rowCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : m_messages.size();
}

int MessagesModel::columnCount(const QModelIndex& parent) const {
  return parent.isValid() ? 0 : m_headerData.size();
}

bool MessagesModel::setData(const QModelIndex& idx, const QVariant& value, int role) {
  Q_UNUSED(role)

  if (!idx.isValid()) {
    return false;
  }

  auto& msg = messageForRow(idx.row());

  switch (idx.column()) {
    case MSG_MDL_READ_INDEX:
      msg.m_isRead = value.toBool();
      break;

    case MSG_MDL_IMPORTANT_INDEX:
      msg.m_isImportant = value.toBool();
      break;

    case MSG_MDL_DELETED_INDEX:
      msg.m_isDeleted = value.toBool();
      break;

    case MSG_MDL_PDELETED_INDEX:
      msg.m_isPdeleted = value.toBool();
      break;

    case MSG_MDL_LABELS:
      msg.m_assignedLabels = value.value<QList<Label*>>();
      break;

    default:
      throw ApplicationException(tr("cannot set model data for column %1").arg(idx.column()));
      break;
  }

  emit dataChanged(index(idx.row(), 0), index(idx.row(), columnCount() - 1));
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

void MessagesModel::loadMessages(RootItem* item, bool keep_additional_article_id) {
  m_selectedItem = item;

  if (!keep_additional_article_id) {
    m_additionalArticleId = 0;
  }

  if (item != nullptr) {
    auto* acc = item->account();
    m_hashedFeeds = acc->getPrimaryIdHashedSubTreeFeeds();
    m_hashedLabels = acc->labelsNode()->getHashedLabels();
  }
  else {
    m_hashedFeeds.clear();
    m_hashedLabels.clear();
  }

  if (item == nullptr) {
    setFilter(QSL(DEFAULT_SQL_MESSAGES_FILTER));
  }
  else {
    if (!item->account()->loadMessagesForItem(item, this)) {
      setFilter(QSL(DEFAULT_SQL_MESSAGES_FILTER));
      qCriticalNN << LOGSEC_MESSAGEMODEL << "Loading of messages from item '" << item->title() << "' failed.";
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Loading of articles from item '%1' failed").arg(item->title()),
                            tr("Loading of articles failed, maybe messages could not be downloaded."),
                            QSystemTrayIcon::MessageIcon::Critical});
    }
  }

  fetchInitialArticles();
}

bool MessagesModel::setMessageImportantById(int id, RootItem::Importance important) {
  for (int i = 0; i < rowCount(); i++) {
    int found_id = data(i, MSG_MDL_ID_INDEX).toInt();

    if (found_id == id) {
      return setData(index(i, MSG_MDL_IMPORTANT_INDEX), int(important));
    }
  }

  return false;
}

void MessagesModel::highlightMessages(MessagesModel::MessageHighlighter highlighter) {
  m_messageHighlighter = highlighter;

  reloadWholeLayout();
}

int MessagesModel::messageId(int row_index) const {
  return data(row_index, MSG_MDL_ID_INDEX).toInt();
}

RootItem::Importance MessagesModel::messageImportance(int row_index) const {
  return RootItem::Importance(data(row_index, MSG_MDL_IMPORTANT_INDEX).toInt());
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

void MessagesModel::reloadLazyLoading() {
  m_lazyLoading = qApp->settings()->value(GROUP(Messages), SETTING(Messages::ArticleListLazyLoading)).toBool();
}

void MessagesModel::reloadWholeLayout() {
  emit layoutAboutToBeChanged();
  emit layoutChanged();
}

void MessagesModel::reloadChangedLayout(const QModelIndexList& indices) {
  if (indices.isEmpty()) {
    return;
  }

  auto idxs = indices;

  std::sort(idxs.begin(), idxs.end(), [](const QModelIndex& a, const QModelIndex& b) {
    if (a.row() == b.row()) {
      return a.column() < b.column();
    }
    return a.row() < b.row();
  });

  emit dataChanged(idxs.constFirst(), idxs.constLast());
  // emit dataChanged(index(idx_low.row(), 0), index(idx_high.row(), columnCount() - 1));
}

Message& MessagesModel::messageForRow(int row) {
  if (row >= 0 && row < m_messages.size()) {
    return m_messages[row];
  }
  else {
    throw ApplicationException(tr("article with row %1 not found").arg(row));
  }
}

int MessagesModel::rowForMessage(int message_id) const {
  int i = 0;
  for (const auto& msg : m_messages) {
    if (msg.m_id == message_id) {
      return i;
    }

    i++;
  }

  return -1;
}

QModelIndex MessagesModel::indexForMessage(int message_id) const {
  auto rw = rowForMessage(message_id);

  if (rw < 0) {
    return QModelIndex();
  }
  else {
    return index(rw, MSG_MDL_TITLE_INDEX);
  }
}

const Message& MessagesModel::messageForRow(int row) const {
  if (row >= 0 && row < m_messages.size()) {
    return m_messages[row];
  }
  else {
    throw ApplicationException(tr("article with row %1 not found").arg(row));
  }
}

void MessagesModel::setupHeaderData() {
  m_headerData << tr("Id") << tr("Read") << tr("Important") << tr("Deleted") << tr("Permanently deleted")
               << tr("Feed ID") << tr("Title") << tr("URL") << tr("Author") << tr("Date") << tr("Contents")
               << tr("Score") << tr("Account ID") << tr("Custom ID") << tr("Custom hash") << tr("Feed")
               << tr("Has attachments") << tr("Assigned labels");

  m_tooltipData << tr("ID of the article.") << tr("Is article read?") << tr("Is article important?")
                << tr("Is article deleted?") << tr("Is article permanently deleted from recycle bin?")
                << tr("ID of feed which this article belongs to.") << tr("Title of the article.")
                << tr("Url of the article.") << tr("Author of the article.") << tr("Creation date of the article.")
                << tr("Contents of the article.") << tr("Score of the article.") << tr("Account ID of the article.")
                << tr("Custom ID of the article.") << tr("Custom hash of the article.")
                << tr("Name of feed of the article.") << tr("Indication of attachments presence within the article.")
                << tr("Labels assigned to the article.");
}

bool MessagesModel::lazyLoading() const {
  return m_lazyLoading;
}

Qt::ItemFlags MessagesModel::flags(const QModelIndex& index) const {
  if (index.isValid()) {
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemNeverHasChildren;
  }
  else {
    return Qt::NoItemFlags;
  }
}

QList<Message> MessagesModel::messagesAt(const QList<int>& row_indices) const {
  QList<Message> msgs;
  msgs.reserve(row_indices.size());

  for (int idx : row_indices) {
    msgs << messageForRow(idx);
  }

  return msgs;
}

QVariant MessagesModel::data(int row, int column, int role) const {
  return data(index(row, column), role);
}

QVariant MessagesModel::data(const QModelIndex& idx, int role) const {
  if (!idx.isValid()) {
    return QVariant();
  }

  switch (role) {
    case Qt::ItemDataRole::EditRole: {
      const Message& msg = messageForRow(idx.row());

      switch (idx.column()) {
        case MSG_MDL_ID_INDEX:
          return msg.m_id;

        case MSG_MDL_READ_INDEX:
          return msg.m_isRead;

        case MSG_MDL_IMPORTANT_INDEX:
          return msg.m_isImportant;

        case MSG_MDL_DELETED_INDEX:
          return msg.m_isDeleted;

        case MSG_MDL_PDELETED_INDEX:
          return msg.m_isPdeleted;

        case MSG_MDL_FEED_ID_INDEX:
          return msg.m_feedId;

        case MSG_MDL_TITLE_INDEX:
          return msg.m_title;

        case MSG_MDL_URL_INDEX:
          return msg.m_url;

        case MSG_MDL_AUTHOR_INDEX:
          return msg.m_author;

        case MSG_MDL_DCREATED_INDEX:
          return msg.m_created;

        case MSG_MDL_CONTENTS_INDEX:
          return msg.m_contents;

        case MSG_MDL_SCORE_INDEX:
          return msg.m_score;

        case MSG_MDL_ACCOUNT_ID_INDEX:
          return msg.m_accountId;

        case MSG_MDL_CUSTOM_ID_INDEX:
          return msg.m_customId;

        case MSG_MDL_CUSTOM_HASH_INDEX:
          return msg.m_customHash;

        case MSG_MDL_FEED_TITLE_INDEX:
          return msg.m_feedTitle;

        case MSG_MDL_HAS_ENCLOSURES:
          return !msg.m_enclosures.isEmpty();

        case MSG_MDL_LABELS:
          return QVariant::fromValue(msg.m_assignedLabels);

        default:
          throw ApplicationException(tr("article model column %1 is out of range").arg(idx.column()));
      }
    }

    case Qt::ItemDataRole::DisplayRole: {
      int index_column = idx.column();

      if (index_column == MSG_MDL_DCREATED_INDEX) {
        QDateTime utc_dt = data(idx, Qt::ItemDataRole::EditRole).toDateTime();
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
      else if (index_column == MSG_MDL_CONTENTS_INDEX) {
        // Do not display full contents here.
        QString contents = data(idx, Qt::ItemDataRole::EditRole).toString().mid(0, 64).simplified() + QL1S("...");
        return contents;
      }
      else if (index_column == MSG_MDL_LABELS) {
        const Message& msg = messageForRow(idx.row());

        return formatLabels(msg.m_assignedLabels);
      }
      else if (index_column == MSG_MDL_AUTHOR_INDEX) {
        const QString author_name = data(idx, Qt::ItemDataRole::EditRole).toString();
        return author_name.isEmpty() ? QSL("-") : author_name;
      }
      else if (index_column != MSG_MDL_IMPORTANT_INDEX && index_column != MSG_MDL_READ_INDEX &&
               index_column != MSG_MDL_HAS_ENCLOSURES && index_column != MSG_MDL_SCORE_INDEX) {
        return data(idx, Qt::ItemDataRole::EditRole);
      }
      else {
        return QVariant();
      }
    }

    case TEXT_DIRECTION_ROLE: {
      int index_column = idx.column();

      if (index_column != MSG_MDL_TITLE_INDEX && index_column != MSG_MDL_FEED_TITLE_INDEX &&
          index_column != MSG_MDL_AUTHOR_INDEX) {
        return Qt::LayoutDirection::LayoutDirectionAuto;
      }
      else {
        const Message& msg = messageForRow(idx.row());
        RtlBehavior rtl_mode = msg.m_rtlBehavior;

        return (rtl_mode == RtlBehavior::Everywhere || rtl_mode == RtlBehavior::EverywhereExceptFeedList)
                 ? Qt::LayoutDirection::RightToLeft
                 : Qt::LayoutDirection::LayoutDirectionAuto;
      }
    }

    case Qt::ItemDataRole::ToolTipRole: {
      if (!qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::EnableTooltipsFeedsMessages)).toBool()) {
        return QVariant();
      }
      else {
        if (idx.column() == MSG_MDL_SCORE_INDEX) {
          return data(idx, Qt::ItemDataRole::EditRole);
        }
        else if (idx.column() == MSG_MDL_URL_INDEX) {
          return TextFactory::shorten(data(idx, Qt::ItemDataRole::EditRole).toString(), TEXT_TOOLTIP_LIMIT);
        }
        else if (idx.column() == MSG_MDL_DCREATED_INDEX) {
          return qApp->localization()
            ->loadedLocale()
            .toString(data(idx, Qt::ItemDataRole::EditRole).toDateTime().toLocalTime(),
                      QLocale::FormatType::LongFormat);
        }
        else if (idx.column() == MSG_MDL_READ_INDEX && m_unreadIconType == MessageUnreadIcon::FeedIcon) {
          return data(idx.row(), MSG_MDL_FEED_TITLE_INDEX, Qt::ItemDataRole::EditRole);
        }
        else {
          return data(idx, Qt::ItemDataRole::DisplayRole);
        }
      }
    }

    case Qt::ItemDataRole::FontRole: {
      QModelIndex idx_read = index(idx.row(), MSG_MDL_READ_INDEX);
      QVariant data_read = data(idx_read, Qt::ItemDataRole::EditRole);
      const bool is_bin = qobject_cast<RecycleBin*>(loadedItem()) != nullptr;
      bool striked;

      if (is_bin) {
        QModelIndex idx_del = index(idx.row(), MSG_MDL_DELETED_INDEX);
        QModelIndex idx_pdel = index(idx.row(), MSG_MDL_PDELETED_INDEX);

        // NOTE: If we are in the bin and the article is permanently deleted (deleted from recycle bin)
        // or it is NOT deleted (it was restored from bin), then draw as striked.
        striked =
          data(idx_pdel, Qt::ItemDataRole::EditRole).toBool() || !data(idx_del, Qt::ItemDataRole::EditRole).toBool();
      }
      else {
        QModelIndex idx_del = index(idx.row(), MSG_MDL_DELETED_INDEX);

        striked = data(idx_del, Qt::ItemDataRole::EditRole).toBool();
      }

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
        QModelIndex idx_important = index(idx.row(), MSG_MDL_IMPORTANT_INDEX);
        QVariant dta = data(idx_important, Qt::ItemDataRole::EditRole);

        if (dta.toBool()) {
          return qApp->skins()->colorForModel(role == Qt::ItemDataRole::ForegroundRole
                                                ? SkinEnums::PaletteColors::FgInteresting
                                                : SkinEnums::PaletteColors::FgSelectedInteresting);
        }
      }

      if (Globals::hasFlag(m_messageHighlighter, MessageHighlighter::HighlightUnread)) {
        QModelIndex idx_read = index(idx.row(), MSG_MDL_READ_INDEX);
        QVariant dta = data(idx_read, Qt::ItemDataRole::EditRole);

        if (dta.toBool()) {
          return qApp->skins()->colorForModel(role == Qt::ItemDataRole::ForegroundRole
                                                ? SkinEnums::PaletteColors::FgInteresting
                                                : SkinEnums::PaletteColors::FgSelectedInteresting);
        }
      }

      return QVariant();
    }

    case Qt::ItemDataRole::SizeHintRole: {
      if (!m_multilineListItems || m_view == nullptr || m_view->isColumnHidden(idx.column()) ||
          idx.column() != MSG_MDL_TITLE_INDEX) {
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

      if (index_column == MSG_MDL_READ_INDEX) {
        if (m_unreadIconType == MessageUnreadIcon::FeedIcon && m_selectedItem != nullptr) {
          QModelIndex idx_feedid = index(idx.row(), MSG_MDL_FEED_ID_INDEX);
          QVariant dta = data(idx_feedid, Qt::ItemDataRole::EditRole);
          int feed_id = dta.toInt();

          auto* fd = m_hashedFeeds.value(feed_id);

          if (fd == nullptr) {
            return qApp->icons()->fromTheme(QSL("application-rss+xml"));
          }
          else {
            return fd->icon();
          }
        }
        else {
          QModelIndex idx_read = index(idx.row(), MSG_MDL_READ_INDEX);
          QVariant dta = data(idx_read, Qt::ItemDataRole::EditRole);

          if (m_unreadIconType == MessageUnreadIcon::Dot) {
            return dta.toBool() ? QVariant() : m_unreadIcon;
          }
          else {
            return dta.toBool() ? m_readIcon : m_unreadIcon;
          }
        }
      }
      else if (index_column == MSG_MDL_IMPORTANT_INDEX) {
        QModelIndex idx_important = index(idx.row(), MSG_MDL_IMPORTANT_INDEX);
        QVariant dta = data(idx_important, Qt::ItemDataRole::EditRole);

        return dta.toBool() ? m_favoriteIcon : QVariant();
      }
      else if (index_column == MSG_MDL_HAS_ENCLOSURES) {
        QModelIndex idx_enc = index(idx.row(), MSG_MDL_HAS_ENCLOSURES);
        QVariant dta = data(idx_enc, Qt::ItemDataRole::EditRole);

        return dta.toBool() ? m_enclosuresIcon : QVariant();
      }
      else if (index_column == MSG_MDL_SCORE_INDEX) {
        QVariant dta = data(idx, Qt::ItemDataRole::EditRole);
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

void MessagesModel::switchMessageReadUnread(int row_index) {
  RootItem::ReadStatus current_read = RootItem::ReadStatus(data(row_index, MSG_MDL_READ_INDEX).toInt());

  setMessageRead(row_index,
                 current_read == RootItem::ReadStatus::Read ? RootItem::ReadStatus::Unread
                                                            : RootItem::ReadStatus::Read);
}

void MessagesModel::setMessageRead(int row_index, RootItem::ReadStatus read) {
  if (data(row_index, MSG_MDL_READ_INDEX).toInt() == int(read)) {
    // Read status is the same is the one currently set.
    // In that case, no extra work is needed.
    return;
  }

  const Message& message = messageForRow(row_index);

  m_selectedItem->account()->onBeforeSetMessagesRead(m_selectedItem, {message.m_customId}, read);

  // Rewrite "visible" data in the model.
  bool working_change = setData(index(row_index, MSG_MDL_READ_INDEX), int(read));

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebugNN << "Setting of new data to the model failed for message read change.";
    return;
  }

  DatabaseQueries::markMessagesReadUnread(m_db, {QString::number(message.m_id)}, read);
  m_selectedItem->account()->onAfterSetMessagesRead(m_selectedItem, {message}, read);
}

bool MessagesModel::setMessageReadById(int id, RootItem::ReadStatus read) {
  for (int i = 0; i < rowCount(); i++) {
    int found_id = data(i, MSG_MDL_ID_INDEX).toInt();

    if (found_id == id) {
      bool set = setData(index(i, MSG_MDL_READ_INDEX), int(read));
      return set;
    }
  }

  return false;
}

bool MessagesModel::setMessageLabelsById(int id, const QList<Label*>& labels) {
  for (int i = 0; i < rowCount(); i++) {
    int found_id = data(i, MSG_MDL_ID_INDEX).toInt();

    if (found_id == id) {
      bool set = setData(index(i, MSG_MDL_LABELS), QVariant::fromValue(labels));
      return set;
    }
  }

  return false;
}

QString MessagesModel::formatLabels(const QList<Label*>& labels) const {
  auto label_titles_std = boolinq::from(labels)
                            .select([](const Label* label) {
                              return label != nullptr ? label->title() : QString();
                            })
                            .where([](const QString& title) {
                              return !title.isEmpty();
                            })
                            .toStdList();
  QStringList label_titles = FROM_STD_LIST(QStringList, label_titles_std);

  return label_titles.join(QL1C(','));
}

void MessagesModel::fillComputedMessageData(Message* msg) {
  auto* fd = m_hashedFeeds.value(msg->m_feedId);

  msg->m_rtlBehavior = fd != nullptr ? fd->rtlBehavior() : msg->m_rtlBehavior;
  msg->m_feedTitle = fd != nullptr ? fd->title() : msg->m_feedTitle;
  msg->m_feedCustomId = fd != nullptr ? fd->customId() : QString();
}

void MessagesModel::switchMessageImportance(int row_index) {
  const QModelIndex target_index = index(row_index, MSG_MDL_IMPORTANT_INDEX);
  const RootItem::Importance current_importance = (RootItem::Importance)data(target_index).toInt();
  const RootItem::Importance next_importance = current_importance == RootItem::Importance::Important
                                                 ? RootItem::Importance::NotImportant
                                                 : RootItem::Importance::Important;
  const Message& message = messageForRow(row_index);
  const QPair<Message, RootItem::Importance> pair(message, next_importance);

  m_selectedItem->account()->onBeforeSwitchMessageImportance(m_selectedItem, {pair});

  // Rewrite "visible" data in the model.
  const bool working_change = setData(target_index, int(next_importance));

  if (!working_change) {
    // If rewriting in the model failed, then cancel all actions.
    qDebugNN << LOGSEC_MESSAGEMODEL << "Setting of new data to the model failed for message importance change.";
    return;
  }

  // Commit changes.
  DatabaseQueries::markMessageImportant(m_db, message.m_id, next_importance);
  m_selectedItem->account()->onAfterSwitchMessageImportance(m_selectedItem,
                                                            QList<QPair<Message, RootItem::Importance>>() << pair);
}

void MessagesModel::switchBatchMessageImportance(const QModelIndexList& messages) {
  QStringList message_ids;
  message_ids.reserve(messages.size());

  QList<QPair<Message, RootItem::Importance>> message_states;
  message_states.reserve(messages.size());

  QModelIndexList changed_indices;
  changed_indices.reserve(messages.size());

  // Obtain IDs of all desired messages.
  blockSignals(true);

  for (const QModelIndex& message : messages) {
    const Message& msg = messageForRow(message.row());
    const RootItem::Importance message_importance = RootItem::Importance(msg.m_isImportant);

    message_states.append(QPair<Message, RootItem::Importance>(msg,
                                                               message_importance == RootItem::Importance::Important
                                                                 ? RootItem::Importance::NotImportant
                                                                 : RootItem::Importance::Important));
    message_ids.append(QString::number(msg.m_id));
    QModelIndex idx_msg_imp = index(message.row(), MSG_MDL_IMPORTANT_INDEX);

    setData(idx_msg_imp,
            message_importance == RootItem::Importance::Important ? int(RootItem::Importance::NotImportant)
                                                                  : int(RootItem::Importance::Important));

    changed_indices.append(idx_msg_imp);
  }

  blockSignals(false);
  reloadChangedLayout(changed_indices);

  m_selectedItem->account()->onBeforeSwitchMessageImportance(m_selectedItem, message_states);
  DatabaseQueries::switchMessagesImportance(m_db, message_ids);
  m_selectedItem->account()->onAfterSwitchMessageImportance(m_selectedItem, message_states);
}

void MessagesModel::setBatchMessagesDeleted(const QModelIndexList& messages) {
  QStringList message_ids;
  message_ids.reserve(messages.size());

  QList<Message> msgs;
  msgs.reserve(messages.size());

  QModelIndexList changed_indices;
  changed_indices.reserve(messages.size());

  blockSignals(true);

  // Obtain IDs of all desired messages.
  for (const QModelIndex& message : messages) {
    const Message& msg = messageForRow(message.row());

    msgs.append(msg);
    message_ids.append(QString::number(msg.m_id));

    QModelIndex idx_del;

    if (m_selectedItem->kind() == RootItem::Kind::Bin) {
      idx_del = index(message.row(), MSG_MDL_PDELETED_INDEX);
    }
    else {
      idx_del = index(message.row(), MSG_MDL_DELETED_INDEX);
    }

    setData(idx_del, 1);
    changed_indices.append(idx_del);
  }

  blockSignals(false);
  reloadChangedLayout(changed_indices);

  m_selectedItem->account()->onBeforeMessagesDelete(m_selectedItem, msgs);

  if (m_selectedItem->kind() != RootItem::Kind::Bin) {
    DatabaseQueries::deleteOrRestoreMessagesToFromBin(m_db, message_ids, true);
  }
  else {
    DatabaseQueries::permanentlyDeleteMessages(m_db, message_ids);
  }

  m_selectedItem->account()->onAfterMessagesDelete(m_selectedItem, msgs);
}

void MessagesModel::setBatchMessagesRead(const QModelIndexList& messages, RootItem::ReadStatus read) {
  QStringList message_ids;
  message_ids.reserve(messages.size());

  QList<Message> msgs;
  msgs.reserve(messages.size());

  QModelIndexList changed_indices;
  changed_indices.reserve(messages.size());

  blockSignals(true);

  // Obtain IDs of all desired messages.
  for (const QModelIndex& message : messages) {
    const Message& msg = messageForRow(message.row());

    msgs.append(msg);
    message_ids.append(QString::number(msg.m_id));

    QModelIndex idx = index(message.row(), MSG_MDL_READ_INDEX);
    setData(idx, int(read));

    changed_indices.append(idx);
  }

  blockSignals(false);
  reloadChangedLayout(changed_indices);

  m_selectedItem->account()->onBeforeSetMessagesRead(m_selectedItem, msgs, read);
  DatabaseQueries::markMessagesReadUnread(m_db, message_ids, read);
  m_selectedItem->account()->onAfterSetMessagesRead(m_selectedItem, msgs, read);
}

void MessagesModel::setBatchMessagesRestored(const QModelIndexList& messages) {
  QStringList message_ids;
  message_ids.reserve(messages.size());

  QList<Message> msgs;
  msgs.reserve(messages.size());

  QModelIndexList changed_indices;
  changed_indices.reserve(messages.size());

  blockSignals(true);

  // Obtain IDs of all desired messages.
  for (const QModelIndex& message : messages) {
    const Message& msg = messageForRow(message.row());

    msgs.append(msg);
    message_ids.append(QString::number(msg.m_id));

    QModelIndex idx = index(message.row(), MSG_MDL_DELETED_INDEX);
    setData(idx, 0);
    changed_indices.append(idx);

    idx = index(message.row(), MSG_MDL_PDELETED_INDEX);
    setData(idx, 0);
    changed_indices.append(idx);
  }

  blockSignals(false);
  reloadChangedLayout(changed_indices);

  m_selectedItem->account()->onBeforeMessagesRestoredFromBin(m_selectedItem, msgs);
  DatabaseQueries::deleteOrRestoreMessagesToFromBin(m_db, message_ids, false);
  m_selectedItem->account()->onAfterMessagesRestoredFromBin(m_selectedItem, msgs);
}

void MessagesModel::markArticleDataReadUnread(bool read) {
  for (Message& msg : m_messages) {
    msg.m_isRead = read;
  }

  reloadChangedLayout({index(0, 0), index(rowCount() - 1, columnCount() - 1)});
}

QVariant MessagesModel::headerData(int section, Qt::Orientation orientation, int role) const {
  Q_UNUSED(orientation)

  switch (role) {
    case Qt::ItemDataRole::DisplayRole:
      // Display textual headers for all columns except "read" and
      // "important" and "has enclosures" columns.
      if (section != MSG_MDL_READ_INDEX && section != MSG_MDL_IMPORTANT_INDEX && section != MSG_MDL_SCORE_INDEX &&
          section != MSG_MDL_HAS_ENCLOSURES) {
        return m_headerData.at(section);
      }
      else {
        return QVariant();
      }

    case Qt::ItemDataRole::ToolTipRole:
      return m_tooltipData.at(section);

    case Qt::ItemDataRole::EditRole:
      return m_headerData.at(section);

    // Display icons for "read" and "important" columns.
    case Qt::ItemDataRole::DecorationRole: {
      switch (section) {
        case MSG_MDL_HAS_ENCLOSURES:
          return m_enclosuresIcon;

        case MSG_MDL_READ_INDEX:
          return m_readIcon;

        case MSG_MDL_IMPORTANT_INDEX:
          return m_favoriteIcon;

        case MSG_MDL_SCORE_INDEX:
          return m_scoreIcons.at(5);

        default:
          return QVariant();
      }
    }

    default:
      return QVariant();
  }
}
