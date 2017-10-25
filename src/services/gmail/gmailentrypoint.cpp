// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gmailentrypoint.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailserviceroot.h"
#include "services/gmail/gui/formeditgmailaccount.h"

#include <QMessageBox>

ServiceRoot* GmailEntryPoint::createNewRoot() const {
#if defined(USE_WEBENGINE)
  FormEditGmailAccount form_acc(qApp->mainFormWidget());

  return form_acc.execForCreate();
#else
  QMessageBox::warning(qApp->mainFormWidget(),
                       QObject::tr("Not supported"),
                       QObject::tr("This plugin is not supported in NonWebEngine variant of this program."));
  return nullptr;
#endif
}

QList<ServiceRoot*> GmailEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->connection(QSL("GmailEntryPoint"), DatabaseFactory::FromSettings);

  return DatabaseQueries::getGmailAccounts(database);
}

bool GmailEntryPoint::isSingleInstanceService() const {
  return false;
}

QString GmailEntryPoint::name() const {
  return QSL("Gmail");
}

QString GmailEntryPoint::code() const {
  return SERVICE_CODE_GMAIL;
}

QString GmailEntryPoint::description() const {
  return QObject::tr("Simple Gmail integration via JSON API. Allows sending e-mails too.");
}

QString GmailEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon GmailEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("gmail"));
}
