// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gmailentrypoint.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailserviceroot.h"
#include "services/gmail/gui/formeditgmailaccount.h"

#include <QMessageBox>

ServiceRoot* GmailEntryPoint::createNewRoot() const {
  FormEditGmailAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<GmailServiceRoot>();
}

QList<ServiceRoot*> GmailEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->driver()->connection(QSL("GmailEntryPoint"));

  return DatabaseQueries::getAccounts<GmailServiceRoot>(database, code());
}

QString GmailEntryPoint::name() const {
  return QSL("Gmail");
}

QString GmailEntryPoint::code() const {
  return QSL(SERVICE_CODE_GMAIL);
}

QString GmailEntryPoint::description() const {
  return QObject::tr("Simple Gmail integration via JSON API. Allows sending e-mails too.");
}

QString GmailEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon GmailEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("gmail"));
}
