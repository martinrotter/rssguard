// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendarreminderdetails.h"

#include <algorithm>
#include <librssguard/exceptions/applicationexception.h>

namespace {
  QJsonObject configuration(const CalendarReminderSettings& account, const QJsonObject& metadata) {
    const auto settings = CalendarReminderSettings::forCalendar(account, metadata);
    const auto mode = metadata.value(QStringLiteral("reminder_override"))
                        .toObject()
                        .value(QStringLiteral("mode"))
                        .toString(QStringLiteral("inherit"));
    QJsonObject result{{QStringLiteral("mode"), mode}};
    if (mode == QLatin1String("local")) {
      auto rules = settings.m_rules;
      std::sort(rules.begin(), rules.end(), [](const auto& left, const auto& right) {
        return left.m_minutesBefore < right.m_minutesBefore;
      });
      QJsonArray values;
      for (const auto& rule : rules) {
        values.append(QJsonArray{rule.m_minutesBefore, rule.m_repeatMinutes});
      }
      result[QStringLiteral("rules")] = values;
    }
    return result;
  }
} // namespace

GCalendarReminderDetails::GCalendarReminderDetails(Scope scope, QWidget* parent) : QWidget(parent), m_scope(scope) {
  m_ui.setupUi(this);
  const bool account = scope == Scope::Account;
  m_ui.m_cbEnabled->setVisible(account);
  m_ui.m_panelCalendarMode->setVisible(!account);
  m_ui.m_panelSelection->hide();
  m_ui.m_lblMixed->hide();
  m_ui.m_lblPlaceholders->setVisible(account);
  for (auto* widget : QList<QWidget*>{m_ui.m_lblCheckInterval,
                                      m_ui.m_spinCheckInterval,
                                      m_ui.m_cbGoogleReminders,
                                      m_ui.m_lblAllDayTime,
                                      m_ui.m_timeAllDay,
                                      m_ui.m_lblNotificationTitle,
                                      m_ui.m_txtNotificationTitle,
                                      m_ui.m_lblNotificationBody,
                                      m_ui.m_txtNotificationBody}) {
    widget->setVisible(account);
  }
  const QStringList modes{QStringLiteral("inherit"),
                          QStringLiteral("disabled"),
                          QStringLiteral("local"),
                          QStringLiteral("google")};
  for (int i = 0; i < modes.size(); ++i) {
    m_ui.m_cmbMode->setItemData(i, modes.at(i));
  }
  m_ui.m_cmbMode->setPlaceholderText(tr("Multiple values"));
  m_ui.m_lblTiming
    ->setHelpText(tr("Reminder timing"),
                  account
                    ? tr("Reminders use cached events while RSS Guard is running. The default check interval is 30 "
                         "minutes, so delivery "
                         "can be one interval late. Missed reminders are caught up for one interval. Repeats stop when "
                         "the event ends. "
                         "Reading an event does not dismiss reminders.")
                    : tr("The account must have reminders enabled. Check interval, all-day time and notification text "
                         "come from account "
                         "settings. Local offsets are minutes before start, separated by commas; 0 means at start."),
                  false);
  if (!account) {
    m_ui.m_mcbReminders->addActionWidget(m_ui.m_panelEditor);
  }
  connect(m_ui.m_cbEnabled, &QCheckBox::toggled, this, &GCalendarReminderDetails::updateControls);
  connect(m_ui.m_cbGoogleReminders, &QCheckBox::toggled, this, &GCalendarReminderDetails::updateControls);
  connect(m_ui.m_cmbMode,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &GCalendarReminderDetails::updateControls);
  connect(m_ui.m_txtOffsets, &QLineEdit::textChanged, this, &GCalendarReminderDetails::updateControls);
  updateControls();
}

void GCalendarReminderDetails::updateControls() {
  const bool account = m_scope == Scope::Account;
  const bool local = account ? !m_ui.m_cbGoogleReminders->isChecked()
                             : m_ui.m_cmbMode->currentData().toString() == QLatin1String("local");
  m_ui.m_panelSettings->setEnabled(account ? m_ui.m_cbEnabled->isChecked() : local);
  m_ui.m_txtOffsets->setEnabled(local);
  m_ui.m_spinRepeatInterval->setEnabled(local && !m_ui.m_txtOffsets->text().trimmed().isEmpty());
  m_ui.m_timeAllDay->setEnabled(local);
}

void GCalendarReminderDetails::loadAccount(const CalendarReminderSettings& settings) {
  m_ui.m_cbEnabled->setChecked(settings.m_enabled);
  m_ui.m_cbGoogleReminders->setChecked(settings.m_useGoogleReminders);
  m_ui.m_spinCheckInterval->setValue(settings.m_checkIntervalMinutes);
  QStringList offsets;
  for (const auto& rule : settings.m_rules) {
    offsets.append(QString::number(rule.m_minutesBefore));
  }
  m_ui.m_txtOffsets->setText(offsets.join(QStringLiteral(", ")));
  m_ui.m_spinRepeatInterval->setValue(settings.m_rules.isEmpty() ? settings.m_localRepeatMinutes
                                                                 : settings.m_rules.first().m_repeatMinutes);
  m_ui.m_timeAllDay->setTime(settings.m_allDayTime);
  m_ui.m_txtNotificationTitle->setText(settings.m_title);
  m_ui.m_txtNotificationBody->setPlainText(settings.m_body);
  updateControls();
}

