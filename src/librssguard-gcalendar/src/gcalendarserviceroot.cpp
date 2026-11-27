// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendarserviceroot.h"

#include "gcalendarentrypoint.h"
#include "gcalendarfeed.h"
#include "gui/calendareventpreviewer.h"
#include "gui/formcalendarevent.h"
#include "gui/formeditgcalendaraccount.h"
#include "gui/formgcalendarfeeddetails.h"

#include "ui_formcalendareventdetails.h"

#include <algorithm>
#include <memory>
#include <librssguard/database/databasequeries.h>
#include <librssguard/exceptions/sqlexception.h>
#include <librssguard/gui/guiutilities.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/feedreader.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/mutex.h>
#include <librssguard/miscellaneous/settings.h>
#include <librssguard/miscellaneous/settingskeys.h>
#include <librssguard/network-web/oauth2service.h>
#include <librssguard/services/abstract/category.h>

#include <QAction>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QMenu>
#include <QMetaType>
#include <QMutexLocker>
#include <QScopedPointer>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QtConcurrent>

#if QT_VERSION_MAJOR < 6
Q_DECLARE_METATYPE(ServiceRoot::SyncInResult)
#endif

namespace {
  void validateCachedStart(const CalendarEvent& event) {
    if (event.m_start.isValid() && event.m_start.toMSecsSinceEpoch() < 0) {
      throw CalendarApiException(
        Feed::Status::ParsingError,
        GCalendarServiceRoot::tr("Events starting before 1970 cannot be cached by RSS Guard. "
                                 "Manage these events in Google Calendar."));
    }
  }

  QJsonObject readMetadata(const QString& text) {
    const auto document = QJsonDocument::fromJson(text.toUtf8());
    if (!document.isObject()) {
      throw ApplicationException(QObject::tr("Invalid calendar custom data; existing data was preserved."));
    }
    return document.object();
  }

  QVariantHash accountCustomData(const QSqlDatabase& db, int account_id) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT custom_data FROM Accounts WHERE id = :id;"));
    query.bindValue(QSL(":id"), account_id);
    query.exec();
    if (!query.next()) {
      throw ApplicationException(QObject::tr("This Calendar account no longer exists."));
    }
    return readMetadata(query.value(0).toString()).toVariantHash();
  }

  QVariantHash updateAccountCustomData(const QSqlDatabase& db,
                                       int account_id,
                                       const std::function<void(QVariantHash&)>& update) {
    auto data = accountCustomData(db, account_id);
    update(data);
    DatabaseQueries::storeCustomData(db, QSL("Accounts"), account_id, data);
    return data;
  }

  QVariantHash feedCustomData(const QSqlDatabase& db, int feed_id, int account_id) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT custom_data FROM Feeds WHERE id = :feed AND account_id = :account;"));
    query.bindValue(QSL(":feed"), feed_id);
    query.bindValue(QSL(":account"), account_id);
    query.exec();
    if (!query.next()) {
      throw ApplicationException(QObject::tr("This calendar no longer exists."));
    }
    return readMetadata(query.value(0).toString()).toVariantHash();
  }

  QVariantHash calendarDataWithOverride(QVariantHash data,
                                        const QJsonObject& requested,
                                        const CalendarReminderSettings& account,
                                        int future_days,
                                        const QDateTime& now) {
    auto calendar = QJsonObject::fromVariantHash(data).value(QSL("gcalendar")).toObject();
    GCalendarFeed validation;
    validation.setCustomDatabaseData(data);
    validation.setCalendarResource(calendar.value(QSL("resource")).toObject());
    CalendarReminderSettings::forCalendar(account, calendar);
    if (!requested.isEmpty()) {
      const auto previous = calendar.value(QSL("reminder_override")).toObject();
      auto settings = previous;
      settings[QSL("mode")] = requested.value(QSL("mode"));
      settings[QSL("version")] = 1;
      settings[QSL("enabled_at")] =
        previous.value(QSL("mode")) == settings.value(QSL("mode")) && previous.contains(QSL("enabled_at"))
          ? previous.value(QSL("enabled_at"))
          : QJsonValue(now.toString(Qt::ISODateWithMs));
      if (settings.value(QSL("mode")) == QLatin1String("local")) {
        if (!requested.value(QSL("rules")).isArray() || requested.value(QSL("rules")).toArray().isEmpty()) {
          throw ApplicationException(GCalendarServiceRoot::tr("Enter at least one local reminder offset."));
        }
        QHash<int, QJsonObject> old_rules;
        for (const auto& value : previous.value(QSL("rules")).toArray()) {
          const auto rule = value.toObject();
          old_rules.insert(rule.value(QSL("minutes_before")).toInt(), rule);
        }
        QJsonArray rules;
        QSet<int> offsets;
        for (const auto& value : requested.value(QSL("rules")).toArray()) {
          const auto requested_rule = value.toObject();
          const int before = requested_rule.value(QSL("minutes_before")).toInt(-1);
          if (offsets.contains(before)) {
            throw ApplicationException(GCalendarServiceRoot::
                                         tr("Enter distinct nonnegative reminder offsets in minutes."));
          }
          offsets.insert(before);
          auto rule = old_rules.value(before, requested_rule);
          rule[QSL("minutes_before")] = requested_rule.value(QSL("minutes_before"));
          rule[QSL("repeat_minutes")] = requested_rule.value(QSL("repeat_minutes"));
          if (!rule.contains(QSL("enabled_at"))) {
            rule[QSL("enabled_at")] = now.toString(Qt::ISODateWithMs);
          }
          rules.append(rule);
        }
        settings[QSL("rules")] = rules;
      }
      calendar[QSL("reminder_override")] = settings;
    }
    const auto effective = CalendarReminderSettings::forCalendar(account, calendar);
    if (calendar.value(QSL("reminder_override")).toObject().value(QSL("mode")) != QLatin1String("disabled")) {
      effective.validateCacheWindow(future_days);
    }
    data[QSL("gcalendar")] = calendar.toVariantHash();
    return data;
  }
} // namespace

GCalendarServiceRoot::GCalendarServiceRoot(RootItem* parent, GCalendarNetworkFactory::Transport transport)
  : ServiceRoot(parent), m_transport(std::move(transport)) {
  setTitle(tr("Google Calendar"));
  setIcon(qApp->icons()->miscIcon(QSL("gcalendar")));
#if QT_VERSION_MAJOR < 6
  qRegisterMetaType<SyncInResult>("SyncInResult");
#endif
  connect(this, &ServiceRoot::syncInFinished, this, [this](const SyncInResult& result) {
    const bool succeeded = std::holds_alternative<RootItem*>(result);
    if (m_discoveryHasFeedLock) {
      m_discoveryHasFeedLock = false;
      qApp->feedUpdateLock()->unlock();
    }
    emit calendarDiscoveryFinished(succeeded);
  });
}

GCalendarServiceRoot::~GCalendarServiceRoot() {
  stop();
  delete m_previewer;
}

QString GCalendarServiceRoot::code() const {
  return QString::fromLatin1(GCALENDAR_SERVICE_CODE);
}

bool GCalendarServiceRoot::supportsFeedAdding() const {
  return false;
}

bool GCalendarServiceRoot::supportsCategoryAdding() const {
  return false;
}

bool GCalendarServiceRoot::displaysEnclosures() const {
  return false;
}

bool GCalendarServiceRoot::alwaysDetectsArticleContentChanges() const {
  return true;
}

bool GCalendarServiceRoot::canBeEdited() const {
  return true;
}

bool GCalendarServiceRoot::canBeDeleted() const {
  return ServiceRoot::canBeDeleted() && m_eventOperations.isEmpty();
}

FormAccountDetails* GCalendarServiceRoot::accountSetupDialog() const {
  return new FormEditGCalendarAccount(qApp->mainFormWidget());
}

void GCalendarServiceRoot::editItems(const QList<RootItem*>& items) {
  const auto feeds = qlinq::from(items).ofType<Feed*>().toList();
  if (!feeds.isEmpty()) {
    try {
      FormGCalendarFeedDetails dialog(this, qApp->mainFormWidget());
      dialog.addEditFeed<Feed>(feeds);
    }
    catch (const ApplicationException& ex) {
      reportOperationFailure(tr("Cannot edit calendar"), ex.message(), true);
    }
    catch (...) {
      reportOperationFailure(tr("Cannot edit calendar"), tr("An unexpected error occurred."), true);
    }
    return;
  }
  if (items.first()->kind() == RootItem::Kind::ServiceRoot) {
    QScopedPointer<FormEditGCalendarAccount> dialog(qobject_cast<FormEditGCalendarAccount*>(accountSetupDialog()));
    dialog->addEditAccount(this);
    return;
  }

  ServiceRoot::editItems(items);
}

