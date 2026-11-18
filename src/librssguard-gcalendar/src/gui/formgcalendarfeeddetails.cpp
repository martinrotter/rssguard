// For license of this file, see <project-root-folder>/LICENSE.md.

#include "formgcalendarfeeddetails.h"

#include "gcalendarreminderdetails.h"
#include "src/gcalendarfeed.h"
#include "src/gcalendarserviceroot.h"

FormGCalendarFeedDetails::FormGCalendarFeedDetails(ServiceRoot* account, QWidget* parent)
  : FormFeedDetails(account, parent),
    m_remindersDetails(new GCalendarReminderDetails(GCalendarReminderDetails::Scope::Calendar, this)) {
  insertScrollableCustomTab(m_remindersDetails, tr("Reminders"), 0);
  activateTab(0);
}

void FormGCalendarFeedDetails::loadFeedData() {
  FormFeedDetails::loadFeedData();
  QList<QJsonObject> metadata;
  for (auto* feed : feeds<Feed>()) {
    auto* calendar = dynamic_cast<GCalendarFeed*>(feed);
    if (calendar == nullptr) {
      throw ApplicationException(tr("Select calendars belonging to this account."));
    }
    metadata.append(calendar->calendarMetadata());
  }
  auto* account = qobject_cast<GCalendarServiceRoot*>(m_serviceRoot);
  m_remindersDetails->loadCalendars(account->reminderSettings(), metadata);
}

void FormGCalendarFeedDetails::apply() {
  const auto settings = m_remindersDetails->calendarOverride();
  auto* account = qobject_cast<GCalendarServiceRoot*>(m_serviceRoot);
  account->saveCalendarFeedSettings(feeds<Feed>(), settings, [this](Feed* feed) {
    applyCommonFeedSettings(feed);
  });
}
