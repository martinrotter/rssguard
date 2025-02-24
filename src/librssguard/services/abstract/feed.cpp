// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/feed.h"

#include "3rd-party/boolinq/boolinq.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/gui/formfeeddetails.h"
#include "services/abstract/serviceroot.h"

Feed::Feed(RootItem* parent)
  : RootItem(parent), m_source(QString()), m_status(Status::Normal), m_statusString(QString()),
    m_autoUpdateType(AutoUpdateType::DefaultAutoUpdate), m_autoUpdateInterval(DEFAULT_AUTO_UPDATE_INTERVAL),
    m_lastUpdated(QDateTime::currentDateTimeUtc()), m_isSwitchedOff(false), m_isQuiet(false),
    m_openArticlesDirectly(false), m_isRtl(false), m_messageFilters(QList<QPointer<MessageFilter>>()) {
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
  setOpenArticlesDirectly(other.openArticlesDirectly());
  setArticleIgnoreLimit(Feed::ArticleIgnoreLimit(other.articleIgnoreLimit()));
  setIsRtl(other.isRtl());
  setIsSwitchedOff(other.isSwitchedOff());
  setIsQuiet(other.isQuiet());
}

QList<Message> Feed::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedMessagesForFeed(database, customId(), getParentServiceRoot()->accountId());
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
        case Status::OtherError:
          return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgSelectedError);

        default:
          return QVariant();
      }

    case TEXT_DIRECTION_ROLE: {
      if (column == FDS_MODEL_TITLE_INDEX) {
        const bool m_useRtlForFeedTitles = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UseRtlForFeedTitles)).toBool();
        return isRtl() && m_useRtlForFeedTitles ? Qt::LayoutDirection::RightToLeft : Qt::LayoutDirection::LayoutDirectionAuto;
      }
      else {
        return Qt::LayoutDirection::LayoutDirectionAuto;
      }
    }

    case Qt::ItemDataRole::ForegroundRole:
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
        case Status::OtherError:
          return qApp->skins()->colorForModel(SkinEnums::PaletteColors::FgError);

        default:
          return QVariant();
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

bool Feed::openArticlesDirectly() const {
  return m_openArticlesDirectly;
}

void Feed::setOpenArticlesDirectly(bool opn) {
  m_openArticlesDirectly = opn;
}

bool Feed::isRtl() const {
  return m_isRtl;
}

void Feed::setIsRtl(bool rtl) {
  m_isRtl = rtl;
}

bool Feed::removeUnwantedArticles(QSqlDatabase& db) {
  Feed::ArticleIgnoreLimit feed_setup = articleIgnoreLimit();
  Feed::ArticleIgnoreLimit app_setup = Feed::ArticleIgnoreLimit::fromSettings();

  return DatabaseQueries::removeUnwantedArticlesFromFeed(db, this, feed_setup, app_setup);
}

void Feed::appendMessageFilter(MessageFilter* filter) {
  removeMessageFilter(filter);
  m_messageFilters.append(QPointer<MessageFilter>(filter));
}

void Feed::updateCounts(bool including_total_count) {
  QSqlDatabase database = qApp->database()->driver()->threadSafeConnection(metaObject()->className());
  int account_id = getParentServiceRoot()->accountId();
  auto fc = DatabaseQueries::getMessageCountsForFeed(database, customId(), account_id);

  if (including_total_count) {
    setCountOfAllMessages(fc.m_total);
  }

  setCountOfUnreadMessages(fc.m_unread);
}

bool Feed::cleanMessages(bool clean_read_only) {
  return getParentServiceRoot()->cleanFeeds(QList<Feed*>() << this, clean_read_only);
}

bool Feed::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = getParentServiceRoot();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this, status), status);
  }

  return service->markFeedsReadUnread({this}, status);
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

  auto std_fltrs = boolinq::from(m_messageFilters)
                     .select([](const QPointer<MessageFilter>& pn) {
                       return pn->name();
                     })
                     .toStdList();
  QStringList fltrs = FROM_STD_LIST(QStringList, std_fltrs);

  return tr("Auto-update status: %1\n"
            "Active message filters: %2\n"
            "Status: %3\n"
            "Source: <a href=\"%4\">%4</a>\n"
            "Item ID: %5")
    .arg(getAutoUpdateStatusDescription(),
         m_messageFilters.size() > 0
           ? QSL("%1 (%2)").arg(QString::number(m_messageFilters.size()), fltrs.join(QSL(", ")))
           : QString::number(m_messageFilters.size()),
         stat,
         m_source,
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