QJsonObject GCalendarServiceRoot::metadata(const QVariantHash& data) {
  const auto value = QJsonObject::fromVariantHash(data).value(QSL("gcalendar"));
  if (!value.isUndefined() && (!value.isObject() || value.toObject().value(QSL("version")).toInt() != 1)) {
    throw ApplicationException(tr("Unsupported Calendar account data version."));
  }
  auto result = value.toObject();
  for (const auto& key : {QSL("reminders"), QSL("ledger")}) {
    if (result.contains(key) && !result.value(key).isObject()) {
      throw ApplicationException(tr("Invalid Calendar account settings or reminder history."));
    }
  }
  result[QSL("version")] = 1;
  return result;
}

QVariantHash GCalendarServiceRoot::customDatabaseData() const {
  QMutexLocker lock(&m_stateMutex);
  auto data = m_data;
  const auto common = ServiceRoot::customDatabaseData();
  for (auto it = common.constBegin(); it != common.constEnd(); ++it) {
    data[it.key()] = it.value();
  }
  if (!data.contains(QSL("gcalendar"))) {
    data[QSL("gcalendar")] = metadata(data).toVariantHash();
  }
  return data;
}

void GCalendarServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  const auto calendar = metadata(data);
  CalendarReminderSettings::fromJson(calendar.value(QSL("reminders")).toObject());
  CalendarReminders::entries(calendar.value(QSL("ledger")).toObject());
  ServiceRoot::setCustomDatabaseData(data);
  {
    QMutexLocker lock(&m_stateMutex);
    m_data = data;
  }
  setTitle(calendar.value(QSL("name")).toString(tr("Google Calendar")));
}

QJsonObject GCalendarServiceRoot::calendarSettings() const {
  return metadata(customDatabaseData());
}

CalendarReminderSettings GCalendarServiceRoot::reminderSettings() const {
  return CalendarReminderSettings::fromJson(calendarSettings().value(QSL("reminders")).toObject());
}

void GCalendarServiceRoot::saveAccountDataToDatabase() {
  saveAccountDataToDatabase(QString());
}

void GCalendarServiceRoot::saveAccountDataToDatabase(const QString& refresh_token) {
  const auto desired = customDatabaseData();
  const auto common = ServiceRoot::customDatabaseData();
  const int original_id = accountId();
  const int original_order = sortOrder();
  QVariantHash saved;
  try {
    saved = qApp->database()->worker()->write<QVariantHash>([&](const QSqlDatabase& db) {
      auto data = accountId() > 0 ? accountCustomData(db, accountId()) : desired;
      auto calendar = metadata(data);
      CalendarReminderSettings::fromJson(calendar.value(QSL("reminders")).toObject());
      CalendarReminders::entries(calendar.value(QSL("ledger")).toObject());
      const auto configuration = metadata(desired);
      // Runtime tokens and the trigger ledger always come from the current database row.
      for (const auto& key : {QSL("name"),
                              QSL("history_days"),
                              QSL("future_days"),
                              QSL("reminders"),
                              QSL("refresh_calendar_list_on_startup")}) {
        if (configuration.contains(key)) {
          calendar[key] = configuration.value(key);
        }
      }
      data[QSL("gcalendar")] = calendar.toVariantHash();
      for (const auto& key : {QSL("client_id"), QSL("client_secret")}) {
        if (data.value(key) != desired.value(key)) {
          data.remove(QSL("refresh_token"));
        }
        data[key] = desired.value(key);
      }
      if (!refresh_token.isEmpty()) {
        data[QSL("refresh_token")] = refresh_token;
      }
      for (auto it = common.constBegin(); it != common.constEnd(); ++it) {
        data[it.key()] = it.value();
      }
      auto connection = db;
      if (!connection.transaction()) {
        throw SqlException(connection.lastError());
      }
      try {
        {
          QMutexLocker lock(&m_stateMutex);
          m_data = data;
        }
        DatabaseQueries::createOverwriteAccount(db, this);
        if (!connection.commit()) {
          throw SqlException(connection.lastError());
        }
      }
      catch (...) {
        connection.rollback();
        throw;
      }
      return data;
    });
  }
  catch (...) {
    {
      QMutexLocker lock(&m_stateMutex);
      m_data = desired;
    }
    setAccountId(original_id);
    setSortOrder(original_order);
    throw;
  }
  setCustomDatabaseData(saved);
}

void GCalendarServiceRoot::applySettings(const QString& name,
                                         const QString& client_id,
                                         const QString& client_secret,
                                         int history_days,
                                         int future_days,
                                         CalendarReminderSettings reminders,
                                         const QString& refresh_token,
                                         bool refresh_calendar_list_on_startup) {
  std::unique_lock mutation(m_mutationMutex, std::try_to_lock);
  if (!mutation.owns_lock()) {
    throw ApplicationException(tr("Wait for the current calendar update before changing account settings."));
  }
  if (name.trimmed().isEmpty() || history_days < 0 || history_days > 36500 || future_days < 1 || future_days > 36500) {
    throw ApplicationException(tr("Enter an account name and a valid calendar cache window."));
  }
  if (client_id.trimmed().isEmpty()) {
#if !defined(GCALENDAR_OFFICIAL_SUPPORT)
    throw ApplicationException(tr("Enter your Google desktop application's client ID."));
#endif
  }
  const auto previous = reminderSettings();
  if (reminders.m_enabled && (!previous.m_enabled || !previous.m_enabledAt.isValid())) {
    reminders.m_enabledAt = reminderNow();
  }
  else {
    reminders.m_enabledAt = previous.m_enabledAt;
  }
  for (auto& rule : reminders.m_rules) {
    const auto old = std::find_if(previous.m_rules.constBegin(),
                                  previous.m_rules.constEnd(),
                                  [&](const CalendarReminderRule& candidate) {
                                    return candidate.m_id == rule.m_id;
                                  });
    rule.m_enabledAt = old == previous.m_rules.constEnd() ? reminderNow() : old->m_enabledAt;
  }
  reminders = CalendarReminderSettings::fromJson(reminders.toJson());
  reminders.validateCacheWindow(future_days);
  for (auto* feed : getSubTreeFeeds()) {
    const auto* calendar = dynamic_cast<GCalendarFeed*>(feed);
    if (calendar != nullptr &&
        calendar->calendarMetadata().value(QSL("reminder_override")).toObject().value(QSL("mode")).toString() !=
          QLatin1String("disabled")) {
      CalendarReminderSettings::forCalendar(reminders, calendar->calendarMetadata()).validateCacheWindow(future_days);
    }
  }
  auto data = customDatabaseData();
  auto calendar = metadata(data);
  calendar[QSL("name")] = name.trimmed();
  calendar[QSL("history_days")] = history_days;
  calendar[QSL("future_days")] = future_days;
  calendar[QSL("reminders")] = reminders.toJson();
  calendar[QSL("refresh_calendar_list_on_startup")] = refresh_calendar_list_on_startup;
  data[QSL("gcalendar")] = calendar.toVariantHash();
  const bool changed_credentials = data.value(QSL("client_id")).toString() != client_id.trimmed() ||
                                   data.value(QSL("client_secret")).toString() != client_secret.trimmed();
  data[QSL("client_id")] = client_id.trimmed();
  data[QSL("client_secret")] = client_secret.trimmed();
  const auto old = customDatabaseData();
  setCustomDatabaseData(data);
  try {
    saveAccountDataToDatabase(refresh_token);
  }
  catch (...) {
    setCustomDatabaseData(old);
    throw;
  }
  if (changed_credentials || !refresh_token.isEmpty()) {
    invalidateAuthorization(true);
    m_discoverAfterAuth = false;
    if (m_oauth != nullptr) {
      m_oauth->logout();
      delete m_oauth;
    }
  }
  if (m_started) {
    ensureOAuth();
    ++m_configurationGeneration;
    restartReminderTimer(reminders.m_enabled && !previous.m_enabled);
    if (!refresh_token.isEmpty()) {
      m_discoverAfterAuth = true;
      requestAccessToken();
    }
    else if (changed_credentials) {
      setStatus(tr("Authorize this account to refresh calendars. Cached events remain available."));
    }
  }
  itemChanged({this});
}

void GCalendarServiceRoot::setStatus(const QString& status) {
  m_status = status;
  emit statusChanged(status);
  itemChanged({this});
}

void GCalendarServiceRoot::reportOperationFailure(const QString& title, const QString& error, bool interactive) {
  setStatus(error);
  qWarningNN << LOGSEC_CORE << NONQUOTE_W_SPACE(title) << "for Calendar account" << accountId()
             << NONQUOTE_W_SPACE_DOT(error);
  if (interactive) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent, {title, error, QSystemTrayIcon::Critical}, {true, true});
  }
}

