// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/greaderentrypoint.h"

#include "src/greaderserviceroot.h"
#include "src/gui/formeditgreaderaccount.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

GreaderEntryPoint::GreaderEntryPoint(QObject* parent) : QObject(parent) {}

GreaderEntryPoint::~GreaderEntryPoint() {
  qDebugNN << LOGSEC_GREADER << "Destructing" << QUOTE_W_SPACE(QSL(SERVICE_CODE_GREADER)) << "plugin.";
}

ServiceRoot* GreaderEntryPoint::createNewRoot() const {
  FormEditGreaderAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<GreaderServiceRoot>();
}

QList<ServiceRoot*> GreaderEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->driver()->connection(QSL("GreaderEntryPoint"));

  return DatabaseQueries::getAccounts<GreaderServiceRoot>(database, code());
}

QString GreaderEntryPoint::name() const {
  return QSL("Google Reader API");
}

QString GreaderEntryPoint::code() const {
  return QSL(SERVICE_CODE_GREADER);
}

QString GreaderEntryPoint::description() const {
  return QObject::tr("Google Reader API is used by many online RSS readers.\n\nList of supported readers:") +
         QSL(" Inoreader, FreshRSS, Bazqux, TheOldReader, Reedah ") + QObject::tr("and possibly others.");
}

QString GreaderEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon GreaderEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("google"));
}
