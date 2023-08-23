// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/ttrssserviceentrypoint.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/iconfactory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/formeditttrssaccount.h"
#include "services/tt-rss/ttrssserviceroot.h"

QString TtRssServiceEntryPoint::name() const {
  return QSL("Tiny Tiny RSS");
}

QString TtRssServiceEntryPoint::description() const {
  return QObject::tr("This service offers integration with Tiny Tiny RSS.\n\n"
                     "Tiny Tiny RSS is an open source web-based news feed (RSS/Atom) reader and aggregator, "
                     "designed to allow you to read news from any location, while feeling as close to a real "
                     "desktop application as possible.\n\nAt least API level %1 is required.").arg(TTRSS_MINIMAL_API_LEVEL);
}

QString TtRssServiceEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon TtRssServiceEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("tt-rss"));
}

QString TtRssServiceEntryPoint::code() const {
  return QSL(SERVICE_CODE_TT_RSS);
}

ServiceRoot* TtRssServiceEntryPoint::createNewRoot() const {
  FormEditTtRssAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<TtRssServiceRoot>();
}

QList<ServiceRoot*> TtRssServiceEntryPoint::initializeSubtree() const {
  // Check DB if standard account is enabled.
  QSqlDatabase database = qApp->database()->driver()->connection(QSL("TtRssServiceEntryPoint"));

  return DatabaseQueries::getAccounts<TtRssServiceRoot>(database, code());
}