void GCalendarServiceRoot::reportAuthenticationFailure(const QString& error) {
  if (!m_started) {
    return;
  }
  const auto message = tr("Authorization failed: %1").arg(error);
  reportOperationFailure(tr("Google Calendar authorization failed"), message, false);
  if (!m_loginFailureReported) {
    m_loginFailureReported = true;
    QPointer<GCalendarServiceRoot> account(this);
    qApp->showGuiMessage(Notification::Event::LoginFailure,
                         {tr("Google Calendar authorization failed"),
                          tr("Account '%1': %2").arg(title(), error),
                          QSystemTrayIcon::Critical},
                         {},
                         {tr("Authorize"), qApp, [account]() {
                            if (account != nullptr) {
                              account->authorize();
                            }
                          }});
  }
}

QString GCalendarServiceRoot::additionalTooltip() const {
  return m_status.toHtmlEscaped();
}

void GCalendarServiceRoot::ensureOAuth() {
  if (m_oauth != nullptr) {
    m_oauth->setTokenRequestProxy(networkProxy());
    return;
  }
  const auto data = customDatabaseData();
  m_oauth = GCalendarNetworkFactory::createOAuth(data.value(QSL("client_id")).toString(),
                                                 data.value(QSL("client_secret")).toString(),
                                                 networkProxy(),
                                                 this);
  m_oauth->setRefreshToken(data.value(QSL("refresh_token")).toString());
  connect(m_oauth, &OAuth2Service::tokensRetrieved, this, [this](const QString& access, const QString& refresh, int) {
    if (!m_started) {
      return;
    }
    {
      QMutexLocker lock(&m_stateMutex);
      m_context.m_bearer = QSL("Bearer ") + access;
      m_bearerExpires = m_oauth->tokensExpireIn();
    }
    authenticationFinished({});
    try {
      const auto data = qApp->database()->worker()->write<QVariantHash>([&](const QSqlDatabase& db) {
        if (!refresh.isEmpty()) {
          DatabaseQueries::storeNewOauthTokens(db, refresh, accountId());
        }
        return accountCustomData(db, accountId());
      });
      setCustomDatabaseData(data);
      setStatus(tr("Authorized"));
      discoverAfterAuthorization();
    }
    catch (const ApplicationException& ex) {
      setStatus(ex.message());
    }
  });
  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, [this](const QString&, const QString& description) {
    const auto error = description.isEmpty() ? tr("Calendar authorization failed.") : description;
    authenticationFinished(error);
    reportAuthenticationFailure(error);
  });
  connect(m_oauth, &OAuth2Service::authFailed, this, [this]() {
    const auto error = tr("Authorization was declined.");
    authenticationFinished(error);
    reportAuthenticationFailure(error);
  });
  connect(this, &ServiceRoot::proxyChanged, m_oauth, &OAuth2Service::setTokenRequestProxy);
  connect(this, &ServiceRoot::proxyChanged, m_oauth, [this](const QNetworkProxy& proxy) {
    QMutexLocker lock(&m_stateMutex);
    m_context.m_proxy = proxy;
  });
}

void GCalendarServiceRoot::refreshAccessToken() {
  m_oauth->refreshAccessToken();
}

void GCalendarServiceRoot::requestAccessToken() {
  if (!m_started) {
    return;
  }
  ensureOAuth();
  {
    QMutexLocker lock(&m_stateMutex);
    m_authorizationPending = true;
  }
  if (m_oauth->refreshToken().isEmpty()) {
    const auto error = tr("Authorize the Calendar account before refreshing events.");
    authenticationFinished(error);
    reportAuthenticationFailure(error);
    return;
  }
  refreshAccessToken();
}

void GCalendarServiceRoot::authenticationFinished(const QString& error) {
  QMutexLocker lock(&m_stateMutex);
  if (!error.isEmpty()) {
    m_context.m_bearer.clear();
    m_bearerExpires = {};
  }
  m_authorizationError = error;
  m_authorizationPending = false;
  ++m_authorizationGeneration;
  m_authorizationReady.wakeAll();
}

void GCalendarServiceRoot::invalidateAuthorization(bool renew_lifetime) {
  QMutexLocker lock(&m_stateMutex);
  m_context.m_cancelled->store(true);
  if (renew_lifetime) {
    m_context.m_cancelled = std::make_shared<std::atomic_bool>(false);
  }
  m_context.m_bearer.clear();
  m_bearerExpires = {};
  m_authorizationError.clear();
  m_authorizationPending = false;
  ++m_authorizationGeneration;
  m_authorizationReady.wakeAll();
}

void GCalendarServiceRoot::discoverAfterAuthorization() {
  if (!m_discoverAfterAuth || syncInRunning() || !m_started) {
    return;
  }
  {
    QMutexLocker lock(&m_stateMutex);
    if (m_authorizationPending || m_context.m_bearer.isEmpty()) {
      return;
    }
  }
  m_discoverAfterAuth = false;
  refreshCalendarList();
}

QString GCalendarServiceRoot::authorizedBearer(const QString& rejected,
                                               const GCalendarNetworkFactory::Context& context) {
  QMutexLocker lock(&m_stateMutex);
  const auto valid = [this]() {
    return !m_context.m_bearer.isEmpty() && m_bearerExpires > QDateTime::currentDateTimeUtc();
  };
  context.checkCancelled();
  if (!m_authorizationPending && valid() && (rejected.isEmpty() || rejected != m_context.m_bearer)) {
    return m_context.m_bearer;
  }
  // HTTP jobs may wait for OAuth, but must never block its owning GUI thread.
  if (QThread::currentThread() == thread()) {
    throw CalendarApiException(Feed::Status::AuthError,
                                tr("Authorize the Calendar account before refreshing events."),
                                401);
  }
  const auto generation = m_authorizationGeneration;
  if (!m_authorizationPending) {
    m_authorizationPending = true;
    QMetaObject::invokeMethod(
      this,
      [this, lifetime = context.m_cancelled, generation]() {
        {
          QMutexLocker lock(&m_stateMutex);
          if (lifetime->load() || !m_authorizationPending || generation != m_authorizationGeneration) {
            return;
          }
        }
        requestAccessToken();
      },
      Qt::QueuedConnection);
  }
  QElapsedTimer elapsed;
  elapsed.start();
  while (m_authorizationPending && generation == m_authorizationGeneration && !context.isCancelled()) {
    const auto remaining = qint64(context.m_timeout) - elapsed.elapsed();
    if (remaining <= 0) {
      throw CalendarApiException(Feed::Status::NetworkError,
                                  tr("Timed out waiting for Calendar authorization."),
                                  NetworkResult(QNetworkReply::TimeoutError, 0, {}, {}));
    }
    m_authorizationReady.wait(&m_stateMutex, static_cast<unsigned long>(std::min(qint64(100), remaining)));
  }
  context.checkCancelled();
  if (!valid()) {
    throw CalendarApiException(Feed::Status::AuthError,
                               m_authorizationError.isEmpty() ? tr("Calendar authorization failed.")
                                                              : m_authorizationError,
                                401);
  }
  return m_context.m_bearer;
}

void GCalendarServiceRoot::start(bool freshly_activated) {
  if (m_started) {
    return;
  }
  {
    QMutexLocker lock(&m_stateMutex);
    m_context = {};
    m_context.m_proxy = networkProxy();
  }
  if (!m_loaded) {
    DatabaseQueries::loadRootFromDatabase<Category, GCalendarFeed>(this);
    m_loaded = true;
    const auto run_pending_discovery = [this]() {
      if (m_discoveryPending) {
        refreshCalendarList();
      }
    };
    connect(qApp->feedUpdateLock(), &Mutex::unlocked, this, run_pending_discovery, Qt::QueuedConnection);
    connect(this, &GCalendarServiceRoot::eventOperationFinished, this, run_pending_discovery, Qt::QueuedConnection);
  }
  m_started = true;
  ensureOAuth();
  restartReminderTimer(true);
  if (!m_oauth->refreshToken().isEmpty()) {
    m_discoverAfterAuth =
      getSubTreeFeeds().isEmpty() || calendarSettings().value(QSL("refresh_calendar_list_on_startup")).toBool(false);
    requestAccessToken();
  }
  else if (freshly_activated) {
    authorize();
  }
  else {
    setStatus(tr("Authorize this account to refresh calendars. Cached events remain available."));
  }
}

void GCalendarServiceRoot::stop() {
  invalidateAuthorization(false);
  m_started = false;
  m_discoveryPending = false;
  waitForSyncInFinished();
  if (m_discoveryHasFeedLock) {
    m_discoveryHasFeedLock = false;
    qApp->feedUpdateLock()->unlock();
  }
  m_discoverAfterAuth = false;
  ++m_configurationGeneration;
  if (m_reminderTimer != nullptr) {
    m_reminderTimer->stop();
  }
  if (m_reminderScan != nullptr) {
    m_reminderScan->waitForFinished();
    m_reminderScan = nullptr;
  }
  m_checkRunning = false;
  for (auto* operation : std::as_const(m_eventOperations)) {
    operation->waitForFinished();
  }
  // Native feed fetching retains this lock through its after-storage callback.
  const std::lock_guard lock(m_mutationMutex);
  if (m_oauth != nullptr) {
    m_oauth->logout();
    delete m_oauth;
  }
  emit accountStopped();
}

