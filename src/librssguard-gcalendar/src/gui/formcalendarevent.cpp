// For license of this file, see <project-root-folder>/LICENSE.md.

#include "formcalendarevent.h"

#include "src/gcalendarfeed.h"
#include "src/gcalendarserviceroot.h"

#include <algorithm>
#include <librssguard/gui/guiutilities.h>
#include <librssguard/miscellaneous/application.h>

#include <QCloseEvent>
#include <QMessageBox>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>

namespace {
  const QStringList frequencies{QString(),
                                QStringLiteral("DAILY"),
                                QStringLiteral("WEEKLY"),
                                QStringLiteral("MONTHLY"),
                                QStringLiteral("YEARLY")};
}

FormCalendarEvent::FormCalendarEvent(GCalendarServiceRoot* account,
                                     const Message* message,
                                     const QString& calendar_id,
                                     QWidget* parent)
  : QDialog(parent), m_account(account) {
  m_ui.setupUi(this);
  m_ui.m_lblStatus->label()->setWordWrap(true);
  m_ui.m_lblStatus->label()->setTextFormat(Qt::PlainText);
  setStatus(WidgetWithStatus::StatusType::Information, {});
  GuiUtilities::applyDialogProperties(*this, account->icon());
  setWindowTitle(message == nullptr ? tr("New event") : tr("Edit event"));
  m_loading = true;
  auto feeds = account->getSubTreeFeeds();
  std::sort(feeds.begin(), feeds.end(), [](const Feed* lhs, const Feed* rhs) {
    return lhs->title().compare(rhs->title(), Qt::CaseSensitivity::CaseInsensitive) < 0;
  });
  for (auto* feed : feeds) {
    auto* calendar = dynamic_cast<GCalendarFeed*>(feed);
    if (calendar == nullptr) {
      continue;
    }
    const auto role = calendar->calendarResource().value(QSL("accessRole")).toString();
    const bool writable = role == QLatin1String("owner") || role == QLatin1String("writer") ||
                          role == QLatin1String("writerWithoutPrivateAccess");
    if ((message != nullptr && feed->id() == message->m_feedId) ||
        (message == nullptr && writable && !feed->isSwitchedOff())) {
      m_ui.m_cmbCalendar->addItem(feed->icon(), feed->title(), feed->customId());
    }
  }
  if (m_ui.m_cmbCalendar->count() == 0) {
    throw ApplicationException(tr("No writable calendar is available. Refresh the calendar list first."));
  }
  if (!calendar_id.isEmpty()) {
    const int selected = m_ui.m_cmbCalendar->findData(calendar_id);
    if (selected >= 0) {
      m_ui.m_cmbCalendar->setCurrentIndex(selected);
    }
  }
  m_calendarId = m_ui.m_cmbCalendar->currentData().toString();
  auto* calendar = dynamic_cast<GCalendarFeed*>(account->getHashedSubTreeFeeds().value(m_calendarId));
  const auto zone = calendar->calendarResource().value(QSL("timeZone")).toString(QSL("UTC")).toUtf8();
  if (message == nullptr) {
    m_occurrence = CalendarEventDraft::create(zone, QDateTime::currentDateTimeUtc());
  }
  else {
    const auto event = CalendarEvent::fromMessage(*message, zone);
    if (!event.canEdit(calendar->calendarResource().value(QSL("accessRole")).toString())) {
      throw ApplicationException(tr("This event is read-only."));
    }
    m_occurrence = CalendarEventDraft::fromEvent(event);
    m_messageId = message->m_id;
    m_seriesId = event.m_resource.value(QSL("recurringEventId")).toString();
    m_ui.m_cmbCalendar->setEnabled(false);
  }
  m_ui.m_cmbTarget->setVisible(!m_seriesId.isEmpty());
  m_ui.m_lblTarget->setVisible(!m_seriesId.isEmpty());
  for (const auto& id : QTimeZone::availableTimeZoneIds()) {
    m_ui.m_cmbTimeZone->addItem(QString::fromUtf8(id));
  }
  connect(m_ui.m_cmbCalendar, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
    m_calendarId = m_ui.m_cmbCalendar->currentData().toString();
  });
  connect(m_ui.m_cmbTarget,
          QOverload<int>::of(&QComboBox::currentIndexChanged),
          this,
          &FormCalendarEvent::selectTarget);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormCalendarEvent::save);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &FormCalendarEvent::reject);
  connect(m_ui.m_btnReload, &QPushButton::clicked, this, [this]() {
    reload(false);
  });
  connect(m_ui.m_btnReapply, &QPushButton::clicked, this, [this]() {
    reload(true);
  });
  const auto update_dates = [this]() {
    if (!m_loading) {
      updateDateControls();
    }
  };
  connect(m_ui.m_dateStart, &QDateEdit::dateChanged, this, update_dates);
  connect(m_ui.m_dateEnd, &QDateEdit::dateChanged, this, update_dates);
  connect(m_ui.m_timeStart, &QTimeEdit::timeChanged, this, update_dates);
  connect(m_ui.m_timeEnd, &QTimeEdit::timeChanged, this, update_dates);
  connect(m_ui.m_cmbTimeZone, &QComboBox::currentTextChanged, this, update_dates);
  connect(m_ui.m_cbAllDay, &QCheckBox::toggled, this, update_dates);
  connect(m_ui.m_cmbRecurrence, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
    const bool basic = m_ui.m_cmbRecurrence->currentIndex() > 0 && m_ui.m_cmbRecurrence->currentIndex() < 5;
    m_ui.m_spinRepeatCount->setVisible(basic);
    m_ui.m_lblRepeatCount->setVisible(basic);
    m_ui.m_lblRecurrenceHint->setVisible(m_ui.m_cmbRecurrence->currentIndex() == 5 ||
                                         (!m_series && !m_seriesId.isEmpty()));
  });
  connect(account, &GCalendarServiceRoot::eventOperationFinished, this, &FormCalendarEvent::finish);
  connect(account, &QObject::destroyed, this, [this]() {
    m_busy = false;
    QDialog::reject();
  });
  connect(account, &GCalendarServiceRoot::accountStopped, this, [this]() {
    m_busy = false;
    QDialog::reject();
  });
  m_loading = false;
  loadForm();
}

