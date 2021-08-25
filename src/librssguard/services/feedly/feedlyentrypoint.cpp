// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/feedlyentrypoint.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/feedly/definitions.h"
#include "services/feedly/feedlyserviceroot.h"
#include "services/feedly/gui/formeditfeedlyaccount.h"

ServiceRoot* FeedlyEntryPoint::createNewRoot() const {
  FormEditFeedlyAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<FeedlyServiceRoot>();
}

QList<ServiceRoot*> FeedlyEntryPoint::initializeSubtree() const {
  QSqlDatabase database = qApp->database()->driver()->connection(QSL("FeedlyEntryPoint"));

  return DatabaseQueries::getAccounts<FeedlyServiceRoot>(database, code());
}

QString FeedlyEntryPoint::name() const {
  return QSL("Feedly");
}

QString FeedlyEntryPoint::code() const {
  return SERVICE_CODE_FEEDLY;
}

QString FeedlyEntryPoint::description() const {
  return QObject::tr("Keep up with the topics and trends you care about, without the overwhelm.\n\n"
                     "Feedly is a secure space where you can privately organize and research the "
                     "topics and trends that matter to you.");
}

QString FeedlyEntryPoint::author() const {
  return APP_AUTHOR;
}

QIcon FeedlyEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("feedly"));
}