GCalendarNetworkFactory::Context GCalendarServiceRoot::requestContext() {
  QMutexLocker lock(&m_stateMutex);
  m_context.checkCancelled();
  if (!m_started) {
    throw CalendarApiException(Feed::Status::AuthError,
                                tr("Authorize the Calendar account before refreshing events."),
                                401);
  }
  auto context = GCalendarNetworkFactory::networkContext(m_context.m_proxy);
  context.m_bearer = m_context.m_bearer;
  context.m_cancelled = m_context.m_cancelled;
  context.m_authorize = [this](const QString& rejected, const GCalendarNetworkFactory::Context& request) {
    return authorizedBearer(rejected, request);
  };
  return context;
}

void GCalendarServiceRoot::authorize() {
  if (!m_started) {
    return;
  }
  ensureOAuth();
  if (m_loginFailureReported) {
    m_oauth->logout();
    QMutexLocker lock(&m_stateMutex);
    m_context.m_bearer.clear();
    m_bearerExpires = {};
  }
  m_discoverAfterAuth = true;
  m_oauth->setRedirectUrl(QSL("http://127.0.0.1:0"), true);
  {
    QMutexLocker lock(&m_stateMutex);
    m_authorizationPending = true;
  }
  if (m_oauth->login()) {
    authenticationFinished({});
    if (!syncInRunning()) {
      m_discoverAfterAuth = false;
      refreshCalendarList();
    }
  }
}

void GCalendarServiceRoot::requestSyncIn() {
  refreshCalendarList();
}

void GCalendarServiceRoot::refreshCalendarList() {
  if (!m_started || syncInRunning() || qApp->feedReader()->isQuitting()) {
    return;
  }
  const auto context = requestContext();
  // The standard sync-in replaces the feed tree. Keep feed pointers valid for active fetches and edits.
  if (!m_eventOperations.isEmpty() || !qApp->feedUpdateLock()->tryLock()) {
    m_discoveryPending = true;
    return;
  }
  m_discoveryPending = false;
  m_discoveryHasFeedLock = true;
  const auto transport = m_transport;
  const auto fallback_icon = icon();
  QHash<QString, QJsonValue> reminder_overrides;
  for (auto* feed : getSubTreeFeeds()) {
    const auto* calendar = static_cast<const GCalendarFeed*>(feed);
    reminder_overrides.insert(feed->customId(), calendar->calendarMetadata().value(QSL("reminder_override")));
  }
  startSyncInTask([this, context, transport, fallback_icon, reminder_overrides]() -> RootItem* {
    const auto calendars = GCalendarNetworkFactory(context, transport).calendars();
    QScopedPointer<RootItem> root(new RootItem());
    QSet<QString> ids;
    for (const auto& resource : calendars) {
      const auto id = resource.value(QSL("id")).toString();
      if (id.isEmpty() || ids.contains(id)) {
        throw ApplicationException(tr("Invalid or duplicate calendar ID; cached calendars were preserved."));
      }
      ids.insert(id);
      if (resource.value(QSL("deleted")).toBool()) {
        continue;
      }
      auto* feed = new GCalendarFeed(root.data());
      root->appendChild(feed);
      const auto reminder_override = reminder_overrides.value(id, QJsonValue(QJsonValue::Undefined));
      if (!reminder_override.isUndefined()) {
        feed->setCustomDatabaseData({{QSL("gcalendar"),
                                      QVariantHash{{QSL("version"), 1},
                                                   {QSL("reminder_override"), reminder_override.toVariant()}}}});
      }
      feed->setCalendarResource(resource);
      const QColor color(resource.value(QSL("backgroundColor")).toString());
      feed->setIcon(color.isValid() ? IconFactory::fromColor(color) : fallback_icon);
      feed->setSource(GCalendarNetworkFactory::apiUrl().toString());
      feed->setIsQuiet(true);
      feed->setExcludesFromGlobalUnreadCount(true);
    }
    context.checkCancelled();
    return root.take();
  });
}

QList<Message> GCalendarServiceRoot::obtainNewMessages(Feed* feed,
                                                       const QHash<BagOfMessages, QStringList>& stated_messages,
                                                       const QHash<QString, QStringList>& tagged_messages) {
  return obtainNewMessagesForUpdate(feed, stated_messages, tagged_messages).messages;
}

FeedFetchResult GCalendarServiceRoot::obtainNewMessagesForUpdate(Feed* feed,
                                                                 const QHash<BagOfMessages, QStringList>&,
                                                                 const QHash<QString, QStringList>&) {
  auto guard = std::make_shared<std::unique_lock<std::mutex>>(m_mutationMutex);
  auto context = requestContext();
  auto snapshot = fetchSnapshot(feed, context);
  if (snapshot.afterMessagesStored) {
    snapshot.afterMessagesStored = [guard, after_stored = std::move(snapshot.afterMessagesStored)]() { after_stored(); };
  }
  return snapshot;
}

FeedFetchResult GCalendarServiceRoot::fetchSnapshot(Feed* feed, const GCalendarNetworkFactory::Context& context) {
  context.checkCancelled();
  if (feed->isSwitchedOff()) {
    throw ApplicationException(tr("This calendar is disabled."));
  }
  const auto data = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return feedCustomData(db, feed->id(), accountId());
  });
  GCalendarFeed calendar;
  calendar.setCustomDatabaseData(data);
  const auto resource = calendar.calendarResource();
  if (resource.isEmpty() || resource.value(QSL("accessRole")).toString() == QLatin1String("freeBusyReader")) {
    throw CalendarApiException(Feed::Status::AuthError,
                                tr("Calendar details are unavailable or access is limited to free/busy information."));
  }
  const auto zone = resource.value(QSL("timeZone")).toString(QSL("UTC")).toUtf8();
  const auto settings = calendarSettings();
  const auto now = QDateTime::currentDateTimeUtc();
  const auto from = now.addDays(-settings.value(QSL("history_days")).toInt(30));
  const auto until = now.addDays(settings.value(QSL("future_days")).toInt(365));
  const auto calendar_id = feed->customId();
  const auto remote = GCalendarNetworkFactory(context, m_transport).events(calendar_id, from, until);
  const auto cached_data = qApp->database()->worker()->read<QHash<QString, QJsonObject>>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT custom_id, custom_data, is_pdeleted FROM Messages WHERE feed = :feed AND account_id = "
                      ":account;"));
    query.bindValue(QSL(":feed"), feed->id());
    query.bindValue(QSL(":account"), accountId());
    query.exec();
    QHash<QString, QJsonObject> result;
    while (query.next()) {
      context.checkCancelled();
      Message message;
      message.m_customData = query.value(1).toString();
      const auto event = CalendarEvent::fromMessage(message, zone);
      if (!event.isValid()) {
        if (query.value(2).toBool()) {
          continue;
        }
        throw ApplicationException(tr("Invalid cached event; calendar data was preserved."));
      }
      result.insert(query.value(0).toString(), readMetadata(message.m_customData));
    }
    return result;
  });
  QHash<QString, QJsonObject> resources;
  FeedFetchResult result;
  for (const auto& item : remote) {
    context.checkCancelled();
    const auto event = CalendarEvent::fromJson(calendar_id, item, zone);
    if (!event.isValid() || resources.contains(event.identity())) {
      throw CalendarApiException(Feed::Status::ParsingError,
                                  event.isValid() ? tr("Duplicate event occurrence in calendar response.")
                                                  : event.m_error);
    }
    resources.insert(event.identity(), item);
    if (!event.isCancelled()) {
      validateCachedStart(event);
      auto message = event.toMessage();
      if (cached_data.contains(event.identity())) {
        auto envelope = cached_data.value(event.identity());
        auto metadata = envelope.value(QSL("gcalendar")).toObject();
        metadata[QSL("event")] = item;
        metadata[QSL("time_zone")] = QString::fromUtf8(zone);
        envelope[QSL("gcalendar")] = metadata;
        message.m_customData = QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
      }
      result.messages.append(message);
    }
  }
  const int feed_id = feed->id();
  const bool update_counts = qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::UpdateFeedListDuringFetching)).toBool();
  result.afterMessagesStored = [this, feed_id, calendar_id, zone, from, until, resources, context, update_counts]() {
    const auto changes = reconcileSnapshot(feed_id, calendar_id, zone, from, until, resources, context);
    if (update_counts && !changes.isEmpty()) {
      updateCounts();
    }
  };
  return result;
}

