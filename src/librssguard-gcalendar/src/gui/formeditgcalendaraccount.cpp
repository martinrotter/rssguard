// For license of this file, see <project-root-folder>/LICENSE.md.

#include "formeditgcalendaraccount.h"

#include "gcalendaraccountdetails.h"
#include "gcalendarreminderdetails.h"
#include "src/gcalendarserviceroot.h"

#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/services/abstract/importantnode.h>
#include <librssguard/services/abstract/labelsnode.h>
#include <librssguard/services/abstract/searchsnode.h>
#include <librssguard/services/abstract/unreadnode.h>

FormEditGCalendarAccount::FormEditGCalendarAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("gcalendar")), parent), m_details(new GCalendarAccountDetails(this)),
    m_remindersDetails(new GCalendarReminderDetails(GCalendarReminderDetails::Scope::Account, this)) {
  insertScrollableCustomTab(m_details, tr("Calendar setup"), 0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, [this]() {
    m_details->testSetup(m_proxyDetails->proxy());
  });
  connect(m_proxyDetails, &NetworkProxyDetails::changed, m_details, &GCalendarAccountDetails::invalidateTest);
  connect(m_details, &GCalendarAccountDetails::testRunningChanged, m_proxyDetails, &QWidget::setDisabled);
  insertScrollableCustomTab(m_remindersDetails, tr("Reminders"), 1);
  activateTab(0);
}

bool FormEditGCalendarAccount::loadAccountData() {
  if (!FormAccountDetails::loadAccountData()) {
    return false;
  }
  const auto data = account<GCalendarServiceRoot>()->customDatabaseData();
  const auto calendar = account<GCalendarServiceRoot>()->calendarSettings();
  const auto reminders = account<GCalendarServiceRoot>()->reminderSettings();
  m_details->m_ui.m_txtName->lineEdit()->setText(account<GCalendarServiceRoot>()->title());
  m_details->m_ui.m_txtClientId->lineEdit()->setText(data.value(QSL("client_id")).toString());
  m_details->m_ui.m_txtClientSecret->lineEdit()->setText(data.value(QSL("client_secret")).toString());
  m_details->m_ui.m_spinHistoryDays->setValue(calendar.value(QSL("history_days")).toInt(30));
  m_details->m_ui.m_spinFutureDays->setValue(calendar.value(QSL("future_days")).toInt(365));
  m_details->m_ui.m_cbRefreshCalendarListOnStartup
    ->setChecked(calendar.value(QSL("refresh_calendar_list_on_startup")).toBool(false));
  m_remindersDetails->loadAccount(reminders);
  return true;
}

FormEditGCalendarAccount::~FormEditGCalendarAccount() {
  if (m_creatingNew && result() != QDialog::Accepted) {
    delete m_account;
  }
}

void FormEditGCalendarAccount::apply() {
  if (m_details->isTesting()) {
    throw ApplicationException(tr("Wait for the setup test to finish before saving."));
  }
  const auto reminders = m_remindersDetails->accountSettings(account<GCalendarServiceRoot>()->reminderSettings());
  auto* calendar = account<GCalendarServiceRoot>();
  const auto previous_data = calendar->customDatabaseData();
  const auto proxy = calendar->networkProxy();
  try {
    FormAccountDetails::apply();
    calendar->applySettings(m_details->m_ui.m_txtName->lineEdit()->text(),
                            m_details->m_ui.m_txtClientId->lineEdit()->text(),
                            m_details->m_ui.m_txtClientSecret->lineEdit()->text(),
                            m_details->m_ui.m_spinHistoryDays->value(),
                            m_details->m_ui.m_spinFutureDays->value(),
                            reminders,
                            m_details->testedRefreshToken(),
                            m_details->m_ui.m_cbRefreshCalendarListOnStartup->isChecked());
  }
  catch (...) {
    calendar->setCustomDatabaseData(previous_data);
    calendar->setNetworkProxy(proxy);
    calendar->itemChanged({calendar->importantNode(),
                           calendar->labelsNode(),
                           calendar->unreadNode(),
                           calendar->probesNode()});
    throw;
  }
}
