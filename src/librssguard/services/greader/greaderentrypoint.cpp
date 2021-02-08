// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/greaderentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/greader/definitions.h"
#include "services/greader/greaderserviceroot.h"
#include "services/greader/gui/formeditgreaderaccount.h"

ServiceRoot* GreaderEntryPoint::createNewRoot() const {
  FormEditGreaderAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<GreaderServiceRoot>();
}

QList<ServiceRoot*> GreaderEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->connection(QSL("GreaderEntryPoint"));

  return DatabaseQueries::getGreaderAccounts(database);
}

QString GreaderEntryPoint::name() const {
  return QSL("Google Reader API");
}

QString GreaderEntryPoint::code() const {
  return SERVICE_CODE_GREADER;
}

QString GreaderEntryPoint::description() const {
  return QObject::tr("Google Reader API is used by many online RSS readers. This is here to support") +
         QSL(" FreshRSS, Bazqux, TheOldReader, Reedah, ...");
}

QString GreaderEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon GreaderEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("google"));
}
