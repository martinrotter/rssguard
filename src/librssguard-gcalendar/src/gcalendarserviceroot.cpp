// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendarserviceroot.h"

#include "gcalendarfeed.h"
#include "gui/calendareventpreviewer.h"
#include "gui/formcalendarevent.h"
#include "gui/formeditgcalendaraccount.h"
#include "gui/formgcalendarfeeddetails.h"

#include "ui_formcalendareventdetails.h"

#include <algorithm>
#include <librssguard/database/databasequeries.h>
#include <librssguard/exceptions/sqlexception.h>
#include <librssguard/gui/guiutilities.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/feedreader.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/mutex.h>
#include <librssguard/network-web/oauth2service.h>
#include <librssguard/services/abstract/category.h>
#include <librssguard/services/abstract/label.h>
#include <librssguard/services/abstract/search.h>

#include <QAction>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QMutexLocker>
#include <QScopeGuard>
#include <QScopedPointer>
#include <QSet>
#include <QSharedPointer>
#include <QSpinBox>
#include <QTextBrowser>
#include <QThread>
#include <QTimer>
#include <QVBoxLayout>
#include <QtConcurrent>

namespace {
  void validateCachedStart(const CalendarEvent& event) {
    if (event.m_start.isValid() && event.m_start.toMSecsSinceEpoch() < 0) {
      throw ApplicationException(GCalendarServiceRoot::tr("Events starting before 1970 cannot be cached by RSS Guard. "
                                                          "Manage these events in Google Calendar."));
    }
  }

  void moveTreeToGui(RootItem* item) {
    for (auto* node : item->getSubTree<RootItem>()) {
      node->moveToThread(qApp->thread());
    }
  }

  QJsonObject readMetadata(const QString& text) {
    const auto document = QJsonDocument::fromJson(text.toUtf8());
    if (!document.isObject()) {
      throw ApplicationException(QObject::tr("Invalid calendar custom data; existing data was preserved."));
    }
    return document.object();
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
}

GCalendarServiceRoot::~GCalendarServiceRoot() {
  stop();
  delete m_previewer;
}

QString GCalendarServiceRoot::code() const {
  return QSL(SERVICE_CODE_GCALENDAR);
}

bool GCalendarServiceRoot::isSyncable() const {
  return true;
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

bool GCalendarServiceRoot::canBeEdited() const {
  return true;
}

bool GCalendarServiceRoot::canBeDeleted() const {
  return ServiceRoot::canBeDeleted() && m_eventOperations.isEmpty() && m_discovery == nullptr;
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
      setStatus(ex.message());
      qApp->showGuiMessage(Notification::Event::GeneralEvent,
                           {tr("Cannot edit calendar"), ex.message(), QSystemTrayIcon::Critical});
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

MessageStoragePolicy GCalendarServiceRoot::messageStoragePolicy() const {
  return {true, true, false};
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
      auto data = accountId() > 0 ? DatabaseQueries::loadAccountCustomData(db, accountId()) : desired;
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
        DatabaseQueries::createOverwriteAccount(db, this, data);
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
  std::unique_lock<std::mutex> mutation(m_mutationMutex, std::try_to_lock);
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
  if ((changed_credentials || !refresh_token.isEmpty()) && m_oauth != nullptr) {
    m_oauth->logout();
    delete m_oauth;
    QMutexLocker lock(&m_stateMutex);
    m_context.m_bearer.clear();
  }
  if (m_started) {
    ensureOAuth();
    ++m_configurationGeneration;
    restartReminderTimer(reminders.m_enabled && !previous.m_enabled);
    if (!refresh_token.isEmpty()) {
      m_discoverAfterAuth = true;
      requestAccessToken();
    }
  }
  itemChanged({this});
}

QList<ArticleColumn> GCalendarServiceRoot::articleColumns() const {
  return {{QSL("starts"), tr("Starts"), MSG_MDL_DCREATED_INDEX},
          {QSL("calendar"), tr("Calendar"), MSG_MDL_FEED_TITLE_INDEX},
          {QSL("ends"), tr("Ends")},
          {QSL("location"), tr("Location")},
          {QSL("all_day"), tr("All day")}};
}

QVariant GCalendarServiceRoot::articleColumnData(const Message& message, const QString& column_id, int role) const {
  if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::ToolTipRole) {
    return {};
  }
  if (column_id == QLatin1String("calendar")) {
    return message.m_feedTitle;
  }
  const auto event = CalendarEvent::fromMessage(message);
  if (!event.isValid()) {
    return {};
  }
  if (column_id == QLatin1String("starts")) {
    return event.displayStart();
  }
  if (column_id == QLatin1String("ends")) {
    return role == Qt::EditRole ? QVariant(event.m_end) : QVariant(event.displayEnd());
  }
  if (column_id == QLatin1String("location")) {
    return event.m_resource.value(QSL("location")).toString();
  }
  if (column_id == QLatin1String("all_day")) {
    return event.isAllDay() ? tr("Yes") : tr("No");
  }
  return {};
}

void GCalendarServiceRoot::setStatus(const QString& status) {
  m_status = status;
  emit statusChanged(status);
  itemChanged({this});
}

QString GCalendarServiceRoot::additionalTooltip() const {
  return m_status.toHtmlEscaped();
}

void GCalendarServiceRoot::ensureOAuth() {
  if (m_oauth != nullptr) {
    m_oauth->setNetworkProxy(networkProxy());
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
        return DatabaseQueries::loadAccountCustomData(db, accountId());
      });
      setCustomDatabaseData(data);
      setStatus(tr("Authorized"));
      if (m_discoverAfterAuth) {
        m_discoverAfterAuth = false;
        requestSyncIn();
      }
    }
    catch (const ApplicationException& ex) {
      setStatus(ex.message());
    }
  });
  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, [this](const QString&, const QString& description) {
    authenticationFinished(description);
    setStatus(tr("Authorization failed: %1").arg(description));
  });
  connect(m_oauth, &OAuth2Service::authFailed, this, [this]() {
    authenticationFinished(tr("Authorization was declined."));
    setStatus(tr("Authorization was declined."));
  });
  connect(this, &ServiceRoot::proxyChanged, m_oauth, &OAuth2Service::setNetworkProxy);
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
    setStatus(error);
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

