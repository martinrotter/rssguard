// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/feed.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/cacheforserviceroot.h"
#include "services/abstract/gui/formfeeddetails.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/recyclebin.h"
#include "services/abstract/serviceroot.h"
#include "services/abstract/unreadnode.h"

#include <QThread>

Feed::Feed(RootItem* parent)
  : RootItem(parent), m_source(QString()), m_status(Status::Normal), m_statusString(QString()), m_autoUpdateType(AutoUpdateType::DefaultAutoUpdate),
  m_autoUpdateInitialInterval(DEFAULT_AUTO_UPDATE_INTERVAL), m_autoUpdateRemainingInterval(DEFAULT_AUTO_UPDATE_INTERVAL),
  m_messageFilters(QList<QPointer<MessageFilter>>()) {
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
  setAutoUpdateInitialInterval(other.autoUpdateInitialInterval());
  setAutoUpdateRemainingInterval(other.autoUpdateRemainingInterval());
  setMessageFilters(other.messageFilters());
}

QList<Message> Feed::undeletedMessages() const {
  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());

  return DatabaseQueries::getUndeletedMessagesForFeed(database, customId(), getParentServiceRoot()->accountId());
}

QVariant Feed::data(int column, int role) const {
  switch (role) {
    case Qt::ItemDataRole::ForegroundRole:
      switch (status()) {
        case Status::NewMessages:
          return qApp->skins()->currentSkin().m_colorPalette[Skin::PaletteColors::Highlight];

        case Status::NetworkError:
        case Status::ParsingError:
        case Status::AuthError:
        case Status::OtherError:
          return qApp->skins()->currentSkin().m_colorPalette[Skin::PaletteColors::Error];

        default:
          return QVariant();
      }

    default:
      return RootItem::data(column, role);
  }
}

int Feed::autoUpdateInitialInterval() const {
  return m_autoUpdateInitialInterval;
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

bool Feed::editViaGui() {
  QScopedPointer<FormFeedDetails> form_pointer(new FormFeedDetails(getParentServiceRoot(), qApp->mainFormWidget()));

  form_pointer->addEditFeed(this);
  return false;
}

void Feed::setAutoUpdateInitialInterval(int auto_update_interval) {
  // If new initial auto-update interval is set, then
  // we should reset time that remains to the next auto-update.
  m_autoUpdateInitialInterval = auto_update_interval;
  m_autoUpdateRemainingInterval = auto_update_interval;
}

Feed::AutoUpdateType Feed::autoUpdateType() const {
  return m_autoUpdateType;
}

void Feed::setAutoUpdateType(Feed::AutoUpdateType auto_update_type) {
  m_autoUpdateType = auto_update_type;
}

int Feed::autoUpdateRemainingInterval() const {
  return m_autoUpdateRemainingInterval;
}

void Feed::setAutoUpdateRemainingInterval(int auto_update_remaining_interval) {
  m_autoUpdateRemainingInterval = auto_update_remaining_interval;
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

void Feed::appendMessageFilter(MessageFilter* filter) {
  m_messageFilters.append(QPointer<MessageFilter>(filter));
}

void Feed::updateCounts(bool including_total_count) {
  bool is_main_thread = QThread::currentThread() == qApp->thread();
  QSqlDatabase database = is_main_thread ?
                          qApp->database()->driver()->connection(metaObject()->className()) :
                          qApp->database()->driver()->connection(QSL("feed_upd"));
  int account_id = getParentServiceRoot()->accountId();

  if (including_total_count) {
    setCountOfAllMessages(DatabaseQueries::getMessageCountsForFeed(database, customId(), account_id, true));
  }

  setCountOfUnreadMessages(DatabaseQueries::getMessageCountsForFeed(database, customId(), account_id, false));
}

bool Feed::cleanMessages(bool clean_read_only) {
  return getParentServiceRoot()->cleanFeeds(QList<Feed*>() << this, clean_read_only);
}

bool Feed::markAsReadUnread(RootItem::ReadStatus status) {
  ServiceRoot* service = getParentServiceRoot();
  auto* cache = dynamic_cast<CacheForServiceRoot*>(service);

  if (cache != nullptr) {
    cache->addMessageStatesToCache(service->customIDSOfMessagesForItem(this), status);
  }

  return service->markFeedsReadUnread(QList<Feed*>() << this, status);
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
      auto_update_string = qApp->feedReader()->autoUpdateEnabled()
              ? tr("uses global settings (%n minute(s) to next auto-fetch of articles)",
                   nullptr,
                   qApp->feedReader()->autoUpdateRemainingInterval())
              : tr("uses global settings (global auto-fetching of articles is disabled)");
      break;

    case AutoUpdateType::SpecificAutoUpdate:
    default:

      //: Describes feed auto-update status.
      auto_update_string = tr("uses specific settings (%n minute(s) to next auto-fetching of new articles)", nullptr, autoUpdateRemainingInterval());
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
  int idx = m_messageFilters.indexOf(filter);

  if (idx >= 0) {
    m_messageFilters.removeAll(filter);
  }
}

QString Feed::additionalTooltip() const {
  QString stat = getStatusDescription();

  if (!m_statusString.simplified().isEmpty()) {
    stat += QSL(" (%1)").arg(m_statusString);
  }

  return tr("Auto-update status: %1\n"
            "Active message filters: %2\n"
            "Status: %3").arg(getAutoUpdateStatusDescription(),
                              QString::number(m_messageFilters.size()),
                              stat);
}