void FormCalendarEvent::setStatus(WidgetWithStatus::StatusType status, const QString& text) {
  m_ui.m_lblStatus->setStatus(status, text, text);
  m_ui.m_lblStatus->setVisible(!text.isEmpty());
}

CalendarEventDraft& FormCalendarEvent::draft() {
  return m_series ? *m_seriesDraft : m_occurrence;
}

void FormCalendarEvent::loadForm() {
  m_loading = true;
  const auto& fields = draft().m_fields;
  m_ui.m_txtTitle->setText(fields.m_title);
  m_ui.m_txtDescription->setPlainText(fields.m_description);
  m_ui.m_txtLocation->setText(fields.m_location);
  m_ui.m_cbAllDay->setChecked(fields.m_allDay);
  m_ui.m_dateStart->setDate(fields.m_startDate);
  m_ui.m_dateEnd->setDate(fields.m_endDate);
  m_ui.m_timeStart->setTime(fields.m_startTime);
  m_ui.m_timeEnd->setTime(fields.m_endTime);
  m_ui.m_cmbTimeZone->setCurrentText(QString::fromUtf8(fields.m_zone));
  m_loadedRecurrence = fields.m_recurrence.isEmpty() ? 0 : 5;
  m_loadedRules = fields.m_recurrence;
  m_loadedCount = 0;
  if (fields.m_recurrence.size() == 1) {
    static const QRegularExpression basic(QSL("^RRULE:FREQ=(DAILY|WEEKLY|MONTHLY|YEARLY)(?:;COUNT=([1-9][0-9]*))?$"));
    const auto match = basic.match(fields.m_recurrence.first().toString());
    bool count_valid = true;
    const int count = match.captured(2).isEmpty() ? 0 : match.captured(2).toInt(&count_valid);
    if (match.hasMatch() && count_valid && count <= m_ui.m_spinRepeatCount->maximum()) {
      m_loadedRecurrence = frequencies.indexOf(match.captured(1));
      m_loadedCount = count;
    }
  }
  m_ui.m_cmbRecurrence->setCurrentIndex(m_loadedRecurrence);
  m_ui.m_spinRepeatCount->setValue(m_loadedCount);
  m_ui.m_cmbRecurrence->setEnabled(m_series || m_seriesId.isEmpty());
  const bool basic = m_loadedRecurrence > 0 && m_loadedRecurrence < 5;
  m_ui.m_spinRepeatCount->setVisible(basic);
  m_ui.m_lblRepeatCount->setVisible(basic);
  m_ui.m_lblRecurrenceHint->setVisible(m_loadedRecurrence == 5 || (!m_series && !m_seriesId.isEmpty()));
  m_ui.m_cbNotifyAttendees->setVisible(!draft().baseline().value(QSL("attendees")).toArray().isEmpty());
  updateDateControls(true);
  m_loading = false;
}

