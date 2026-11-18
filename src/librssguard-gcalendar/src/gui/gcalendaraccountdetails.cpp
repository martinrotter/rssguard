// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendaraccountdetails.h"

#include <algorithm>
#include <librssguard/definitions/definitions.h>
#include <librssguard/network-web/oauth2service.h>
#include <utility>

#include <QtConcurrent>

GCalendarAccountDetails::GCalendarAccountDetails(QWidget* parent, GCalendarNetworkFactory::Transport transport)
  : QWidget(parent), m_transport(std::move(transport)) {
  m_ui.setupUi(this);
  m_ui.m_txtClientSecret->lineEdit()->setPasswordMode(true);
  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_lblTestResult->label()->setTextFormat(Qt::PlainText);
  m_ui.m_lblOAuthHelp
    ->setHelpText(tr("Use OAuth credentials for a Google desktop application with the Calendar API "
                     "enabled. Test setup authorizes and verifies Calendar access. Save the account "
                     "to keep the tested authorization. Calendar authorization is separate from Gmail."),
                  false);
  setTabOrder(m_ui.m_txtName->lineEdit(), m_ui.m_txtClientId->lineEdit());
  setTabOrder(m_ui.m_txtClientId->lineEdit(), m_ui.m_txtClientSecret->lineEdit());
  setTabOrder(m_ui.m_txtClientSecret->lineEdit(), m_ui.m_spinHistoryDays);
  setTabOrder(m_ui.m_spinHistoryDays, m_ui.m_spinFutureDays);
  setTabOrder(m_ui.m_spinFutureDays, m_ui.m_cbRefreshCalendarListOnStartup);
  setTabOrder(m_ui.m_cbRefreshCalendarListOnStartup, m_ui.m_btnTestSetup);
  connect(m_ui.m_txtName->lineEdit(), &QLineEdit::textChanged, this, &GCalendarAccountDetails::validateFields);
  for (auto* input : {m_ui.m_txtClientId->lineEdit(), m_ui.m_txtClientSecret->lineEdit()}) {
    connect(input, &QLineEdit::textChanged, this, &GCalendarAccountDetails::invalidateTest);
  }
  invalidateTest();
}

GCalendarAccountDetails::~GCalendarAccountDetails() {
  m_context.m_cancelled->store(true);
  if (m_oauth != nullptr) {
    m_oauth->logout();
  }
  if (m_check != nullptr) {
    m_check->waitForFinished();
  }
}

bool GCalendarAccountDetails::isTesting() const {
  return m_testing;
}

QString GCalendarAccountDetails::testedRefreshToken() const {
  return !m_testing && m_testedId == m_ui.m_txtClientId->lineEdit()->text().trimmed() &&
             m_testedSecret == m_ui.m_txtClientSecret->lineEdit()->text().trimmed()
           ? m_refreshToken
           : QString();
}

void GCalendarAccountDetails::validateFields() {
  using Status = WidgetWithStatus::StatusType;
  const bool name_empty = m_ui.m_txtName->lineEdit()->text().trimmed().isEmpty();
  m_ui.m_txtName->setStatus(name_empty ? Status::Error : Status::Ok,
                            name_empty ? tr("Enter an account name.") : tr("Account name is set."));
  const bool id_empty = m_ui.m_txtClientId->lineEdit()->text().trimmed().isEmpty();
#if defined(GCALENDAR_OFFICIAL_SUPPORT)
  m_ui.m_txtClientId->setStatus(Status::Ok,
                                id_empty ? tr("Preconfigured client ID will be used.") : tr("Client ID is set."));
#else
  m_ui.m_txtClientId->setStatus(id_empty ? Status::Error : Status::Ok,
                                id_empty ? tr("Enter your Google desktop application's client ID.")
                                         : tr("Client ID is set."));
#endif
  const bool secret_empty = m_ui.m_txtClientSecret->lineEdit()->text().trimmed().isEmpty();
  m_ui.m_txtClientSecret->setStatus(secret_empty ? Status::Information : Status::Ok,
                                    secret_empty ? tr("No client secret entered.") : tr("Client secret is set."));
  m_ui.m_btnTestSetup->setEnabled(!m_testing && m_ui.m_txtClientId->status() != Status::Error);
}

void GCalendarAccountDetails::setTesting(bool testing) {
  m_testing = testing;
  m_ui.m_txtClientId->setEnabled(!testing);
  m_ui.m_txtClientSecret->setEnabled(!testing);
  validateFields();
  emit testRunningChanged(testing);
}

