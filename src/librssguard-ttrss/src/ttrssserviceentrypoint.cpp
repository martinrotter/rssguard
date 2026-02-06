// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/ttrssserviceentrypoint.h"

#include "src/definitions.h"
#include "src/gui/formeditttrssaccount.h"
#include "src/ttrssserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/iconfactory.h>

TtRssServiceEntryPoint::TtRssServiceEntryPoint(QObject* parent) : QObject(parent) {}

TtRssServiceEntryPoint::~TtRssServiceEntryPoint() {
  qDebugNN << LOGSEC_TTRSS << "Destructing" << QUOTE_W_SPACE(QSL(SERVICE_CODE_TT_RSS)) << "plugin.";
}

QString TtRssServiceEntryPoint::name() const {
  return QSL("Tiny Tiny RSS");
}

QString TtRssServiceEntryPoint::description() const {
  return QObject::tr("This service offers integration with Tiny Tiny RSS.\n\n"
                     "Tiny Tiny RSS is an open source web-based news feed (RSS/Atom) reader and aggregator, "
                     "designed to allow you to read news from any location, while feeling as close to a real "
                     "desktop application as possible.\n\nAt least API level %1 is required.")
    .arg(TTRSS_MINIMAL_API_LEVEL);
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
  return qApp->database()->worker()->read<QList<ServiceRoot*>>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getAccounts<TtRssServiceRoot>(db, code());
  });
}
