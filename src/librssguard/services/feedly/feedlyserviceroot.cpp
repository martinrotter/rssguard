// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/feedlyserviceroot.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/recyclebin.h"
#include "services/feedly/feedlyentrypoint.h"
#include "services/feedly/feedlyfeed.h"
#include "services/feedly/feedlynetwork.h"
#include "services/feedly/gui/formeditfeedlyaccount.h"

#if defined (FEEDLY_OFFICIAL_SUPPORT)
#include "network-web/oauth2service.h"
#endif

FeedlyServiceRoot::FeedlyServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new FeedlyNetwork(this)) {
  setIcon(FeedlyEntryPoint().icon());
  m_network->setService(this);
}

bool FeedlyServiceRoot::isSyncable() const {
  return true;
}

bool FeedlyServiceRoot::canBeEdited() const {
  return true;
}

bool FeedlyServiceRoot::canBeDeleted() const {
  return true;
}

bool FeedlyServiceRoot::editViaGui() {
  FormEditFeedlyAccount form_pointer(qApp->mainFormWidget());

  form_pointer.addEditAccount(this);
  return true;
}

bool FeedlyServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::deleteGreaderAccount(database, accountId())) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}

void FeedlyServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)
  loadFromDatabase();
  loadCacheFromFile();

  if (childCount() <= 3) {
    syncIn();
  }
}

QString FeedlyServiceRoot::code() const {
  return FeedlyEntryPoint().code();
}

void FeedlyServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
}

void FeedlyServiceRoot::updateTitle() {
  setTitle(QString("%1 (Feedly)").arg(TextFactory::extractUsernameFromEmail(m_network->username())));
}

void FeedlyServiceRoot::saveAccountDataToDatabase(bool creating_new) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (!creating_new) {
    if (DatabaseQueries::overwriteFeedlyAccount(database,
                                                m_network->username(),
                                                m_network->developerAccessToken(),
#if defined (FEEDLY_OFFICIAL_SUPPORT)
                                                m_network->oauth()->refreshToken(),
#else
                                                {},
#endif
                                                m_network->batchSize(),
                                                accountId())) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    if (DatabaseQueries::createFeedlyAccount(database,
                                             m_network->username(),
                                             m_network->developerAccessToken(),
#if defined (FEEDLY_OFFICIAL_SUPPORT)
                                             m_network->oauth()->refreshToken(),
#else
                                             {},
#endif
                                             m_network->batchSize(),
                                             accountId())) {
      updateTitle();
    }
  }
}

RootItem* FeedlyServiceRoot::obtainNewTreeForSyncIn() const {
  return nullptr;

  //return m_network->categoriesFeedsLabelsTree(true, networkProxy());
}

void FeedlyServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories<Category>(database, accountId());
  Assignment feeds = DatabaseQueries::getFeeds<FeedlyFeed>(database, qApp->feedReader()->messageFilters(), accountId());
  auto labels = DatabaseQueries::getLabels(database, accountId());

  performInitialAssembly(categories, feeds, labels);
}
