// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/newsblur/newsblurentrypoint.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/newsblur/gui/formeditnewsbluraccount.h"
#include "services/newsblur/newsblurserviceroot.h"

ServiceRoot* NewsBlurEntryPoint::createNewRoot() const {
  FormEditNewsBlurAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<NewsBlurServiceRoot>();
}

QList<ServiceRoot*> NewsBlurEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->driver()->connection(QSL("NewsBlurEntryPoint"));

  return DatabaseQueries::getAccounts<NewsBlurServiceRoot>(database, code());
}

QString NewsBlurEntryPoint::name() const {
  return QSL("NewsBlur");
}

QString NewsBlurEntryPoint::code() const {
  return QSL(SERVICE_CODE_NEWSBLUR);
}

QString NewsBlurEntryPoint::description() const {
  return QObject::tr("Personal news reader bringing people together to talk about the world.");
}

QString NewsBlurEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon NewsBlurEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("newsblur"));
}
