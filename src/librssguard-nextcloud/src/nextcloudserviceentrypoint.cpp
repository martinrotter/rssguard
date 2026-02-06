// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/nextcloudserviceentrypoint.h"

#include "src/definitions.h"
#include "src/gui/formeditnextcloudaccount.h"
#include "src/nextcloudserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

NextcloudServiceEntryPoint::NextcloudServiceEntryPoint(QObject* parent) : QObject(parent) {}

NextcloudServiceEntryPoint::~NextcloudServiceEntryPoint() {
  qDebugNN << LOGSEC_NEXTCLOUD << "Destructing" << QUOTE_W_SPACE(QSL(SERVICE_CODE_NEXTCLOUD)) << "plugin.";
}

ServiceRoot* NextcloudServiceEntryPoint::createNewRoot() const {
  FormEditNextcloudAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<NextcloudServiceRoot>();
}

QList<ServiceRoot*> NextcloudServiceEntryPoint::initializeSubtree() const {
  return qApp->database()->worker()->read<QList<ServiceRoot*>>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getAccounts<NextcloudServiceRoot>(db, code());
  });
}

QString NextcloudServiceEntryPoint::name() const {
  return QSL("Nextcloud News");
}

QString NextcloudServiceEntryPoint::code() const {
  return QSL(SERVICE_CODE_NEXTCLOUD);
}

QString NextcloudServiceEntryPoint::description() const {
  return QObject::tr("The News app is an RSS/Atom feed aggregator. "
                     "It is part of Nextcloud suite. This plugin implements %1 API.")
    .arg(QSL(NEXTCLOUD_API_VERSION));
}

QString NextcloudServiceEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon NextcloudServiceEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("nextcloud"));
}