QString GCalendarServiceRoot::authorizedBearer(const QString& rejected,
                                               const std::shared_ptr<std::atomic_bool>& cancelled,
                                               int timeout) {
  QMutexLocker lock(&m_stateMutex);
  const auto valid = [this]() {
    return !m_context.m_bearer.isEmpty() && m_bearerExpires > QDateTime::currentDateTimeUtc();
  };
  if (cancelled->load()) {
    throw CalendarApiException(0, tr("Calendar operation cancelled."));
  }
  if (!m_authorizationPending && valid() && (rejected.isEmpty() || rejected != m_context.m_bearer)) {
    return m_context.m_bearer;
  }
  // HTTP jobs may wait for OAuth, but must never block its owning GUI thread.
  if (QThread::currentThread() == thread()) {
    throw CalendarApiException(401, tr("Authorize the Calendar account before refreshing events."));
  }
  const auto generation = m_authorizationGeneration;
  if (!m_authorizationPending) {
    m_authorizationPending = true;
    QMetaObject::invokeMethod(
      this,
      [this, cancelled]() {
        if (!cancelled->load()) {
          requestAccessToken();
        }
      },
      Qt::QueuedConnection);
  }
  QElapsedTimer elapsed;
  elapsed.start();
  while (m_authorizationPending && generation == m_authorizationGeneration && !cancelled->load()) {
    const auto remaining = qint64(timeout) - elapsed.elapsed();
    if (remaining <= 0) {
      throw CalendarApiException(0, tr("Timed out waiting for Calendar authorization."));
    }
    m_authorizationReady.wait(&m_stateMutex, static_cast<unsigned long>(std::min(qint64(100), remaining)));
  }
  if (cancelled->load()) {
    throw CalendarApiException(0, tr("Calendar operation cancelled."));
  }
  if (!valid()) {
    throw CalendarApiException(401,
                               m_authorizationError.isEmpty() ? tr("Calendar authorization failed.")
                                                              : m_authorizationError);
  }
  return m_context.m_bearer;
}

