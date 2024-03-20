// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gmailentrypoint.h"

#include "src/gmailserviceroot.h"
#include "src/gui/formeditgmailaccount.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

#include <QMessageBox>

GmailEntryPoint::GmailEntryPoint(QObject* parent) : QObject(parent) {}

GmailEntryPoint::~GmailEntryPoint() {
  qDebugNN << LOGSEC_GMAIL << "Destructing" << QUOTE_W_SPACE(QSL(SERVICE_CODE_GMAIL)) << "plugin.";
}

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
