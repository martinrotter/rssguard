// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/reddit/redditentrypoint.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/reddit/gui/formeditredditaccount.h"
#include "services/reddit/redditserviceroot.h"

#include <QMessageBox>

ServiceRoot* RedditEntryPoint::createNewRoot() const {
  FormEditRedditAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<RedditServiceRoot>();
}

QList<ServiceRoot*> RedditEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->driver()->connection(QSL("RedditEntryPoint"));

  return DatabaseQueries::getAccounts<RedditServiceRoot>(database, code());
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