void GCalendarServiceRoot::loadCachedTree() {
  RootItem::Assignment categories, feeds;
  QList<Label*> labels;
  QList<Search*> probes;
  const auto filters = qApp->feedReader()->messageFilters();
  qApp->database()->worker()->read([&](const QSqlDatabase& db) {
    categories = DatabaseQueries::getCategories<Category>(db, accountId());
    feeds = DatabaseQueries::getFeeds<GCalendarFeed>(db, filters, accountId());
    labels = DatabaseQueries::getLabelsForAccount(qApp->icons(), db, accountId());
    probes = DatabaseQueries::getProbesForAccount(db, accountId());
    for (const auto& item : categories + feeds) {
      moveTreeToGui(item.second);
    }
    for (auto* label : labels) {
      moveTreeToGui(label);
    }
    for (auto* probe : probes) {
      moveTreeToGui(probe);
    }
  });
  performInitialAssembly(categories, feeds, labels, probes);
  m_loaded = true;
}

void GCalendarServiceRoot::start(bool freshly_activated) {
  if (m_started) {
    return;
  }
  {
    QMutexLocker lock(&m_stateMutex);
    m_context = GCalendarNetworkFactory::networkContext(networkProxy());
  }
  if (!m_loaded) {
    loadCachedTree();
    connect(qApp->feedUpdateLock(), &Mutex::unlocked, this, &GCalendarServiceRoot::finishCalendarDiscovery);
    connect(this, &GCalendarServiceRoot::eventOperationFinished, this, &GCalendarServiceRoot::finishCalendarDiscovery);
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
  {
    QMutexLocker lock(&m_stateMutex);
    m_context.m_cancelled->store(true);
    m_context.m_bearer.clear();
    m_authorizationPending = false;
    ++m_authorizationGeneration;
    m_authorizationReady.wakeAll();
  }
  m_started = false;
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
  if (m_discovery != nullptr) {
    m_discovery->waitForFinished();
    m_discovery->deleteLater();
    m_discovery = nullptr;
  }
  // Native feed fetching retains this lock through its after-storage callback.
  const std::lock_guard<std::mutex> lock(m_mutationMutex);
  if (m_oauth != nullptr) {
    m_oauth->logout();
    delete m_oauth;
  }
  emit accountStopped();
}

GCalendarNetworkFactory::Context GCalendarServiceRoot::requestContext() {
  QMutexLocker lock(&m_stateMutex);
  if (m_context.m_cancelled->load() || !m_started) {
    throw ApplicationException(tr("Authorize the Calendar account before refreshing events."));
  }
  auto context = m_context;
  context.m_authorize = [this, cancelled = context.m_cancelled, timeout = context.m_timeout](const QString& rejected) {
    return authorizedBearer(rejected, cancelled, timeout);
  };
  return context;
}

void GCalendarServiceRoot::authorize() {
  if (!m_started) {
    return;
  }
  ensureOAuth();
  m_discoverAfterAuth = true;
  m_oauth->setRedirectUrl(QSL("http://127.0.0.1:0"), true);
  {
    QMutexLocker lock(&m_stateMutex);
    m_authorizationPending = true;
  }
  if (m_oauth->login()) {
    authenticationFinished({});
    m_discoverAfterAuth = false;
    requestSyncIn();
  }
}

void GCalendarServiceRoot::requestSyncIn() {
  if (!m_started || m_discovery != nullptr) {
    return;
  }
  GCalendarNetworkFactory::Context context;
  try {
    context = requestContext();
  }
  catch (const ApplicationException& ex) {
    setStatus(ex.message());
    emit calendarDiscoveryFinished(false);
    return;
  }
  auto* watcher = new QFutureWatcher<Discovery>(this);
  m_discovery = watcher;
  connect(watcher, &QFutureWatcher<Discovery>::finished, this, &GCalendarServiceRoot::finishCalendarDiscovery);
  setStatus(tr("Refreshing calendars..."));
  const auto transport = m_transport;
  watcher->setFuture(QtConcurrent::run([context, transport]() {
    Discovery result;
    try {
      result.m_calendars = GCalendarNetworkFactory(context, transport).calendars();
    }
    catch (const ApplicationException& ex) {
      result.m_error = ex.message();
    }
    return result;
  }));
}

void GCalendarServiceRoot::finishCalendarDiscovery() {
  if (m_discovery == nullptr || !m_discovery->isFinished() || !m_started) {
    return;
  }
  const auto result = m_discovery->result();
  bool succeeded = false;
  try {
    if (!result.m_error.isEmpty()) {
      throw ApplicationException(result.m_error);
    }
    if (qApp->feedUpdateLock()->isLocked() || !applyCalendars(result.m_calendars)) {
      setStatus(tr("Waiting for the current calendar update before applying the calendar list."));
      return;
    }
    setStatus(tr("Calendars refreshed."));
    succeeded = true;
  }
  catch (const ApplicationException& ex) {
    setStatus(ex.message());
  }
  m_discovery->deleteLater();
  m_discovery = nullptr;
  emit calendarDiscoveryFinished(succeeded);
}

bool GCalendarServiceRoot::applyCalendars(const QList<QJsonObject>& calendars) {
  std::unique_lock<std::mutex> lock(m_mutationMutex, std::try_to_lock);
  if (!lock.owns_lock()) {
    return false;
  }
  QSet<QString> seen;
  QList<QJsonObject> resources;
  QHash<QString, QByteArray> calendar_icons;
  for (const auto& resource : calendars) {
    const auto id = resource.value(QSL("id")).toString();
    if (id.isEmpty() || seen.contains(id)) {
      throw ApplicationException(tr("Invalid or duplicate calendar ID; cached calendars were preserved."));
    }
    seen.insert(id);
    if (!resource.value(QSL("deleted")).toBool()) {
      GCalendarFeed validation;
      validation.setCalendarResource(resource);
      // Render pixmaps on the GUI thread; pass only serialized icons to the database worker.
      const QColor color(resource.value(QSL("backgroundColor")).toString());
      calendar_icons.insert(id, IconFactory::toByteArray(color.isValid() ? IconFactory::fromColor(color) : icon()));
      resources.append(resource);
    }
  }
  const auto existing = getHashedSubTreeFeeds();
  QList<GCalendarFeed*> added;
  const auto cleanup_added = qScopeGuard([&]() {
    qDeleteAll(added);
  });
  QHash<GCalendarFeed*, QVariantHash> updated;
  QList<Feed*> removed;
  QSet<QString> active;
  // Validate all stored namespaces before making any changes.
  for (const auto& resource : resources) {
    const auto id = resource.value(QSL("id")).toString();
    active.insert(id);
    auto* feed = dynamic_cast<GCalendarFeed*>(existing.value(id));
    if (feed != nullptr) {
      const auto data = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
        return feedCustomData(db, feed->id(), accountId());
      });
      GCalendarFeed validation;
      validation.setCustomDatabaseData(data);
      validation.setCalendarResource(resource);
      updated.insert(feed, validation.customDatabaseData());
    }
    else {
      auto* created = new GCalendarFeed();
      created->setCalendarResource(resource);
      created->setIcon(IconFactory::fromByteArray(calendar_icons.value(id)));
      created->setIsQuiet(true);
      created->setExcludesFromGlobalUnreadCount(true);
      auto data = created->customDatabaseData();
      auto metadata = created->calendarMetadata();
      metadata[QSL("reminder_override")] = QJsonObject{{QSL("version"), 1},
                                                       {QSL("mode"), QSL("inherit")},
                                                       {QSL("enabled_at"), reminderNow().toString(Qt::ISODateWithMs)}};
      data[QSL("gcalendar")] = metadata.toVariantHash();
      created->setCustomDatabaseData(data);
      added.append(created);
    }
  }
  for (auto* feed : existing) {
    if (!active.contains(feed->customId())) {
      removed.append(feed);
    }
  }
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    auto connection = db;
    if (!connection.transaction()) {
      throw SqlException(connection.lastError());
    }
    try {
      for (auto it = updated.begin(); it != updated.end(); ++it) {
        const auto resource =
          QJsonObject::fromVariantHash(it.value()).value(QSL("gcalendar")).toObject().value(QSL("resource")).toObject();
        // Hot-load once more on the serialized writer so a recent override survives.
        GCalendarFeed merged;
        merged.setCustomDatabaseData(feedCustomData(db, it.key()->id(), accountId()));
        merged.setCalendarResource(resource);
        it.value() = merged.customDatabaseData();
        SqlQuery query(db);
        query.prepare(QSL("UPDATE Feeds SET title = :title, icon = :icon, custom_data = :data WHERE id = :id AND "
                          "account_id = :account;"));
        query.bindValue(QSL(":title"), merged.title());
        query.bindValue(QSL(":icon"), calendar_icons.value(merged.customId()));
        query.bindValue(QSL(":data"), DatabaseQueries::serializeCustomData(merged.customDatabaseData()));
        query.bindValue(QSL(":id"), it.key()->id());
        query.bindValue(QSL(":account"), accountId());
        query.exec();
      }
      for (auto* feed : added) {
        DatabaseQueries::createOverwriteFeed(db, feed, accountId(), NO_PARENT_CATEGORY);
      }
      for (auto* feed : removed) {
        SqlQuery query(db);
        query.prepare(QSL("DELETE FROM Feeds WHERE id = :id AND account_id = :account;"));
        query.bindValue(QSL(":id"), feed->id());
        query.bindValue(QSL(":account"), accountId());
        query.exec();
      }
      if (!connection.commit()) {
        throw SqlException(connection.lastError());
      }
    }
    catch (...) {
      connection.rollback();
      throw;
    }
  });
  for (auto it = updated.constBegin(); it != updated.constEnd(); ++it) {
    it.key()->setCustomDatabaseData(it.value());
    it.key()->setCalendarResource(it.key()->calendarResource());
    it.key()->setIcon(IconFactory::fromByteArray(calendar_icons.value(it.key()->customId())));
    itemChanged({it.key()});
  }
  for (auto* feed : added) {
    requestItemReassignment(feed, this);
  }
  added.clear();
  for (auto* feed : removed) {
    requestItemRemoval(feed, false);
  }
  updateCounts();
  informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::AccountSyncedIn);
  return true;
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
  const auto context = requestContext();
  auto result = fetchSnapshot(feed, context);
  const auto after_stored = result.afterMessagesStored;
  result.afterMessagesStored = [guard, after_stored]() {
    after_stored();
  };
  return result;
}

