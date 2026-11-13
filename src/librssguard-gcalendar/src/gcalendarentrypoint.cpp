// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gcalendarentrypoint.h"

#include "gcalendarserviceroot.h"
#include "gui/formeditgcalendaraccount.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

GCalendarEntryPoint::GCalendarEntryPoint(QObject* parent) : QObject(parent) {}

ServiceRoot* GCalendarEntryPoint::createNewRoot() const {
  FormEditGCalendarAccount form(qApp->mainFormWidget());
  return form.addEditAccount<GCalendarServiceRoot>();
}

QList<ServiceRoot*> GCalendarEntryPoint::initializeSubtree() const {
  return qApp->database()->worker()->read<QList<ServiceRoot*>>([this](const QSqlDatabase& db) {
    auto roots = DatabaseQueries::getAccounts<GCalendarServiceRoot>(db, code());
    for (auto* root : roots) {
      for (auto* item : root->getSubTree<RootItem>()) {
        item->moveToThread(qApp->thread());
      }
    }
    return roots;
  });
}

QString GCalendarEntryPoint::name() const {
  return QSL("Google Calendar");
}

QString GCalendarEntryPoint::code() const {
  return QSL(SERVICE_CODE_GCALENDAR);
}

QString GCalendarEntryPoint::description() const {
  return tr("Google Calendar events, editing and local reminders via the Calendar REST API.");
}

QString GCalendarEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon GCalendarEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("gcalendar"));
}
