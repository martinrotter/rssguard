// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/owncloudfeed.h"

#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "services/owncloud/gui/formowncloudfeeddetails.h"
#include "services/owncloud/network/owncloudnetworkfactory.h"
#include "services/owncloud/owncloudserviceroot.h"

#include <QPointer>

OwnCloudFeed::OwnCloudFeed(RootItem* parent) : Feed(parent) {}

OwnCloudFeed::OwnCloudFeed(const QSqlRecord& record) : Feed(record) {}

OwnCloudFeed::~OwnCloudFeed() = default;

bool OwnCloudFeed::canBeEdited() const {
  return true;
}

bool OwnCloudFeed::editViaGui() {
  QPointer<FormOwnCloudFeedDetails> form_pointer = new FormOwnCloudFeedDetails(serviceRoot(), qApp->mainFormWidget());
  form_pointer.data()->addEditFeed(this, nullptr);
  delete form_pointer.data();
  return false;
}

bool OwnCloudFeed::canBeDeleted() const {
  return true;
}

bool OwnCloudFeed::deleteViaGui() {
  if (serviceRoot()->network()->deleteFeed(customId()) && removeItself()) {
    serviceRoot()->requestItemRemoval(this);
    return true;
  }
  else {
    return false;
  }
}

bool OwnCloudFeed::editItself(OwnCloudFeed* new_feed_data) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (!DatabaseQueries::editBaseFeed(database, id(), new_feed_data->autoUpdateType(),
                                     new_feed_data->autoUpdateInitialInterval())) {
    // Persistent storage update failed, no way to continue now.
    return false;
  }
  else {
    setAutoUpdateType(new_feed_data->autoUpdateType());
    setAutoUpdateInitialInterval(new_feed_data->autoUpdateInitialInterval());
    return true;
  }
}

bool OwnCloudFeed::removeItself() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  return DatabaseQueries::deleteFeed(database, customId().toInt(), serviceRoot()->accountId());
}

OwnCloudServiceRoot* OwnCloudFeed::serviceRoot() const {
  return qobject_cast<OwnCloudServiceRoot*>(getParentServiceRoot());
}

QList<Message> OwnCloudFeed::obtainNewMessages(bool* error_during_obtaining) {
  OwnCloudGetMessagesResponse messages = serviceRoot()->network()->getMessages(customNumericId());

  if (serviceRoot()->network()->lastError() != QNetworkReply::NoError) {
    setStatus(Feed::NetworkError);
    *error_during_obtaining = true;
    serviceRoot()->itemChanged(QList<RootItem*>() << this);
    return QList<Message>();
  }
  else {
    *error_during_obtaining = false;
    return messages.messages();
  }

  return QList<Message>();
}