FeedFetchResult GCalendarServiceRoot::fetchSnapshot(Feed* feed, const GCalendarNetworkFactory::Context& context) {
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
    throw ApplicationException(tr("Calendar details are unavailable or access is limited to free/busy information."));
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
    const auto event = CalendarEvent::fromJson(calendar_id, item, zone);
    if (!event.isValid() || resources.contains(event.identity())) {
      throw ApplicationException(event.isValid() ? tr("Duplicate event occurrence in calendar response.")
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
  result.afterMessagesStored = [this, feed_id, calendar_id, zone, from, until, resources, context]() {
    if (!context.m_cancelled->load()) {
      reconcileSnapshot(feed_id, calendar_id, zone, from, until, resources, context.m_cancelled);
    }
  };
  return result;
}

void GCalendarServiceRoot::reconcileSnapshot(int feed_id,
                                             const QString& calendar_id,
                                             const QByteArray& zone,
                                             const QDateTime& from,
                                             const QDateTime& until,
                                             const QHash<QString, QJsonObject>& resources,
                                             const std::shared_ptr<std::atomic_bool>& cancelled,
                                             bool retire_missing) {
  QSet<QString> retired_events;
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    if (cancelled->load()) {
      return;
    }
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
        if (!cached.isCancelled()) {
          retired.append(id); // Complete window snapshot, or an expired rolling-window row.
          retired_events.insert(cached.identity());
        }
        else if (!covered) {
          retired.append(id);
          retired_events.insert(cached.identity());
        }
        continue;
      }
      const auto remote = CalendarEvent::fromJson(calendar_id, incoming, zone);
      auto envelope = readMetadata(message.m_customData);
      auto metadata = envelope.value(QSL("gcalendar")).toObject();
      bool deleted = query.value(3).toBool();
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
    }
    query.finish();
    auto connection = db;
    if (!connection.transaction()) {
      throw SqlException(connection.lastError());
    }
    try {
      for (const auto& change : changes) {
        query.prepare(QSL("UPDATE Messages SET custom_data = :data, is_deleted = :deleted WHERE id = :id AND "
                          "is_pdeleted = 0;"));
        query.bindValue(QSL(":data"), change.m_data);
        query.bindValue(QSL(":deleted"), change.m_deleted);
        query.bindValue(QSL(":id"), change.m_id);
        query.exec();
      }
      for (int id : retired) {
        DatabaseQueries::purgeMessage(db, id);
      }
      if (!retired_events.isEmpty()) {
        DatabaseQueries::modifyAccountCustomData(db, accountId(), [&](QVariantHash& data) {
          auto calendar = metadata(data);
          calendar[QSL("ledger")] = CalendarReminders::retire(calendar.value(QSL("ledger")).toObject(), retired_events);
          data[QSL("gcalendar")] = calendar.toVariantHash();
        });
      }
      if (cancelled->load() || !connection.commit()) {
        throw ApplicationException(tr("Calendar reconciliation was cancelled or could not be saved."));
      }
    }
    catch (...) {
      connection.rollback();
      throw;
    }
  });
  QMetaObject::invokeMethod(
    this,
    [this, retired_events]() {
      if (m_started) {
        if (!m_pendingLedger.isEmpty() && !retired_events.isEmpty()) {
          m_pendingLedger = CalendarReminders::retire(m_pendingLedger, retired_events);
        }
        if (!retired_events.isEmpty()) {
          try {
            const auto data = qApp->database()->worker()->read<QVariantHash>([&](const QSqlDatabase& db) {
              return DatabaseQueries::loadAccountCustomData(db, accountId());
            });
            setCustomDatabaseData(data);
          }
          catch (const ApplicationException& ex) {
            setStatus(ex.message());
          }
        }
        updateCounts();
        informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::FeedFetchFinished);
      }
    },
    Qt::QueuedConnection);
}

