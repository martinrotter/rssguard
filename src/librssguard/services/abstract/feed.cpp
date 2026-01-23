// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/feed.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/gui/formfeeddetails.h"
#include "services/abstract/serviceroot.h"

Feed::Feed(RootItem* parent)
  : RootItem(parent), m_source(QString()), m_status(Status::Normal), m_statusString(QString()),
    m_autoUpdateType(AutoUpdateType::DefaultAutoUpdate), m_autoUpdateInterval(DEFAULT_AUTO_UPDATE_INTERVAL),
    m_lastUpdated(QDateTime::currentDateTimeUtc()), m_isSwitchedOff(false), m_isQuiet(false),
    m_rtlBehavior(RtlBehavior::NoRtl), m_messageFilters(QList<QPointer<MessageFilter>>()) {
  setKind(RootItem::Kind::Feed);
}

Feed::Feed(const QString& title, const QString& custom_id, const QIcon& icon, RootItem* parent) : Feed(parent) {
  setTitle(title);
  setCustomId(custom_id);
  setIcon(icon);
}

Feed::Feed(const Feed& other) : RootItem(other) {
  setKind(RootItem::Kind::Feed);

  setCountOfAllMessages(other.countOfAllMessages());
  setCountOfUnreadMessages(other.countOfUnreadMessages());
  setSource(other.source());
  setStatus(other.status(), other.statusString());
  setAutoUpdateType(other.autoUpdateType());
  setAutoUpdateInterval(other.autoUpdateInterval());
  setLastUpdated(other.lastUpdated());
  setMessageFilters(other.messageFilters());
  setArticleIgnoreLimit(Feed::ArticleIgnoreLimit(other.articleIgnoreLimit()));
  setRtlBehavior(other.rtlBehavior());
  setIsSwitchedOff(other.isSwitchedOff());
  setIsQuiet(other.isQuiet());
}

QVariant Feed::data(int column, int role) const {
  switch (role) {
    case HIGHLIGHTED_FOREGROUND_TITLE_ROLE:
      switch (status()) {
        case Status::NewMessages:
          return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgSelectedNewMessages);

        case Status::Normal:
          if (countOfUnreadMessages() > 0) {
            return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgSelectedInteresting);
          }
          else {
            return QVariant();
          }

        case Status::NetworkError:
        case Status::ParsingError:
        case Status::AuthError:
        case Status::SqlError:
        case Status::OtherError:
          return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgSelectedError);

        default:
          return QVariant();
      }

    case TEXT_DIRECTION_ROLE: {
      if (column == FDS_MODEL_TITLE_INDEX) {
        return rtlBehavior() == RtlBehavior::Everywhere ? Qt::LayoutDirection::RightToLeft
                                                        : Qt::LayoutDirection::LayoutDirectionAuto;
      }
      else {
        return Qt::LayoutDirection::LayoutDirectionAuto;
      }
    }

    case Qt::ItemDataRole::ForegroundRole: {
      if (isSwitchedOff()) {
        return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgDisabledFeed);
      }

      switch (status()) {
        case Status::NewMessages:
          return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgNewMessages);

        case Status::Normal:
          if (countOfUnreadMessages() > 0) {
            return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgInteresting);
          }
          else {
            return QVariant();
          }

        case Status::NetworkError:
        case Status::ParsingError:
        case Status::AuthError:
        case Status::SqlError:
        case Status::OtherError:
          return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgError);

        default:
          return QVariant();
      }
    }

    default:
      return RootItem::data(column, role);
  }
}

int Feed::autoUpdateInterval() const {
  return m_autoUpdateInterval;
}

int Feed::countOfAllMessages() const {
  return m_totalCount;
}

int Feed::countOfUnreadMessages() const {
  return m_unreadCount;
}

QVariantHash Feed::customDatabaseData() const {
  return {};
}

void Feed::setCustomDatabaseData(const QVariantHash& data) {
  Q_UNUSED(data)
}

void Feed::setCountOfAllMessages(int count_all_messages) {
  m_totalCount = count_all_messages;
}

void Feed::setCountOfUnreadMessages(int count_unread_messages) {
  if (status() == Status::NewMessages && count_unread_messages < m_unreadCount) {
    setStatus(Status::Normal);
  }

  m_unreadCount = count_unread_messages;
}