QList<CalendarReminderRule> GCalendarReminderDetails::localRules(const QList<CalendarReminderRule>& previous) const {
  QHash<int, CalendarReminderRule> existing;
  for (const auto& rule : previous) {
    existing.insert(rule.m_minutesBefore, rule);
  }
  QList<CalendarReminderRule> rules;
  QSet<int> seen;
  for (const auto& offset : m_ui.m_txtOffsets->text().split(QLatin1Char(','), Qt::SkipEmptyParts)) {
    bool valid;
    const int value = offset.trimmed().toInt(&valid);
    if (!valid || value < 0 || value > 35791 || seen.contains(value)) {
      throw ApplicationException(tr("Enter distinct nonnegative reminder offsets in minutes."));
    }
    seen.insert(value);
    auto rule = existing.value(value, {QStringLiteral("local-%1").arg(value), value, 0});
    rule.m_repeatMinutes = m_ui.m_spinRepeatInterval->value();
    rules.append(rule);
  }
  return rules;
}

CalendarReminderSettings GCalendarReminderDetails::accountSettings(const CalendarReminderSettings& previous) const {
  auto settings = previous;
  settings.m_enabled = m_ui.m_cbEnabled->isChecked();
  settings.m_useGoogleReminders = m_ui.m_cbGoogleReminders->isChecked();
  settings.m_checkIntervalMinutes = m_ui.m_spinCheckInterval->value();
  settings.m_allDayTime = m_ui.m_timeAllDay->time();
  settings.m_title = m_ui.m_txtNotificationTitle->text();
  settings.m_body = m_ui.m_txtNotificationBody->toPlainText();
  settings.m_localRepeatMinutes = m_ui.m_spinRepeatInterval->value();
  settings.m_rules = localRules(previous.m_rules);
  if (settings.m_enabled && !settings.m_useGoogleReminders && settings.m_rules.isEmpty()) {
    throw ApplicationException(tr("Add at least one local reminder offset or select Google's popup offsets."));
  }
  return settings;
}

void GCalendarReminderDetails::loadCalendars(const CalendarReminderSettings& account,
                                             const QList<QJsonObject>& metadata) {
  if (metadata.isEmpty()) {
    throw ApplicationException(tr("Select at least one calendar."));
  }
  loadAccount(CalendarReminderSettings::forCalendar(account, metadata.first()));
  const auto first = configuration(account, metadata.first());
  const bool mixed = std::any_of(metadata.constBegin(), metadata.constEnd(), [&](const auto& value) {
    return configuration(account, value) != first;
  });
  m_batch = metadata.size() > 1;
  m_ui.m_panelSelection->setVisible(m_batch);
  m_ui.m_mcbReminders->setChecked(false);
  m_ui.m_panelEditor->setEnabled(!m_batch);
  m_ui.m_lblMixed->setVisible(mixed);
  m_ui.m_cmbMode->setCurrentIndex(mixed ? -1
                                        : m_ui.m_cmbMode->findData(first.value(QStringLiteral("mode")).toString()));
  if (mixed) {
    m_ui.m_txtOffsets->clear();
    m_ui.m_spinRepeatInterval->setValue(0);
  }
  updateControls();
  m_initialCalendarInput = calendarInput();
}

QJsonObject GCalendarReminderDetails::calendarInput() const {
  return {{QStringLiteral("mode"), m_ui.m_cmbMode->currentData().toString()},
          {QStringLiteral("offsets"), m_ui.m_txtOffsets->text()},
          {QStringLiteral("repeat_minutes"), m_ui.m_spinRepeatInterval->value()}};
}

QJsonObject GCalendarReminderDetails::calendarOverride() const {
  if ((m_batch && !m_ui.m_mcbReminders->isChecked()) || (!m_batch && calendarInput() == m_initialCalendarInput)) {
    return {};
  }
  if (m_ui.m_cmbMode->currentIndex() < 0) {
    throw ApplicationException(tr("Choose reminder rules for the selected calendars."));
  }
  const auto mode = m_ui.m_cmbMode->currentData().toString();
  QJsonObject result{{QStringLiteral("mode"), mode}};
  if (mode == QLatin1String("local")) {
    const auto rules = localRules({});
    if (rules.isEmpty()) {
      throw ApplicationException(tr("Enter at least one local reminder offset."));
    }
    QJsonArray values;
    for (const auto& rule : rules) {
      values.append(QJsonObject{{QStringLiteral("id"), rule.m_id},
                                {QStringLiteral("minutes_before"), rule.m_minutesBefore},
                                {QStringLiteral("repeat_minutes"), rule.m_repeatMinutes}});
    }
    result[QStringLiteral("rules")] = values;
  }
  return result;
}
