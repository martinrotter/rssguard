// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/greaderentrypoint.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/greader/definitions.h"
#include "services/greader/greaderserviceroot.h"
#include "services/greader/gui/formeditgreaderaccount.h"

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
         QSL(" Inoreader, FreshRSS, Bazqux, TheOldReader, Reedah and possibly others.");
}

QString GreaderEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon GreaderEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("google"));
}
