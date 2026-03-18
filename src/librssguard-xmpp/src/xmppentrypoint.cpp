// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppentrypoint.h"

#include "src/xmppserviceroot.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

XmppEntryPoint::XmppEntryPoint(QObject* parent) : QObject(parent) {}

XmppEntryPoint::~XmppEntryPoint() {
  qDebugNN << LOGSEC_XMPP << "Destructing" << QUOTE_W_SPACE(QSL(SERVICE_CODE_XMPP)) << "plugin.";
}

ServiceRoot* XmppEntryPoint::createNewRoot() const {
  // FormEditXmppAccount form_acc(qApp->mainFormWidget());
  // return form_acc.addEditAccount<XmppServiceRoot>();

  return new XmppServiceRoot();
}

QList<ServiceRoot*> XmppEntryPoint::initializeSubtree() const {
  return qApp->database()->worker()->read<QList<ServiceRoot*>>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getAccounts<XmppServiceRoot>(db, code());
  });
}

QString XmppEntryPoint::name() const {
  return QSL("XMPP (PubSub)");
}

QString XmppEntryPoint::code() const {
  return QSL(SERVICE_CODE_XMPP);
}

QString XmppEntryPoint::description() const {
  return QObject::tr("Plugin for XMPP which is able to get articles via PubSub real-time push notifications.");
}

QString XmppEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon XmppEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("xmpp"));
}