void FormCalendarEvent::readForm() {
  auto& fields = draft().m_fields;
  fields.m_title = m_ui.m_txtTitle->text();
  fields.m_description = m_ui.m_txtDescription->toPlainText();
  fields.m_location = m_ui.m_txtLocation->text();
  fields.m_allDay = m_ui.m_cbAllDay->isChecked();
  fields.m_startDate = m_ui.m_dateStart->date();
  fields.m_endDate = m_ui.m_dateEnd->date();
  fields.m_startTime = m_ui.m_timeStart->time();
  fields.m_endTime = m_ui.m_timeEnd->time();
  fields.m_zone = m_ui.m_cmbTimeZone->currentText().trimmed().toUtf8();
  fields.m_startOccurrence = m_ui.m_cmbStartOccurrence->currentData(Qt::UserRole + 1).toInt();
  fields.m_endOccurrence = m_ui.m_cmbEndOccurrence->currentData(Qt::UserRole + 1).toInt();
  if ((m_ui.m_cmbRecurrence->currentIndex() == m_loadedRecurrence &&
       m_ui.m_spinRepeatCount->value() == m_loadedCount) ||
      m_ui.m_cmbRecurrence->currentIndex() == 5) {
    fields.m_recurrence = m_loadedRules;
  }
  else {
    const int index = m_ui.m_cmbRecurrence->currentIndex();
    if (index == 0) {
      fields.m_recurrence = {};
    }
    else if (index > 0 && index < frequencies.size()) {
      QString rule = QSL("RRULE:FREQ=") + frequencies.at(index);
      if (m_ui.m_spinRepeatCount->value() > 0) {
        rule += QSL(";COUNT=%1").arg(m_ui.m_spinRepeatCount->value());
      }
      fields.m_recurrence = QJsonArray{rule};
    }
  }
}

