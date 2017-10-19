// For license of this file, see <object-root-folder>/LICENSE.md.

#include "services/inoreader/inoreaderentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/inoreader/definitions.h"
#include "services/inoreader/gui/formeditinoreaderaccount.h"
#include "services/inoreader/inoreaderserviceroot.h"
#include "services/inoreader/network/inoreadernetworkfactory.h"

#include <QMessageBox>

ServiceRoot* InoreaderEntryPoint::createNewRoot() const {
#if defined(USE_WEBENGINE)
  FormEditInoreaderAccount form_acc(qApp->mainFormWidget());

  return form_acc.execForCreate();
#else
  QMessageBox::warning(qApp->mainFormWidget(),
                       QObject::tr("Not supported"),
                       QObject::tr("This plugin is not supported in NonWebEngine variant of this program."));
  return nullptr;
#endif
}

QList<ServiceRoot*> InoreaderEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->connection(QSL("InoreaderEntryPoint"), DatabaseFactory::FromSettings);

  return DatabaseQueries::getInoreaderAccounts(database);
}

bool InoreaderEntryPoint::isSingleInstanceService() const {
  return false;
}

QString InoreaderEntryPoint::name() const {
  return QSL("Inoreader");
}

QString InoreaderEntryPoint::code() const {
  return SERVICE_CODE_INOREADER;
}

QString InoreaderEntryPoint::description() const {
  return QObject::tr("This is integration of Inoreader.");
}

QString InoreaderEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon InoreaderEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("inoreader"));
}