QSet<int> GCalendarServiceRoot::reconcileSnapshot(int feed_id,
                                                            const QString& calendar_id,
                                                            const QByteArray& zone,
                                                            const QDateTime& from,
                                                            const QDateTime& until,
                                                            const QHash<QString, QJsonObject>& resources,
                                                            const GCalendarNetworkFactory::Context& context,
                                                            bool retire_missing) {
  QSet<int> changed_ids;
  QSet<QString> retired_events;
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    context.checkCancelled();
    SqlQuery query(db);
    query.prepare(QSL("SELECT id, custom_id, custom_data, is_deleted, is_pdeleted FROM Messages "
                      "WHERE account_id = :account AND feed = :feed;"));
    query.bindValue(QSL(":account"), accountId());
    query.bindValue(QSL(":feed"), feed_id);
    query.exec();
    struct Change {
        int m_id;
        QString m_data;
        bool m_deleted;
    };
    QList<Change> changes;
    QList<int> retired;
    while (query.next()) {
      context.checkCancelled();
      if (!retire_missing && !resources.contains(query.value(1).toString())) {
        continue;
      }
      const bool permanent = query.value(4).toBool();
      Message message;
      message.m_customData = query.value(2).toString();
      const auto cached = CalendarEvent::fromMessage(message, zone);
      if (!cached.isValid() || cached.m_calendarId != calendar_id) {
        if (permanent) {
          continue; // Insufficient metadata is not permission to erase a local tombstone.
        }
        throw ApplicationException(tr("Invalid cached event; destructive reconciliation was cancelled."));
      }
      const int id = query.value(0).toInt();
      const auto incoming = resources.value(cached.identity());
      const bool covered = cached.m_end > from && cached.m_start < until;
      if (permanent) {
        if (retire_missing && cached.m_start.isValid() && cached.m_end.isValid() && !covered && incoming.isEmpty()) {
          retired.append(id);
          retired_events.insert(cached.identity());
        }
        continue;
      }
      if (incoming.isEmpty()) {
        if (!retire_missing) {
          continue;
        }
        const auto pin = QDateTime::fromString(readMetadata(message.m_customData)
                                                 .value(QSL("gcalendar"))
                                                 .toObject()
                                                 .value(QSL("keep_until"))
                                                 .toString(),
                                               Qt::ISODateWithMs);
        if (!covered && pin > QDateTime::currentDateTimeUtc()) {
          continue;
        }
        if (!cached.isCancelled() || !covered) {
          retired.append(id); // Complete window snapshot, or an expired rolling-window row.
          retired_events.insert(cached.identity());
          changed_ids.insert(id);
        }
        continue;
      }
      const auto remote = CalendarEvent::fromJson(calendar_id, incoming, zone);
      auto envelope = readMetadata(message.m_customData);
      auto metadata = envelope.value(QSL("gcalendar")).toObject();
      const bool was_deleted = query.value(3).toBool();
      bool deleted = was_deleted;
      if (remote.isCancelled()) {
        auto merged = cached.m_resource;
        for (auto it = incoming.constBegin(); it != incoming.constEnd(); ++it) {
          merged[it.key()] = it.value();
        }
        if (!metadata.contains(QSL("cancelled_local_deleted"))) {
          metadata[QSL("cancelled_local_deleted")] = deleted;
        }
        metadata[QSL("event")] = merged;
        deleted = true;
      }
      else if (metadata.contains(QSL("cancelled_local_deleted"))) {
        deleted = metadata.value(QSL("cancelled_local_deleted")).toBool() && deleted;
        metadata.remove(QSL("cancelled_local_deleted"));
      }
      else {
        continue;
      }
      envelope[QSL("gcalendar")] = metadata;
      changes.append({id, QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact)), deleted});
      if (deleted != was_deleted) {
        changed_ids.insert(id);
      }
    }
    query.finish();
    context.checkCancelled();
    auto connection = db;
    if (!connection.transaction()) {
      throw SqlException(connection.lastError());
    }
    try {
      for (const auto& change : changes) {
        context.checkCancelled();
        query.prepare(QSL("UPDATE Messages SET custom_data = :data, is_deleted = :deleted WHERE id = :id AND "
                          "is_pdeleted = 0;"));
        query.bindValue(QSL(":data"), change.m_data);
        query.bindValue(QSL(":deleted"), change.m_deleted);
        query.bindValue(QSL(":id"), change.m_id);
        query.exec();
      }
      for (int id : retired) {
        context.checkCancelled();
        DatabaseQueries::purgeMessage(db, id);
      }
      if (!retired_events.isEmpty()) {
        updateAccountCustomData(db, accountId(), [&](QVariantHash& data) {
          auto calendar = metadata(data);
          calendar[QSL("ledger")] = CalendarReminders::retire(calendar.value(QSL("ledger")).toObject(), retired_events);
          data[QSL("gcalendar")] = calendar.toVariantHash();
        });
      }
      context.checkCancelled();
      if (!connection.commit()) {
        throw SqlException(connection.lastError());
      }
    }
    catch (...) {
      connection.rollback();
      throw;
    }
  });
  if (!retired_events.isEmpty()) {
    QMetaObject::invokeMethod(
      this,
      [this, retired_events, lifetime = context.m_cancelled]() {
        if (m_started && !lifetime->load()) {
          if (!m_pendingLedger.isEmpty()) {
            m_pendingLedger = CalendarReminders::retire(m_pendingLedger, retired_events);
          }
          try {
            const auto data = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
              return accountCustomData(db, accountId());
            });
            setCustomDatabaseData(data);
          }
          catch (const ApplicationException& ex) {
            setStatus(ex.message());
          }
        }
      },
      Qt::QueuedConnection);
  }
  return changed_ids;
}

QList<QAction*> GCalendarServiceRoot::contextMenuFeedsList(const QList<RootItem*>& items, QMenu* parent_menu) {
  auto actions = ServiceRoot::contextMenuFeedsList(items, parent_menu);
  const auto* calendar = items.size() == 1 ? dynamic_cast<GCalendarFeed*>(items.first()) : nullptr;

  if (calendar == nullptr || calendar->account() != this) {
    return actions;
  }

  actions.append(calendarAccountActions());
  QObject* action_parent = parent_menu != nullptr ? static_cast<QObject*>(parent_menu) : this;
  auto* create = new QAction(qApp->icons()->fromTheme(QSL("list-add")), tr("New event"), action_parent);
  create->setEnabled(m_started && calendar->canCreateEvent());
  connect(create, &QAction::triggered, this, [this, calendar_id = calendar->customId()]() {
    createEvent(calendar_id);
  });
  actions.append(create);
  return actions;
}

QList<QAction*> GCalendarServiceRoot::serviceMenu() {
  return calendarAccountActions() + addItemMenu();
}

QList<QAction*> GCalendarServiceRoot::calendarAccountActions() {
  if (m_actions.isEmpty()) {
    auto* authorize_action =
      new QAction(qApp->icons()->fromTheme(QSL("dialog-password")), tr("Authorize Google Calendar"), this);
    connect(authorize_action, &QAction::triggered, this, &GCalendarServiceRoot::authorize);
    auto* refresh_action =
      new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Refresh calendar list"), this);
    connect(refresh_action, &QAction::triggered, this, &GCalendarServiceRoot::requestSyncIn);
    m_actions = {authorize_action, refresh_action};
  }
  return m_actions;
}

QList<QAction*> GCalendarServiceRoot::addItemMenu() {
  if (m_createEventAction == nullptr) {
    m_createEventAction = new QAction(qApp->icons()->fromTheme(QSL("list-add")), tr("New event"), this);
    connect(m_createEventAction, &QAction::triggered, this, [this]() {
      createEvent();
    });
  }
  const auto feeds = getSubTreeFeeds();
  const bool writable = std::any_of(feeds.cbegin(), feeds.cend(), [](const Feed* feed) {
    const auto* calendar = dynamic_cast<const GCalendarFeed*>(feed);
    return calendar != nullptr && calendar->canCreateEvent();
  });
  m_createEventAction->setEnabled(m_started && writable);
  return {m_createEventAction};
}

QList<QAction*> GCalendarServiceRoot::contextMenuMessagesList(const QList<Message>& messages, QMenu* parent_menu) {
  if (messages.size() != 1 || messages.first().m_accountId != accountId()) {
    return {};
  }
  const auto message = messages.first();
  auto* details = new QAction(tr("Event details"), parent_menu);
  connect(details, &QAction::triggered, this, [this, message]() {
    openCachedEvent(message.m_id);
  });
  auto* edit = new QAction(tr("Edit event"), parent_menu);
  const auto event = CalendarEvent::fromMessage(message);
  auto* calendar = dynamic_cast<GCalendarFeed*>(getHashedSubTreeFeeds().value(event.m_calendarId));
  edit->setEnabled(calendar != nullptr && !calendar->isSwitchedOff() && !message.m_isDeleted && !message.m_isPdeleted &&
                   event.canEdit(calendar->calendarResource().value(QSL("accessRole")).toString()));
  connect(edit, &QAction::triggered, this, [this, message]() {
    editEvent(message);
  });
  return {details, edit};
}