bool Feed::canBeEdited() const {
  return true;
}

bool Feed::isFetching() const {
  return m_status == Status::Fetching;
}

void Feed::setAutoUpdateInterval(int auto_update_interval) {
  // If new initial auto-update interval is set, then
  // we should reset time that remains to the next auto-update.
  m_autoUpdateInterval = auto_update_interval;
  m_lastUpdated = QDateTime::currentDateTimeUtc();
}

Feed::AutoUpdateType Feed::autoUpdateType() const {
  return m_autoUpdateType;
}

void Feed::setAutoUpdateType(Feed::AutoUpdateType auto_update_type) {
  m_autoUpdateType = auto_update_type;
}

Feed::Status Feed::status() const {
  return m_status;
}

bool Feed::isErrorStatus(Feed::Status status) {
  if (status == Feed::Status::Fetching || status == Feed::Status::NewMessages || status == Feed::Status::Normal) {
    return false;
  }
  else {
    return true;
  }
}

void Feed::setStatus(Feed::Status status, const QString& status_text) {
  m_status = status;
  m_statusString = status_text;
}

QString Feed::source() const {
  return m_source;
}

void Feed::setSource(const QString& source) {
  m_source = source;
}

RtlBehavior Feed::rtlBehavior() const {
  return m_rtlBehavior;
}

void Feed::setRtlBehavior(RtlBehavior rtl) {
  m_rtlBehavior = rtl;
}

bool Feed::removeUnwantedArticles(QSqlDatabase& db) {
  Feed::ArticleIgnoreLimit feed_setup = articleIgnoreLimit();
  Feed::ArticleIgnoreLimit app_setup = Feed::ArticleIgnoreLimit::fromSettings();

  bool removed = DatabaseQueries::removeUnwantedArticlesFromFeed(db, this, feed_setup, app_setup);
  return removed;
}

void Feed::appendMessageFilter(MessageFilter* filter) {
  removeMessageFilter(filter);
  m_messageFilters.append(QPointer<MessageFilter>(filter));
}

void Feed::updateCounts() {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  auto fc = DatabaseQueries::getMessageCountsForFeed(database, id());

  setCountOfAllMessages(fc.m_total);
  setCountOfUnreadMessages(fc.m_unread);
}

void Feed::cleanMessages(bool clean_read_only) {
  account()->cleanFeeds({this}, clean_read_only);
}

void Feed::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = account();
  auto article_custom_ids = service->customIDsOfMessagesForItem(this, status);

  service->onBeforeSetMessagesRead(this, article_custom_ids, status);
  DatabaseQueries::markFeedsReadUnread(qApp->database()->driver()->connection(metaObject()->className()),
                                       service->textualFeedIds({this}),
                                       status);
  service->onAfterSetMessagesRead(this, {}, status);
  service->informOthersAboutDataChange(this,
                                       status == RootItem::ReadStatus::Read
                                         ? FeedsModel::ExternalDataChange::MarkedRead
                                         : FeedsModel::ExternalDataChange::MarkedUnread);
}

QString Feed::getAutoUpdateStatusDescription() const {
  QString auto_update_string;

  switch (autoUpdateType()) {
    case AutoUpdateType::DontAutoUpdate:
      //: Describes feed auto-update status.
      auto_update_string = tr("does not use auto-fetching of articles");
      break;

    case AutoUpdateType::DefaultAutoUpdate:
      //: Describes feed auto-update status.
      if (qApp->feedReader()->autoUpdateEnabled()) {
        int secs_to_next =
          QDateTime::currentDateTimeUtc()
            .secsTo(qApp->feedReader()->lastAutoUpdate().addSecs(qApp->feedReader()->autoUpdateInterval()));

        auto_update_string =
          tr("uses global settings (%n minute(s) to next auto-fetch of articles)", nullptr, int(secs_to_next / 60.0));
      }
      else {
        auto_update_string = tr("uses global settings, but global auto-fetching "
                                "of articles is disabled");
      }

      break;

    case AutoUpdateType::SpecificAutoUpdate:
    default:
      int secs_to_next = QDateTime::currentDateTimeUtc().secsTo(lastUpdated().addSecs(autoUpdateInterval()));

      //: Describes feed auto-update status.
      auto_update_string = tr("uses specific settings (%n minute(s) to next "
                              "auto-fetching of new articles)",
                              nullptr,
                              int(secs_to_next / 60.0));
      break;
  }

  return auto_update_string;
}

