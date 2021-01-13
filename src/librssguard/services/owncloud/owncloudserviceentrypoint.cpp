// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/owncloudserviceentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/owncloud/definitions.h"
#include "services/owncloud/gui/formeditowncloudaccount.h"
#include "services/owncloud/owncloudserviceroot.h"

ServiceRoot* OwnCloudServiceEntryPoint::createNewRoot() const {
  FormEditOwnCloudAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<OwnCloudServiceRoot>();
}

QList<ServiceRoot*> OwnCloudServiceEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->connection(QSL("OwnCloudServiceEntryPoint"));

  return DatabaseQueries::getOwnCloudAccounts(database);
}

QString OwnCloudServiceEntryPoint::name() const {
  return QSL("Nextcloud News");
}

QString OwnCloudServiceEntryPoint::code() const {
  return SERVICE_CODE_OWNCLOUD;
}

QString OwnCloudServiceEntryPoint::description() const {
  return QObject::tr("The News app is an RSS/Atom feed aggregator. "
                     "It is part of Nextcloud suite. This plugin implements %1 API.").arg(OWNCLOUD_API_VERSION);
}

QString OwnCloudServiceEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon OwnCloudServiceEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("nextcloud"));
}