CustomMessagePreviewer* GCalendarServiceRoot::customMessagePreviewer() {
  if (m_previewer == nullptr) {
    m_previewer = new CalendarEventPreviewer(this);
  }
  return m_previewer;
}

void GCalendarServiceRoot::createEvent(const QString& calendar_id) {
  if (!m_started) {
    return;
  }
  try {
    auto* form = new FormCalendarEvent(this, nullptr, calendar_id, qApp->mainFormWidget());
    form->setAttribute(Qt::WA_DeleteOnClose);
    connect(form, &FormCalendarEvent::eventStored, this, &GCalendarServiceRoot::openCachedEvent);
    form->show();
  }
  catch (const ApplicationException& ex) {
    reportOperationFailure(tr("Cannot create event"), ex.message(), true);
  }
  catch (...) {
    reportOperationFailure(tr("Cannot create event"), tr("An unexpected error occurred."), true);
  }
}

void GCalendarServiceRoot::editEvent(const Message& message) {
  if (!m_started || message.m_accountId != accountId() || message.m_isDeleted || message.m_isPdeleted) {
    return;
  }
  try {
    auto* form = new FormCalendarEvent(this, &message, {}, qApp->mainFormWidget());
    form->setAttribute(Qt::WA_DeleteOnClose);
    form->show();
  }
  catch (const ApplicationException& ex) {
    reportOperationFailure(tr("Cannot edit event"), ex.message(), true);
  }
  catch (...) {
    reportOperationFailure(tr("Cannot edit event"), tr("An unexpected error occurred."), true);
  }
}

QDateTime GCalendarServiceRoot::reminderNow() const {
  return QDateTime::currentDateTimeUtc();
}

void GCalendarServiceRoot::restartReminderTimer(bool check_now) {
  const auto settings = reminderSettings();
  if (m_reminderTimer == nullptr) {
    m_reminderTimer = new QTimer(this);
    m_reminderTimer->setTimerType(Qt::VeryCoarseTimer);
    connect(m_reminderTimer, &QTimer::timeout, this, &GCalendarServiceRoot::checkReminders);
  }
  m_reminderTimer->stop();
  m_reminderTimer->setInterval(settings.m_checkIntervalMinutes * 60000);
  if (m_started && settings.m_enabled) {
    m_reminderTimer->start();
  }
  if ((check_now && settings.m_enabled) || !m_pendingLedger.isEmpty()) {
    QMetaObject::invokeMethod(
      this,
      [this]() {
        checkReminders();
      },
      Qt::QueuedConnection);
  }
}

QList<CalendarReminder> GCalendarServiceRoot::remindersFromDatabase(const QSqlDatabase& db,
                                                                    int account_id,
                                                                    const QDateTime& now,
                                                                    const QJsonObject& pending,
                                                                    const QSet<int>& only_messages) {
  const auto account = metadata(accountCustomData(db, account_id));
  const auto settings = CalendarReminderSettings::fromJson(account.value(QSL("reminders")).toObject());
  auto ledger = CalendarReminders::entries(account.value(QSL("ledger")).toObject());
  const auto unsaved = CalendarReminders::entries(pending);
  for (auto it = unsaved.constBegin(); it != unsaved.constEnd(); ++it) {
    ledger[it.key()] = it.value();
  }
  const QJsonObject effective_ledger{{QSL("version"), 1}, {QSL("entries"), ledger}};
  struct Calendar {
      QString m_title;
      QJsonObject m_resource;
      QJsonObject m_metadata;
  };
  QHash<int, Calendar> calendars;
  SqlQuery query(db);
  query.prepare(QSL("SELECT id, title, custom_data FROM Feeds WHERE account_id = :account AND is_off = 0;"));
  query.bindValue(QSL(":account"), account_id);
  query.exec();
  while (query.next()) {
    GCalendarFeed calendar;
    calendar.setCustomDatabaseData(readMetadata(query.value(2).toString()).toVariantHash());
    const auto resource = calendar.calendarResource();
    const auto role = resource.value(QSL("accessRole")).toString();
    if (role == QLatin1String("reader") || role == QLatin1String("writer") || role == QLatin1String("owner") ||
        role == QLatin1String("writerWithoutPrivateAccess")) {
      calendars.insert(query.value(0).toInt(), {query.value(1).toString(), resource, calendar.calendarMetadata()});
    }
  }
  QList<Message> messages;
  if (only_messages.isEmpty()) {
    messages = DatabaseQueries::getUndeletedMessagesForAccount(db, account_id, {});
  }
  else {
    QStringList ids;
    for (int id : only_messages) {
      if (id > 0) {
        ids.append(QString::number(id));
      }
    }
    if (ids.isEmpty()) {
      return {};
    }
    query.prepare(QSL("SELECT %1 FROM Messages WHERE account_id = :account AND id IN (%2) AND is_deleted = 0 AND "
                      "is_pdeleted = 0;")
                    .arg(DatabaseQueries::messageTableAttributes().join(QSL(", ")), ids.join(QLatin1Char(','))));
    query.bindValue(QSL(":account"), account_id);
    query.exec();
    while (query.next()) {
      messages.append(Message::fromSqlQuery(query, {}));
    }
  }
  QList<CalendarReminder> due;
  for (const auto& message : messages) {
    if (!calendars.contains(message.m_feedId)) {
      continue;
    }
    const auto calendar = calendars.value(message.m_feedId);
    const auto zone = calendar.m_resource.value(QSL("timeZone")).toString(QSL("UTC")).toUtf8();
    if (!CalendarEvent::fromMessage(message, zone).isValid()) {
      qWarningNN << LOGSEC_CORE << "Skipping malformed Calendar event" << message.m_id;
      continue;
    }
    due.append(CalendarReminders::due(message,
                                      CalendarReminderSettings::forCalendar(settings, calendar.m_metadata),
                                      effective_ledger,
                                      now,
                                      calendar.m_title,
                                      zone,
                                      calendar.m_resource.value(QSL("defaultReminders")).toArray()));
  }
  return due;
}

void GCalendarServiceRoot::checkReminders() {
  if (!m_started || m_checkRunning) {
    return;
  }
  m_checkRunning = true;
  const auto now = reminderNow();
  const auto pending = m_pendingLedger;
  const int account_id = accountId();
  auto* database = qApp->database()->worker();
  auto* watcher = new QFutureWatcher<ReminderScan>(this);
  m_reminderScan = watcher;
  connect(watcher, &QFutureWatcher<ReminderScan>::finished, this, [this, watcher]() {
    watcher->deleteLater();
    if (m_reminderScan != watcher) {
      return;
    }
    m_reminderScan = nullptr;
    if (!m_started) {
      m_checkRunning = false;
      emit remindersChecked(false);
      return;
    }
    bool succeeded = false;
    try {
      auto result = watcher->result();
      if (!result.m_error.isEmpty()) {
        throw ApplicationException(result.m_error);
      }
      if (!result.m_due.isEmpty()) {
        QSet<int> candidates;
        for (const auto& reminder : result.m_due) {
          candidates.insert(reminder.m_messageId);
        }
        const auto now = reminderNow();
        const auto pending = m_pendingLedger;
        result.m_due = qApp->database()->worker()->read<QList<CalendarReminder>>([&](const QSqlDatabase& db) {
          return remindersFromDatabase(db, accountId(), now, pending, candidates);
        });
      }
      if (!result.m_due.isEmpty()) {
        const auto triggered = CalendarReminders::markTriggered({}, result.m_due, reminderNow());
        auto pending = CalendarReminders::entries(m_pendingLedger);
        const auto additions = CalendarReminders::entries(triggered);
        for (auto it = additions.constBegin(); it != additions.constEnd(); ++it) {
          pending[it.key()] = it.value();
        }
        m_pendingLedger = {{QSL("version"), 1}, {QSL("entries"), pending}};
        QSet<QString> delivered;
        for (const auto& reminder : result.m_due) {
          if (!delivered.contains(reminder.m_eventId)) {
            delivered.insert(reminder.m_eventId);
            deliverReminder(reminder);
          }
        }
      }
      checkpointReminders();
      succeeded = true;
    }
    catch (const ApplicationException& ex) {
      reportOperationFailure(tr("Reminder check failed"), ex.message(), false);
    }
    catch (...) {
      reportOperationFailure(tr("Reminder check failed"),
                             tr("An unexpected error occurred while checking reminders."),
                             false);
    }
    m_checkRunning = false;
    emit remindersChecked(succeeded);
  });
  watcher->setFuture(QtConcurrent::run([database, account_id, now, pending]() {
    ReminderScan result;
    try {
      result.m_due = database->read<QList<CalendarReminder>>([&](const QSqlDatabase& db) {
        return remindersFromDatabase(db, account_id, now, pending);
      });
    }
    catch (const ApplicationException& ex) {
      result.m_error = ex.message();
    }
    catch (...) {
      result.m_error = tr("An unexpected error occurred while checking reminders.");
    }
    return result;
  }));
}