void FormCalendarEvent::updateDateControls(bool loading) {
  const bool all_day = m_ui.m_cbAllDay->isChecked();
  m_ui.m_timeStart->setVisible(!all_day);
  m_ui.m_timeEnd->setVisible(!all_day);
  const QTimeZone zone(m_ui.m_cmbTimeZone->currentText().trimmed().toUtf8());
  const auto update = [&](QComboBox* combo, const QDate& date, const QTime& time, int stored) {
    const auto selected = combo->currentData();
    const QSignalBlocker blocker(combo);
    const auto choices = CalendarEventDraft::occurrences(date, time, zone);
    combo->clear();
    if (choices.size() > 1) {
      combo->addItem(tr("Choose occurrence"));
      combo->setItemData(0, -1, Qt::UserRole + 1);
      for (int i = 0; i < choices.size(); ++i) {
        combo->addItem(tr("%1 (%2 UTC)").arg(i + 1).arg(choices.at(i).time().toString(QSL("HH:mm"))),
                       choices.at(i).toMSecsSinceEpoch());
        combo->setItemData(i + 1, i, Qt::UserRole + 1);
      }
      const int index = loading ? stored + 1 : combo->findData(selected);
      combo->setCurrentIndex(index >= 0 && index < combo->count() ? index : 0);
    }
    else {
      combo->addItem(QString(), choices.isEmpty() ? QVariant() : QVariant(choices.first().toMSecsSinceEpoch()));
      combo->setItemData(0, 0, Qt::UserRole + 1);
    }
    combo->setVisible(!all_day && choices.size() > 1);
  };
  update(m_ui.m_cmbStartOccurrence,
         m_ui.m_dateStart->date(),
         m_ui.m_timeStart->time(),
         draft().m_fields.m_startOccurrence);
  update(m_ui.m_cmbEndOccurrence, m_ui.m_dateEnd->date(), m_ui.m_timeEnd->time(), draft().m_fields.m_endOccurrence);
}

void FormCalendarEvent::setBusy(bool busy) {
  m_busy = busy;
  const QList<QWidget*> widgets{m_ui.m_cmbCalendar,
                                m_ui.m_cmbTarget,
                                m_ui.m_txtTitle,
                                m_ui.m_txtDescription,
                                m_ui.m_txtLocation,
                                m_ui.m_cbAllDay,
                                m_ui.m_dateStart,
                                m_ui.m_dateEnd,
                                m_ui.m_timeStart,
                                m_ui.m_timeEnd,
                                m_ui.m_cmbTimeZone,
                                m_ui.m_cmbStartOccurrence,
                                m_ui.m_cmbEndOccurrence,
                                m_ui.m_cmbRecurrence,
                                m_ui.m_spinRepeatCount,
                                m_ui.m_cbNotifyAttendees};
  for (auto* widget : widgets) {
    widget->setEnabled(!busy);
  }
  m_ui.m_cmbCalendar->setEnabled(!busy && draft().isNew() && !m_creationCalendarLocked);
  m_ui.m_cmbRecurrence->setEnabled(!busy && (m_series || m_seriesId.isEmpty()));
  m_ui.m_buttonBox->setEnabled(!busy);
  m_ui.m_btnReload->setEnabled(!busy);
  m_ui.m_btnReapply->setEnabled(!busy);
}

void FormCalendarEvent::save() {
  if (m_busy || m_account == nullptr) {
    return;
  }
  try {
    readForm();
    const auto changes = draft().changes();
    if (changes.isEmpty()) {
      accept();
      return;
    }
    if (draft().isNew()) {
      m_creationCalendarLocked = true;
      if (!m_creationUncertain) {
        m_creationAttempt = draft();
      }
    }
    setStatus(WidgetWithStatus::StatusType::Progress, tr("Saving event..."));
    m_request = m_account->saveRemoteEvent(m_calendarId,
                                           draft().baseline(),
                                           changes,
                                           draft().isNew(),
                                           m_ui.m_cbNotifyAttendees->isChecked(),
                                           m_messageId);
    m_pending = Pending::Save;
    setBusy(true);
  }
  catch (const ApplicationException& ex) {
    setStatus(WidgetWithStatus::StatusType::Error, ex.message());
  }
}

void FormCalendarEvent::selectTarget(int target) {
  if (m_loading || m_busy || m_account == nullptr) {
    return;
  }
  readForm();
  if (target == 1 && !m_seriesDraft) {
    try {
      m_request = m_account->loadRemoteEvent(m_calendarId, m_seriesId);
      m_pending = Pending::Series;
      setBusy(true);
      setStatus(WidgetWithStatus::StatusType::Progress, tr("Loading the whole series..."));
    }
    catch (const ApplicationException& ex) {
      const QSignalBlocker block(m_ui.m_cmbTarget);
      m_ui.m_cmbTarget->setCurrentIndex(0);
      setStatus(WidgetWithStatus::StatusType::Error, ex.message());
    }
    return;
  }
  m_series = target == 1;
  loadForm();
}