void GCalendarAccountDetails::invalidateTest() {
  m_refreshToken.clear();
  m_candidateRefreshToken.clear();
  m_context.m_cancelled->store(true);
  if (m_oauth != nullptr) {
    m_oauth->logout();
  }
  if (m_check == nullptr) {
    setTesting(false);
  }
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("Not tested yet."),
                                  tr("Not tested yet."));
}

void GCalendarAccountDetails::startAuthorization(OAuth2Service* oauth) {
  oauth->login();
}

void GCalendarAccountDetails::testSetup(const QNetworkProxy& proxy) {
  if (m_testing) {
    return;
  }
  validateFields();
  if (m_ui.m_txtClientId->status() == WidgetWithStatus::StatusType::Error) {
    finishTest(false, tr("Enter your Google desktop application's client ID."));
    return;
  }
  m_refreshToken.clear();
  m_candidateRefreshToken.clear();
  m_testedId = m_ui.m_txtClientId->lineEdit()->text().trimmed();
  m_testedSecret = m_ui.m_txtClientSecret->lineEdit()->text().trimmed();
  m_context = GCalendarNetworkFactory::networkContext(proxy);
  delete m_oauth;
  m_oauth = GCalendarNetworkFactory::createOAuth(m_testedId, m_testedSecret, proxy, this);
  connect(m_oauth, &OAuth2Service::tokensRetrieved, this, [this](const QString& access, const QString& refresh, int) {
    if (m_testing && m_check == nullptr) {
      testApi(access, refresh);
    }
  });
  connect(m_oauth, &OAuth2Service::tokensRetrieveError, this, [this](const QString&, const QString& description) {
    if (m_testing && m_check == nullptr) {
      finishTest(false, description);
    }
  });
  connect(m_oauth, &OAuth2Service::authFailed, this, [this]() {
    if (m_testing && m_check == nullptr) {
      finishTest(false, tr("You did not grant access."));
    }
  });
  setTesting(true);
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Progress,
                                  tr("Waiting for authorization..."),
                                  tr("Testing setup."));
  m_oauth->setRedirectUrl(QSL("http://127.0.0.1:0"), true);
  startAuthorization(m_oauth);
}

void GCalendarAccountDetails::testApi(const QString& access, const QString& refresh) {
  if (access.isEmpty()) {
    finishTest(false, tr("Authorization returned no access token."));
    return;
  }
  if (refresh.isEmpty()) {
    finishTest(false, tr("Authorization returned no refresh token. Test setup again to grant offline access."));
    return;
  }
  m_candidateRefreshToken = refresh;
  m_context.m_bearer = QSL("Bearer ") + access;
  const auto context = m_context;
  const auto transport = m_transport;
  auto* watcher = new QFutureWatcher<TestResult>(this);
  m_check = watcher;
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Progress,
                                  tr("Testing Calendar access..."),
                                  tr("Testing setup."));
  connect(watcher, &QFutureWatcher<TestResult>::finished, this, [this, watcher, context]() {
    watcher->deleteLater();
    m_check = nullptr;
    if (context.m_cancelled->load()) {
      invalidateTest();
      emit setupTestFinished(false);
      return;
    }
    const auto result = watcher->result();
    if (result.m_error.isEmpty()) {
      m_refreshToken = m_candidateRefreshToken;
    }
    finishTest(result.m_error.isEmpty(), result.m_error.isEmpty() ? tr("Tested successfully.") : result.m_error);
  });
  watcher->setFuture(QtConcurrent::run([context, transport]() {
    TestResult result;
    try {
      GCalendarNetworkFactory network(context, transport);
      const auto calendars = network.calendars();
      const auto readable = std::find_if(calendars.constBegin(), calendars.constEnd(), [](const QJsonObject& calendar) {
        const auto role = calendar.value(QSL("accessRole")).toString();
        return role == QLatin1String("owner") || role == QLatin1String("writer") || role == QLatin1String("reader") ||
               role == QLatin1String("writerWithoutPrivateAccess");
      });
      if (readable == calendars.constEnd()) {
        throw ApplicationException(tr("No calendar with readable event details was found."));
      }
      const auto now = QDateTime::currentDateTimeUtc();
      network.events(readable->value(QSL("id")).toString(), now, now.addSecs(1));
    }
    catch (const ApplicationException& ex) {
      result.m_error = ex.message();
    }
    return result;
  }));
}

void GCalendarAccountDetails::finishTest(bool succeeded, const QString& message) {
  if (!succeeded) {
    m_refreshToken.clear();
  }
  setTesting(false);
  m_ui.m_lblTestResult->setStatus(succeeded ? WidgetWithStatus::StatusType::Ok : WidgetWithStatus::StatusType::Error,
                                  message,
                                  message);
  emit setupTestFinished(succeeded);
}