QList<QAction*> GCalendarServiceRoot::contextMenuFeedsList(const QList<RootItem*>& items, QMenu* parent_menu) {
  auto* authorize_action =
    new QAction(qApp->icons()->fromTheme(QSL("dialog-password")), tr("Authorize Google Calendar"), parent_menu);
  connect(authorize_action, &QAction::triggered, this, &GCalendarServiceRoot::authorize);
  auto* refresh_action =
    new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Refresh calendar list"), parent_menu);
  connect(refresh_action, &QAction::triggered, this, &GCalendarServiceRoot::requestSyncIn);
  QList<QAction*> actions{authorize_action, refresh_action};
  auto* create = new QAction(qApp->icons()->fromTheme(QSL("list-add")), tr("New event"), parent_menu);
  const auto calendar_id =
    items.size() == 1 && items.first()->kind() == RootItem::Kind::Feed ? items.first()->customId() : QString();
  create->setEnabled(addItemMenu().first()->isEnabled());
  connect(create, &QAction::triggered, this, [this, calendar_id]() {
    createEvent(calendar_id);
  });
  actions.append(create);
  return actions;
}

QList<QAction*> GCalendarServiceRoot::serviceMenu() {
  if (m_actions.isEmpty()) {
    auto* authorize_action =
      new QAction(qApp->icons()->fromTheme(QSL("dialog-password")), tr("Authorize Google Calendar"), this);
    connect(authorize_action, &QAction::triggered, this, &GCalendarServiceRoot::authorize);
    auto* refresh_action =
      new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("Refresh calendar list"), this);
    connect(refresh_action, &QAction::triggered, this, &GCalendarServiceRoot::requestSyncIn);
    m_actions = {authorize_action, refresh_action};
  }
  auto actions = m_actions;
  actions.append(addItemMenu());
  return actions;
}

