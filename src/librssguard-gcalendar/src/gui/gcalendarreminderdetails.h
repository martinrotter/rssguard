// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GCALENDARREMINDERDETAILS_H
#define GCALENDARREMINDERDETAILS_H

#include "src/calendarreminders.h"

#include "ui_gcalendarremindersettings.h"

#include <QWidget>

class GCalendarReminderDetails : public QWidget {
    Q_OBJECT

  public:
    enum class Scope {
      Account,
      Calendar
    };

    explicit GCalendarReminderDetails(Scope scope, QWidget* parent = nullptr);
    void loadAccount(const CalendarReminderSettings& settings);
    CalendarReminderSettings accountSettings(const CalendarReminderSettings& previous) const;
    void loadCalendars(const CalendarReminderSettings& account, const QList<QJsonObject>& metadata);
    QJsonObject calendarOverride() const;

    Ui::GCalendarReminderSettings m_ui;

  private:
    void updateControls();
    QJsonObject calendarInput() const;
    QList<CalendarReminderRule> localRules(const QList<CalendarReminderRule>& previous) const;

    Scope m_scope;
    bool m_batch = false;
    QJsonObject m_initialCalendarInput;
};

#endif // GCALENDARREMINDERDETAILS_H