void FormCalendarEvent::reload(bool reapply) {
  if (m_busy || m_account == nullptr || draft().isNew()) {
    return;
  }
  try {
    readForm();
    if (reapply) {
      draft().changes();
    }
    m_request = m_account->loadRemoteEvent(m_calendarId, draft().eventId());
    m_pending = reapply ? Pending::Reapply : Pending::Reload;
    setBusy(true);
  }
  catch (const ApplicationException& ex) {
    setStatus(WidgetWithStatus::StatusType::Error, ex.message());
  }
}

void FormCalendarEvent::finish(quint64 id,
                               const QJsonObject& resource,
                               int status,
                               const QString& error,
                               bool confirmed,
                               int message_id,
                               bool recovered_creation) {
  if (id != m_request || m_pending == Pending::None) {
    return;
  }
  const auto pending = m_pending;
  m_pending = Pending::None;
  setBusy(false);
  if (pending == Pending::Save) {
    if (confirmed) {
      if (recovered_creation && m_creationAttempt && error.isEmpty()) {
        const auto zone = m_creationAttempt->m_fields.m_zone;
        auto later =
          CalendarEventDraft::fromEvent(CalendarEvent::fromJson(m_calendarId, m_creationAttempt->resource(), zone));
        later.m_fields = draft().m_fields;
        later.rebase(CalendarEvent::fromJson(m_calendarId, resource, zone));
        if (!later.changes().isEmpty()) {
          m_occurrence = later;
          m_creationAttempt.reset();
          m_creationUncertain = false;
          m_messageId = message_id;
          loadForm();
          setStatus(WidgetWithStatus::StatusType::Information,
                    tr("The earlier creation succeeded. Your later edits are retained. Review them, then save."));
          return;
        }
      }
      if (!error.isEmpty()) {
        QMessageBox::warning(this, tr("Event saved"), error);
      }
      emit eventStored(message_id);
      accept();
    }
    else if (status == 412) {
      setStatus(WidgetWithStatus::StatusType::Warning,
                tr("This event changed in Google Calendar. Your draft is retained. Reload it or "
                   "reapply your edits to the latest version."));
      m_ui.m_btnReload->show();
      m_ui.m_btnReapply->show();
    }
    else {
      setStatus(WidgetWithStatus::StatusType::Error, error);
    }
    if (!confirmed && m_creationAttempt && (status == 0 || status >= 500 || status == 408 || recovered_creation)) {
      m_creationUncertain = true;
    }
    return;
  }
  try {
    if (!error.isEmpty()) {
      throw ApplicationException(error);
    }
    const auto event = CalendarEvent::fromJson(m_calendarId, resource, draft().m_fields.m_zone);
    if (pending == Pending::Series) {
      m_seriesDraft = CalendarEventDraft::fromEvent(event);
      m_series = true;
    }
    else if (pending == Pending::Reapply) {
      draft().rebase(event);
    }
    else {
      draft() = CalendarEventDraft::fromEvent(event);
    }
    loadForm();
    setStatus(WidgetWithStatus::StatusType::Information,
              pending == Pending::Reapply ? tr("Your edits have been reapplied. Review them, then save.") : QString());
    m_ui.m_btnReload->hide();
    m_ui.m_btnReapply->hide();
  }
  catch (const ApplicationException& ex) {
    if (pending == Pending::Series) {
      const QSignalBlocker block(m_ui.m_cmbTarget);
      m_ui.m_cmbTarget->setCurrentIndex(0);
    }
    setStatus(WidgetWithStatus::StatusType::Error, ex.message());
  }
}

void FormCalendarEvent::reject() {
  if (!m_busy) {
    QDialog::reject();
  }
}
void FormCalendarEvent::closeEvent(QCloseEvent* event) {
  if (m_busy) {
    event->ignore();
  }
  else {
    QDialog::closeEvent(event);
  }
}