QString Feed::getStatusDescription() const {
  switch (m_status) {
    case Status::Normal:
      return tr("no errors");

    case Status::NewMessages:
      return tr("has new articles");

    case Status::AuthError:
      return tr("authentication error");

    case Status::SqlError:
      return tr("SQL database error");

    case Status::NetworkError:
      return tr("network error");

    case Status::ParsingError:
      return tr("parsing error");

    default:
      return tr("error");
  }
}

Feed::ArticleIgnoreLimit& Feed::articleIgnoreLimit() {
  return m_articleIgnoreLimit;
}

const Feed::ArticleIgnoreLimit& Feed::articleIgnoreLimit() const {
  return m_articleIgnoreLimit;
}

void Feed::setArticleIgnoreLimit(const ArticleIgnoreLimit& ignore_limit) {
  m_articleIgnoreLimit = ignore_limit;
}

bool Feed::isQuiet() const {
  return m_isQuiet;
}

void Feed::setIsQuiet(bool quiet) {
  m_isQuiet = quiet;
}

QDateTime Feed::lastUpdated() const {
  return m_lastUpdated;
}

void Feed::setLastUpdated(const QDateTime& last_updated) {
  m_lastUpdated = last_updated;
}

bool Feed::isSwitchedOff() const {
  return m_isSwitchedOff;
}

void Feed::setIsSwitchedOff(bool switched_off) {
  m_isSwitchedOff = switched_off;
}

QString Feed::statusString() const {
  return m_statusString;
}

QList<QPointer<MessageFilter>> Feed::messageFilters() const {
  return m_messageFilters;
}

void Feed::setMessageFilters(const QList<QPointer<MessageFilter>>& filters) {
  m_messageFilters = filters;
}

void Feed::removeMessageFilter(MessageFilter* filter) {
  m_messageFilters.removeAll(filter);
}

QString Feed::additionalTooltip() const {
  QString stat = getStatusDescription();

  if (!m_statusString.simplified().isEmpty()) {
    stat += QSL(" (%1)").arg(m_statusString);
  }

  auto fltrs = qlinq::from(m_messageFilters)
                 .select([](const QPointer<MessageFilter>& pn) {
                   return pn->name();
                 })
                 .toList();
  QString source_str = QUrl(m_source).isValid() ? QSL("<a href=\"%1\">%1</a>").arg(m_source) : m_source;

  return tr("Auto-update status: %1\n"
            "Active message filters: %2\n"
            "Status: %3\n"
            "Source: %4\n"
            "Item custom ID: %5")
    .arg(getAutoUpdateStatusDescription(),
         m_messageFilters.size() > 0
           ? QSL("%1 (%2)").arg(QString::number(m_messageFilters.size()), fltrs.join(QSL(", ")))
           : QString::number(m_messageFilters.size()),
         stat,
         source_str,
         customId());
}

Qt::ItemFlags Feed::additionalFlags() const {
  return Qt::ItemFlag::ItemNeverHasChildren;
}

Feed::ArticleIgnoreLimit Feed::ArticleIgnoreLimit::fromSettings() {
  Feed::ArticleIgnoreLimit art_limit;

  art_limit.m_avoidOldArticles = qApp->settings()->value(GROUP(Messages), SETTING(Messages::AvoidOldArticles)).toBool();
  art_limit.m_dtToAvoid =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::DateTimeToAvoidArticle)).toDateTime();
  art_limit.m_hoursToAvoid = qApp->settings()->value(GROUP(Messages), SETTING(Messages::HoursToAvoidArticle)).toInt();

  art_limit.m_doNotRemoveStarred =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::LimitDoNotRemoveStarred)).toBool();
  art_limit.m_doNotRemoveUnread =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::LimitDoNotRemoveUnread)).toBool();
  art_limit.m_keepCountOfArticles =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::LimitCountOfArticles)).toInt();
  art_limit.m_moveToBinDontPurge =
    qApp->settings()->value(GROUP(Messages), SETTING(Messages::LimitRecycleInsteadOfPurging)).toBool();

  return art_limit;
}
