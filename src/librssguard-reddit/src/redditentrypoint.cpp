// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/redditentrypoint.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "src/gui/formeditredditaccount.h"
#include "src/redditserviceroot.h"

#include <QMessageBox>

RedditEntryPoint::RedditEntryPoint(QObject* parent) : QObject(parent) {}

RedditEntryPoint::~RedditEntryPoint() {
  qDebugNN << LOGSEC_REDDIT << "Destructing" << QUOTE_W_SPACE(QSL(SERVICE_CODE_REDDIT)) << "plugin.";
}

ServiceRoot* RedditEntryPoint::createNewRoot() const {
  FormEditRedditAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<RedditServiceRoot>();
}

QList<ServiceRoot*> RedditEntryPoint::initializeSubtree() const {
  return qApp->database()->worker()->read<QList<ServiceRoot*>>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getAccounts<RedditServiceRoot>(db, code());
  });
}

QString RedditEntryPoint::name() const {
  return QSL("Reddit");
}

QString RedditEntryPoint::code() const {
  return QSL(SERVICE_CODE_REDDIT);
}

QString RedditEntryPoint::description() const {
  return QObject::tr("Simplistic Reddit client.");
}

QString RedditEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon RedditEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("reddit"));
}
