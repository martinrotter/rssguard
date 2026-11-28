// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/standardserviceentrypoint.h"

#include "src/definitions.h"
#include "src/gui/formeditstandardaccount.h"
#include "src/standardserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>

StandardServiceEntryPoint::StandardServiceEntryPoint(QObject* parent) : QObject(parent) {}

StandardServiceEntryPoint::~StandardServiceEntryPoint() {
  qDebugNN << LOGSEC_STANDARD << "Destructing" << QUOTE_W_SPACE(QSL(SERVICE_CODE_STD_RSS)) << "plugin.";
}

QString StandardServiceEntryPoint::name() const {
  return tr("Local RSS/RDF/ATOM/JSON");
}

QString StandardServiceEntryPoint::description() const {
  return QObject::
    tr("This service offers integration with standard online RSS/RDF/ATOM/JSON/Sitemap/iCalendar feeds and podcasts.");
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
  auto* gui_thread = qApp->thread();
  return qApp->database()->worker()->read<QList<ServiceRoot*>>([this, gui_thread](const QSqlDatabase& db) {
    const auto accounts = DatabaseQueries::getAccounts<StandardServiceRoot>(db, code());

    // The built-in nodes are logical children, so moving the account does not move them automatically.
    for (auto* account : accounts) {
      for (auto* item : account->getSubTree<RootItem>()) {
        item->moveToThread(gui_thread);
      }
    }

    return accounts;
  });
}
