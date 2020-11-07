// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/standardserviceentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "services/standard/standardserviceroot.h"

bool StandardServiceEntryPoint::isSingleInstanceService() const {
  return true;
}

QString StandardServiceEntryPoint::name() const {
  return QObject::tr("Standard online feeds (RSS/ATOM/JSON)");
}

QString StandardServiceEntryPoint::description() const {
  return QObject::tr("This service offers integration with standard online RSS/RDF/ATOM feeds and podcasts.");
}

QString StandardServiceEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon StandardServiceEntryPoint::icon() const {
  return QIcon(APP_ICON_PATH);
}

QString StandardServiceEntryPoint::code() const {
  return SERVICE_CODE_STD_RSS;
}

ServiceRoot* StandardServiceEntryPoint::createNewRoot() const {
  // Switch DB.
  QSqlDatabase database = qApp->database()->connection(QSL("StandardServiceEntryPoint"));
  bool ok;
  int new_id = DatabaseQueries::createAccount(database, code(), &ok);

  if (ok) {
    auto* root = new StandardServiceRoot();

    root->setAccountId(new_id);
    return root;
  }
  else {
    return nullptr;
  }
}

QList<ServiceRoot*> StandardServiceEntryPoint::initializeSubtree() const {
  // Check DB if standard account is enabled.
  QSqlDatabase database = qApp->database()->connection(QSL("StandardServiceEntryPoint"));

  return DatabaseQueries::getStandardAccounts(database);
}