void GCalendarServiceRoot::checkpointReminders() {
  if (m_pendingLedger.isEmpty()) {
    return;
  }
  const auto pending = CalendarReminders::entries(m_pendingLedger);
  const auto saved = qApp->database()->worker()->write<QVariantHash>([&](const QSqlDatabase& db) {
    return updateAccountCustomData(db, accountId(), [&](QVariantHash& data) {
      auto calendar = metadata(data);
      auto ledger = CalendarReminders::entries(calendar.value(QSL("ledger")).toObject());
      SqlQuery exists(db);
      exists.prepare(QSL("SELECT id FROM Messages WHERE account_id = :account AND custom_id = :event;"));
      for (auto it = pending.constBegin(); it != pending.constEnd(); ++it) {
        exists.bindValue(QSL(":account"), accountId());
        exists.bindValue(QSL(":event"), it.value().toObject().value(QSL("event_id")).toString());
        exists.exec();
        if (exists.next()) {
          ledger[it.key()] = it.value();
        }
        exists.finish();
      }
      calendar[QSL("ledger")] = QJsonObject{{QSL("version"), 1}, {QSL("entries"), ledger}};
      data[QSL("gcalendar")] = calendar.toVariantHash();
    });
  });
  setCustomDatabaseData(saved);
  m_pendingLedger = {};
}

void GCalendarServiceRoot::deliverReminder(const CalendarReminder& reminder) {
  QPointer<GCalendarServiceRoot> account(this);
  const int id = reminder.m_messageId;
  qApp->showGuiMessage(Notification::Event::GeneralEvent,
                       {reminder.m_title, reminder.m_body, QSystemTrayIcon::Information, icon()},
                       {},
                       {tr("Open"), qApp, [account, id]() {
                          if (account != nullptr && account->m_started) {
                            account->openCachedEvent(id);
                          }
                        }});
}

void GCalendarServiceRoot::openCachedEvent(int message_id) {
  try {
    const auto message = cachedMessage(message_id);
    if (message.m_id <= 0) {
      return;
    }
    auto dialog = std::make_unique<QDialog>(qApp->mainFormWidget());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    Ui::FormCalendarEventDetails ui;
    ui.setupUi(dialog.get());
    GuiUtilities::applyDialogProperties(*dialog, icon(), message.m_title);
    auto* preview = new CalendarEventPreviewer(this, dialog.get());
    ui.m_layoutPreview->addWidget(preview);
    preview->loadMessage(message, this, nullptr);
    connect(ui.m_buttonBox, &QDialogButtonBox::rejected, dialog.get(), &QDialog::reject);
    connect(this, &GCalendarServiceRoot::accountStopped, dialog.get(), &QDialog::reject);
    connect(this, &QObject::destroyed, dialog.get(), &QDialog::reject);
    dialog.release()->show();
  }
  catch (const ApplicationException& ex) {
    reportOperationFailure(tr("Cannot open event details"), ex.message(), true);
  }
  catch (...) {
    reportOperationFailure(tr("Cannot open event details"), tr("An unexpected error occurred."), true);
  }
}

Message GCalendarServiceRoot::cachedMessage(int message_id) const {
  return qApp->database()->worker()->read<Message>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT %1, Feeds.title AS calendar_title FROM Messages JOIN Feeds ON Feeds.id = Messages.feed "
                      "WHERE Messages.id = :id AND Messages.account_id = :account AND Messages.is_pdeleted = 0;")
                    .arg(DatabaseQueries::messageTableAttributes().join(QSL(", "))));
    query.bindValue(QSL(":id"), message_id);
    query.bindValue(QSL(":account"), accountId());
    query.exec();
    if (!query.next()) {
      return Message();
    }
    auto message = Message::fromSqlQuery(query, {});
    message.m_feedTitle = query.value(QSL("calendar_title")).toString();
    return message;
  });
}

void GCalendarServiceRoot::storeSnapshot(Feed* feed,
                                         FeedFetchResult snapshot,
                                         EventOperation& result,
                                         bool confirmed_edit) {
  for (auto& message : snapshot.messages) {
    message.m_feedId = feed->id();
    message.m_accountId = accountId();
    message.sanitize(feed, true);
  }
  // A confirmed edit is an explicit source update. Keep the regular update policy for the rest of the snapshot.
  const auto report = confirmed_edit
                        ? DatabaseQueries::updateMessages(qApp->database(),
                                                          qApp->settings(),
                                                          snapshot.messages,
                                                          feed,
                                                          true,
                                                          false)
                        : updateMessages(snapshot.messages, feed, false);
  report.throwIfFailed();
  if (confirmed_edit) {
    feed->removeUnwantedArticles();
  }
  if (snapshot.afterMessagesStored) {
    snapshot.afterMessagesStored();
  }
  for (const auto& message : report.m_all) {
    result.m_changedEventIds.insert(message.m_id);
  }
}

int GCalendarServiceRoot::storeConfirmedEvent(Feed* feed,
                                              const QJsonObject& resource,
                                              const GCalendarNetworkFactory::Context& context,
                                              EventOperation& result) {
  const auto data = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
    return feedCustomData(db, feed->id(), accountId());
  });
  GCalendarFeed calendar;
  calendar.setCustomDatabaseData(data);
  const auto zone = calendar.calendarResource().value(QSL("timeZone")).toString(QSL("UTC")).toUtf8();
  const auto event = CalendarEvent::fromJson(feed->customId(), resource, zone);
  if (!event.isValid() || event.isCancelled() || !resource.value(QSL("recurrence")).toArray().isEmpty()) {
    throw ApplicationException(tr("The saved event cannot be stored as an individual occurrence."));
  }
  validateCachedStart(event);
  auto message = event.toMessage();
  auto envelope = qApp->database()->worker()->read<QJsonObject>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT custom_data FROM Messages WHERE account_id = :account AND custom_id = :id;"));
    query.bindValue(QSL(":account"), accountId());
    query.bindValue(QSL(":id"), event.identity());
    query.exec();
    if (query.next()) {
      Message cached;
      cached.m_customData = query.value(0).toString();
      if (!CalendarEvent::fromMessage(cached, zone).isValid()) {
        throw ApplicationException(tr("Cached event metadata is invalid; the saved remote event was not applied "
                                      "locally."));
      }
      return readMetadata(cached.m_customData);
    }
    return readMetadata(message.m_customData);
  });
  auto metadata = envelope.value(QSL("gcalendar")).toObject();
  metadata[QSL("event")] = resource;
  metadata[QSL("time_zone")] = QString::fromUtf8(zone);
  metadata[QSL("keep_until")] = QDateTime::currentDateTimeUtc().addSecs(1800).toString(Qt::ISODateWithMs);
  envelope[QSL("gcalendar")] = metadata;
  message.m_customData = QString::fromUtf8(QJsonDocument(envelope).toJson(QJsonDocument::Compact));
  context.checkCancelled();
  storeSnapshot(feed,
                {{message},
                 [&]() {
                   result.m_changedEventIds.unite(reconcileSnapshot(feed->id(),
                                            feed->customId(),
                                            zone,
                                            event.m_start,
                                            event.m_end,
                                            {{event.identity(), resource}},
                                            context,
                                            false));
                 }},
                result,
                true);
  return qApp->database()->worker()->read<int>([&](const QSqlDatabase& db) {
    SqlQuery query(db);
    query.prepare(QSL("SELECT id FROM Messages WHERE account_id = :account AND custom_id = :id AND is_pdeleted = 0;"));
    query.bindValue(QSL(":account"), accountId());
    query.bindValue(QSL(":id"), event.identity());
    query.exec();
    return query.next() ? query.value(0).toInt() : 0;
  });
}

quint64 GCalendarServiceRoot::loadRemoteEvent(const QString& calendar_id, const QString& event_id) {
  return startEventOperation(calendar_id, {{QSL("id"), event_id}}, {}, false, true, false, 0);
}

quint64 GCalendarServiceRoot::saveRemoteEvent(const QString& calendar_id,
                                              const QJsonObject& baseline,
                                              const QJsonObject& changes,
                                              bool creating,
                                              bool notify_attendees,
                                              int preferred_message_id) {
  return startEventOperation(calendar_id, baseline, changes, creating, false, notify_attendees, preferred_message_id);
}

