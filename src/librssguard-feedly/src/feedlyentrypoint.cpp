// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/feedlyentrypoint.h"

#include "src/feedlyserviceroot.h"
#include "src/gui/formeditfeedlyaccount.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>

FeedlyEntryPoint::FeedlyEntryPoint(QObject* parent) : QObject(parent) {}

FeedlyEntryPoint::~FeedlyEntryPoint() {
  qDebugNN << LOGSEC_FEEDLY << "Destructing" << QUOTE_W_SPACE(QSL(SERVICE_CODE_FEEDLY)) << "plugin.";
}

ServiceRoot* FeedlyEntryPoint::createNewRoot() const {
  FormEditFeedlyAccount form_acc(qApp->mainFormWidget());

  return form_acc.addEditAccount<FeedlyServiceRoot>();
}

QList<ServiceRoot*> FeedlyEntryPoint::initializeSubtree() const {
  auto acc = qApp->database()->worker()->read<QList<ServiceRoot*>>([&](const QSqlDatabase& db) {
    return DatabaseQueries::getAccounts<FeedlyServiceRoot>(db, code());
  });

  return acc;
}

QString FeedlyEntryPoint::name() const {
  return QSL("Feedly");
}

QString FeedlyEntryPoint::code() const {
  return QSL(SERVICE_CODE_FEEDLY);
}

QString FeedlyEntryPoint::description() const {
  return QObject::tr("Keep up with the topics and trends you care about, without the overwhelm.\n\n"
                     "Feedly is a secure space where you can privately organize and research the "
                     "topics and trends that matter to you.");
}

QString FeedlyEntryPoint::author() const {
  return QSL(APP_AUTHOR);
}

QIcon FeedlyEntryPoint::icon() const {
  return qApp->icons()->miscIcon(QSL("feedly"));
}
