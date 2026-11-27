// For license of this file, see <project-root-folder>/LICENSE.md.

#include "formgcalendarfeeddetails.h"

#include "gcalendarreminderdetails.h"
#include "src/gcalendarfeed.h"
#include "src/gcalendarserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/exceptions/sqlexception.h>
#include <librssguard/miscellaneous/application.h>

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
  if (settings.isEmpty()) {
    FormFeedDetails::apply();
    return;
  }
  const auto staged = account->calendarReminderData(m_feeds, settings);
  FormFeedDetails::apply();
  qApp->database()->worker()->write([&](const QSqlDatabase& db) {
    auto connection = db;
    if (!connection.transaction()) {
      throw SqlException(connection.lastError());
    }
    try {
      for (int i = 0; i < m_feeds.size(); ++i) {
        DatabaseQueries::storeCustomData(db, QSL("Feeds"), m_feeds.at(i)->id(), staged.at(i));
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
  for (int i = 0; i < m_feeds.size(); ++i) {
    m_feeds.at(i)->setCustomDatabaseData(staged.at(i));
  }
}
