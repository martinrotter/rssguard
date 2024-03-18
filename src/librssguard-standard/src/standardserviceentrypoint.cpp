// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/standardserviceentrypoint.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include "src/gui/formeditstandardaccount.h"
#include "src/standardserviceroot.h"

StandardServiceEntryPoint::StandardServiceEntryPoint(QObject* parent) : QObject(parent) {}

QString StandardServiceEntryPoint::name() const {
  return QSL("RSS/RDF/ATOM/JSON");
}

QString StandardServiceEntryPoint::description() const {
  return QObject::tr("This service offers integration with standard online RSS/RDF/ATOM/JSON feeds and podcasts.");
}

QString StandardServiceEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon StandardServiceEntryPoint::icon() const {
  return qApp->icons()->fromTheme(QSL("application-rss+xml"));
}

QString StandardServiceEntryPoint::code() const {
  return QSL(SERVICE_CODE_STD_RSS);
}

ServiceRoot* StandardServiceEntryPoint::createNewRoot() const {
  FormEditStandardAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<StandardServiceRoot>();
}

QList<ServiceRoot*> StandardServiceEntryPoint::initializeSubtree() const {
  // Check DB if standard account is enabled.
  QSqlDatabase database = qApp->database()->driver()->connection(QSL("StandardServiceEntryPoint"));

  return DatabaseQueries::getAccounts<StandardServiceRoot>(database, code());
}