QList<QAction*> GCalendarServiceRoot::addItemMenu() {
  if (m_createEventAction == nullptr) {
    m_createEventAction = new QAction(qApp->icons()->fromTheme(QSL("list-add")), tr("New event"), this);
    connect(m_createEventAction, &QAction::triggered, this, [this]() {
      createEvent();
    });
  }
  bool writable = false;
  for (auto* feed : getSubTreeFeeds()) {
    auto* calendar = dynamic_cast<GCalendarFeed*>(feed);
    if (calendar == nullptr || calendar->isSwitchedOff()) {
      continue;
    }
    const auto role = calendar->calendarResource().value(QSL("accessRole")).toString();
    writable = writable || role == QLatin1String("owner") || role == QLatin1String("writer") ||
               role == QLatin1String("writerWithoutPrivateAccess");
  }
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
    setStatus(ex.message());
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
    setStatus(ex.message());
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
  const auto account = metadata(DatabaseQueries::loadAccountCustomData(db, account_id));
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
    auto result = watcher->result();
    bool succeeded = false;
    try {
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
      setStatus(tr("Reminder check failed: %1").arg(ex.message()));
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
    return result;
  }));
}

void GCalendarServiceRoot::checkpointReminders() {
  if (m_pendingLedger.isEmpty()) {
    return;
  }
  const auto pending = CalendarReminders::entries(m_pendingLedger);
  const auto saved = qApp->database()->worker()->write<QVariantHash>([&](const QSqlDatabase& db) {
    return DatabaseQueries::modifyAccountCustomData(db, accountId(), [&](QVariantHash& data) {
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
    auto* dialog = new QDialog(qApp->mainFormWidget());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    Ui::FormCalendarEventDetails ui;
    ui.setupUi(dialog);
    GuiUtilities::applyDialogProperties(*dialog, icon(), message.m_title);
    auto* preview = new CalendarEventPreviewer(this, dialog);
    ui.m_layoutPreview->addWidget(preview);
    preview->loadMessage(message, this, nullptr);
    connect(ui.m_buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);
    connect(this, &GCalendarServiceRoot::accountStopped, dialog, &QDialog::reject);
    connect(this, &QObject::destroyed, dialog, &QDialog::reject);
    dialog->show();
  }
  catch (const ApplicationException& ex) {
    setStatus(ex.message());
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

void GCalendarServiceRoot::storeSnapshot(Feed* feed, FeedFetchResult snapshot) {
  for (auto& message : snapshot.messages) {
    message.m_feedId = feed->id();
    message.m_accountId = accountId();
    message.sanitize(feed, true);
  }
  updateMessages(snapshot.messages, feed, false).throwIfFailed();
  if (snapshot.afterMessagesStored) {
    snapshot.afterMessagesStored();
  }
}

int GCalendarServiceRoot::storeConfirmedEvent(Feed* feed,
                                              const QJsonObject& resource,
                                              const GCalendarNetworkFactory::Context& context) {
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
  message.m_feedId = feed->id();
  message.m_accountId = accountId();
  message.sanitize(feed, true);
  if (context.m_cancelled->load()) {
    throw ApplicationException(tr("Calendar operation cancelled."));
  }
  QList<Message> messages{message};
  updateMessages(messages, feed, false).throwIfFailed();
  reconcileSnapshot(feed->id(),
                    feed->customId(),
                    zone,
                    event.m_start,
                    event.m_end,
                    {{event.identity(), resource}},
                    context.m_cancelled,
                    false);
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
  QPointer<Feed> feed = getHashedSubTreeFeeds().value(calendar_id);
  if (feed == nullptr) {
    throw ApplicationException(tr("Select an available calendar."));
  }
  auto context = requestContext();
  {
    QMutexLocker lock(&m_stateMutex);
    context.m_cancelled = m_context.m_cancelled;
  }
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
    const auto result = watcher->result();
    if (result.m_confirmed) {
      updateCounts();
      informOthersAboutDataChange(this, FeedsModel::ExternalDataChange::FeedFetchFinished);
      emit eventSaved(result.m_messageId);
    }
    if (!result.m_error.isEmpty()) {
      setStatus(result.m_error);
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
    const std::lock_guard<std::mutex> mutation(m_mutationMutex);
    try {
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
        result.m_messageId = storeConfirmedEvent(feed, result.m_resource, context);
      }
      if (series) {
        storeSnapshot(feed, fetchSnapshot(feed, context));
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
              const int id = storeConfirmedEvent(feed, instance, context);
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
      result.m_status = ex.m_status;
      result.m_error = ex.message();
    }
    catch (const ApplicationException& ex) {
      result.m_error = ex.message();
    }
    if (result.m_confirmed && !result.m_error.isEmpty()) {
      result.m_error =
        tr("Saved in Google Calendar, but the local cache could not be refreshed: %1").arg(result.m_error);
    }
    return result;
  }));
  return request_id;
}

void GCalendarServiceRoot::setCalendarReminderOverride(GCalendarFeed* feed, QJsonObject settings) {
  saveCalendarFeedSettings({feed}, settings);
}

void GCalendarServiceRoot::saveCalendarFeedSettings(const QList<Feed*>& feeds,
                                                    const QJsonObject& reminder_override,
                                                    const std::function<void(Feed*)>& apply_common) {
  std::unique_lock<std::mutex> mutation(m_mutationMutex, std::try_to_lock);
  if (!mutation.owns_lock()) {
    throw ApplicationException(tr("Wait for the current calendar update before changing reminder settings."));
  }
  if (feeds.isEmpty()) {
    throw ApplicationException(tr("Select at least one calendar."));
  }
  const auto existing = getPrimaryIdHashedSubTreeFeeds();
  QSet<int> seen;
  QList<QSharedPointer<Feed>> prepared;
  for (auto* feed : feeds) {
    if (dynamic_cast<GCalendarFeed*>(feed) == nullptr || feed->account() != this ||
        existing.value(feed->id()) != feed || seen.contains(feed->id())) {
      throw ApplicationException(tr("Select calendars belonging to this account."));
    }
    seen.insert(feed->id());
    auto copy = QSharedPointer<Feed>::create(*feed);
    if (apply_common) {
      apply_common(copy.data());
    }
    prepared.append(copy);
  }
  const auto now = reminderNow();
  const auto saved = qApp->database()->worker()->write<QList<QVariantHash>>([&](const QSqlDatabase& db) {
    auto connection = db;
    if (!connection.transaction()) {
      throw SqlException(connection.lastError());
    }
    QList<QVariantHash> data;
    try {
      const auto account_data = metadata(DatabaseQueries::loadAccountCustomData(db, accountId()));
      const auto account = CalendarReminderSettings::fromJson(account_data.value(QSL("reminders")).toObject());
      const int future_days = account_data.value(QSL("future_days")).toInt(365);
      // Read and validate every namespace before writing any selected calendar.
      for (auto* feed : feeds) {
        data.append(calendarDataWithOverride(feedCustomData(db, feed->id(), accountId()),
                                             reminder_override,
                                             account,
                                             future_days,
                                             now));
      }
      for (int i = 0; i < feeds.size(); ++i) {
        if (apply_common) {
          DatabaseQueries::updateFeedCommonSettings(db, prepared.at(i).data(), accountId());
        }
        if (!reminder_override.isEmpty()) {
          DatabaseQueries::storeCustomData(db, QSL("Feeds"), feeds.at(i)->id(), data.at(i));
        }
      }
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
  QList<RootItem*> changed;
  for (int i = 0; i < feeds.size(); ++i) {
    if (apply_common) {
      apply_common(feeds.at(i));
    }
    feeds.at(i)->setCustomDatabaseData(saved.at(i));
    changed.append(feeds.at(i));
  }
  itemChanged(changed);
}