quint64 GCalendarServiceRoot::startEventOperation(const QString& calendar_id,
                                                  const QJsonObject& baseline,
                                                  const QJsonObject& changes,
                                                  bool creating,
                                                  bool reading,
                                                  bool notify_attendees,
                                                  int preferred_message_id) {
  if (!m_started) {
    throw ApplicationException(tr("This Calendar account is stopped."));
  }
  if (syncInRunning()) {
    throw ApplicationException(tr("Wait for the calendar list to finish refreshing."));
  }
  QPointer<Feed> feed = getHashedSubTreeFeeds().value(calendar_id);
  if (feed == nullptr) {
    throw ApplicationException(tr("Select an available calendar."));
  }
  const auto context = requestContext();
  const auto configuration = m_configurationGeneration.load();
  const quint64 request_id = ++m_requestId;
  auto* watcher = new QFutureWatcher<EventOperation>(this);
  m_eventOperations.append(watcher);
  connect(watcher, &QFutureWatcher<EventOperation>::finished, this, [this, watcher, request_id, context]() {
    m_eventOperations.removeOne(watcher);
    watcher->deleteLater();
    if (!m_started || context.m_cancelled->load()) {
      return;
    }
    EventOperation result;
    try {
      result = watcher->result();
      if (result.m_confirmed) {
        updateCounts();
        informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::DatabaseCleaned);
      }
    }
    catch (const ApplicationException& ex) {
      if (result.m_error.isEmpty()) {
        result.m_error = ex.message();
      }
    }
    catch (...) {
      if (result.m_error.isEmpty()) {
        result.m_error = tr("An unexpected error occurred while processing the event.");
      }
    }
    if (result.m_confirmed || result.m_status == 200) {
      m_loginFailureReported = false;
    }
    if (result.m_confirmed) {
      if (result.m_messageId > 0) {
        result.m_changedEventIds.insert(result.m_messageId);
      }
      for (int id : std::as_const(result.m_changedEventIds)) {
        emit eventSaved(id);
      }
    }
    if (!result.m_error.isEmpty()) {
      if (result.m_confirmed) {
        result.m_error =
          tr("Saved in Google Calendar, but the local cache could not be refreshed: %1").arg(result.m_error);
      }
      if (result.m_status == 401) {
        reportAuthenticationFailure(result.m_error);
      }
      else {
        reportOperationFailure(tr("Calendar event operation failed"), result.m_error, false);
      }
    }
    emit eventOperationFinished(request_id,
                                result.m_resource,
                                result.m_status,
                                result.m_error,
                                result.m_confirmed,
                                result.m_messageId,
                                result.m_recoveredCreation);
  });
  const auto transport = m_transport;
  watcher->setFuture(QtConcurrent::run([=]() {
    EventOperation result;
    try {
      const std::lock_guard mutation(m_mutationMutex);
      if (context.m_cancelled->load() || configuration != m_configurationGeneration.load() || feed == nullptr) {
        throw ApplicationException(tr("Calendar operation cancelled or account settings changed. The draft was "
                                      "preserved."));
      }
      const auto data = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
        return feedCustomData(db, feed->id(), accountId());
      });
      GCalendarFeed calendar;
      calendar.setCustomDatabaseData(data);
      const auto calendar_resource = calendar.calendarResource();
      const auto zone = calendar_resource.value(QSL("timeZone")).toString(QSL("UTC")).toUtf8();
      const auto role = calendar_resource.value(QSL("accessRole")).toString();
      GCalendarNetworkFactory network(context, transport);
      if (reading) {
        result.m_resource = network.event(calendar_id, baseline.value(QSL("id")).toString());
        if (!CalendarEvent::fromJson(calendar_id, result.m_resource, zone).isValid()) {
          throw ApplicationException(tr("The remote event response is invalid."));
        }
        result.m_status = 200;
        return result;
      }
      if (feed->isSwitchedOff()) {
        throw ApplicationException(tr("This calendar is disabled."));
      }
      const QSet<QString> allowed{QSL("summary"),
                                  QSL("description"),
                                  QSL("location"),
                                  QSL("start"),
                                  QSL("end"),
                                  QSL("recurrence")};
      auto projected = baseline;
      for (auto it = changes.constBegin(); it != changes.constEnd(); ++it) {
        if (!allowed.contains(it.key()) && !(creating && it.key() == QLatin1String("id"))) {
          throw ApplicationException(tr("Unsupported field in event changes."));
        }
        projected[it.key()] = it.value();
      }
      const auto candidate = CalendarEvent::fromJson(calendar_id, projected, zone);
      if (!candidate.canEdit(role)) {
        throw ApplicationException(candidate.isValid() ? tr("This event or calendar is read-only.")
                                                       : candidate.m_error);
      }
      if (projected.value(QSL("recurrence")).toArray().isEmpty()) {
        validateCachedStart(candidate);
      }
      result.m_resource = creating
                            ? network.insertEvent(calendar_id, changes, notify_attendees, &result.m_recoveredCreation)
                            : network.updateEvent(calendar_id,
                                                  baseline.value(QSL("id")).toString(),
                                                  changes,
                                                  baseline.value(QSL("etag")).toString(),
                                                  notify_attendees);
      result.m_confirmed = true;
      result.m_status = 200;
      const auto saved = CalendarEvent::fromJson(calendar_id, result.m_resource, zone);
      const auto expected_id = creating ? changes.value(QSL("id")) : baseline.value(QSL("id"));
      if (!saved.isValid() || result.m_resource.value(QSL("id")) != expected_id) {
        throw ApplicationException(tr("Google accepted the write but returned an invalid event resource. Refresh this "
                                      "calendar."));
      }
      if (context.m_cancelled->load()) {
        return result;
      }
      const bool series = !baseline.value(QSL("recurrence")).toArray().isEmpty() ||
                          !result.m_resource.value(QSL("recurrence")).toArray().isEmpty() ||
                          changes.contains(QSL("recurrence"));
      if (result.m_resource.value(QSL("recurrence")).toArray().isEmpty()) {
        result.m_messageId = storeConfirmedEvent(feed, result.m_resource, context, result);
      }
      if (series) {
        storeSnapshot(feed, fetchSnapshot(feed, context), result);
        const auto settings = calendarSettings();
        const auto now = QDateTime::currentDateTimeUtc();
        const auto from = now.addDays(-settings.value(QSL("history_days")).toInt(30));
        const auto until = now.addDays(settings.value(QSL("future_days")).toInt(365));
        if (!result.m_resource.value(QSL("recurrence")).toArray().isEmpty() && saved.m_start.toMSecsSinceEpoch() >= 0 &&
            (saved.m_end <= from || saved.m_start >= until)) {
          const auto instances = network.instances(calendar_id,
                                                   saved.m_resource.value(QSL("id")).toString(),
                                                   saved.m_start.addSecs(-1),
                                                   saved.m_end.addSecs(1));
          for (const auto& instance : instances) {
            const auto occurrence = CalendarEvent::fromJson(calendar_id, instance, zone);
            if (!occurrence.isValid() || instance.value(QSL("recurringEventId")) != saved.m_resource.value(QSL("id"))) {
              throw ApplicationException(tr("Invalid recurring-event occurrence returned after saving."));
            }
          }
          for (const auto& instance : instances) {
            if (!CalendarEvent::fromJson(calendar_id, instance, zone).isCancelled()) {
              const int id = storeConfirmedEvent(feed, instance, context, result);
              if (result.m_messageId <= 0) {
                result.m_messageId = id;
              }
            }
          }
        }
        const auto cached = qApp->database()->worker()->read<QList<Message>>([&](const QSqlDatabase& db) {
          return DatabaseQueries::getUndeletedMessagesForFeed(db, feed->id(), {});
        });
        for (const auto& message : cached) {
          const auto event = CalendarEvent::fromMessage(message, zone);
          if (event.m_resource.value(QSL("recurringEventId")) == result.m_resource.value(QSL("id")) ||
              event.m_resource.value(QSL("id")) == result.m_resource.value(QSL("id"))) {
            if (result.m_messageId <= 0 || message.m_id == preferred_message_id) {
              result.m_messageId = message.m_id;
            }
          }
        }
      }
    }
    catch (const CalendarApiException& ex) {
      result.m_status = ex.httpStatus();
      result.m_error = ex.message();
    }
    catch (const ApplicationException& ex) {
      result.m_error = ex.message();
    }
    catch (...) {
      result.m_error = tr("An unexpected error occurred while processing the event.");
    }
    return result;
  }));
  return request_id;
}

QList<QVariantHash> GCalendarServiceRoot::calendarReminderData(const QList<Feed*>& feeds,
                                                              const QJsonObject& reminder_override) const {
  const auto settings = calendarSettings();
  const auto reminders = CalendarReminderSettings::fromJson(settings.value(QSL("reminders")).toObject());
  const auto now = reminderNow();
  return qApp->database()->worker()->read<QList<QVariantHash>>([&](const QSqlDatabase& db) {
    QList<QVariantHash> validated;
    for (auto* feed : feeds) {
      validated.append(calendarDataWithOverride(DatabaseQueries::loadFeedCustomData(db, feed),
                                               reminder_override,
                                               reminders,
                                               settings.value(QSL("future_days")).toInt(365),
                                               now));
    }
    return validated;
  });
}
