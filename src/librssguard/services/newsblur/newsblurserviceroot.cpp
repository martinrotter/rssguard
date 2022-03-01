// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/newsblur/newsblurserviceroot.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "exceptions/feedfetchexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/textfactory.h"
#include "network-web/oauth2service.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/recyclebin.h"
#include "services/newsblur/definitions.h"
#include "services/newsblur/gui/formeditnewsbluraccount.h"
#include "services/newsblur/newsblurentrypoint.h"
#include "services/newsblur/newsblurnetwork.h"

NewsBlurServiceRoot::NewsBlurServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new NewsBlurNetwork(this)) {
  m_network->setRoot(this);
  setIcon(NewsBlurEntryPoint().icon());
}

bool NewsBlurServiceRoot::isSyncable() const {
  return true;
}

bool NewsBlurServiceRoot::canBeEdited() const {
  return true;
}

bool NewsBlurServiceRoot::editViaGui() {
  FormEditNewsBlurAccount form_pointer(qApp->mainFormWidget());

  form_pointer.addEditAccount(this);
  return true;
}

QVariantHash NewsBlurServiceRoot::customDatabaseData() const {
  QVariantHash data;

  data[QSL("username")] = m_network->username();
  data[QSL("password")] = TextFactory::encrypt(m_network->password());
  data[QSL("url")] = m_network->baseUrl();

  return data;
}

void NewsBlurServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  m_network->setUsername(data[QSL("username")].toString());
  m_network->setPassword(TextFactory::decrypt(data[QSL("password")].toString()));
  m_network->setBaseUrl(data[QSL("url")].toString());
}

QList<Message> NewsBlurServiceRoot::obtainNewMessages(Feed* feed,
                                                      const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                                      const QHash<QString, QStringList>& tagged_messages) {
  Feed::Status error = Feed::Status::Normal;
  QList<Message> msgs;

  // TODO::

  if (error != Feed::Status::NewMessages && error != Feed::Status::Normal) {
    throw FeedFetchException(error);
  }
  else {
    return msgs;
  }
}

void NewsBlurServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadRootFromDatabase<Category, Feed>(this);
    loadCacheFromFile();
  }

  updateTitleIcon();

  if (getSubTreeFeeds().isEmpty()) {
    syncIn();
  }
}

QString NewsBlurServiceRoot::code() const {
  return NewsBlurEntryPoint().code();
}

void NewsBlurServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

  /*
     // Save the actual data read/unread.
     while (i.hasNext()) {
     i.next();
     auto key = i.key();
     QStringList ids = i.value();

     if (!ids.isEmpty()) {
      if (network()->markMessagesRead(key, ids, networkProxy()) != QNetworkReply::NetworkError::NoError &&
          !ignore_errors) {
        addMessageStatesToCache(ids, key);
      }
     }
     }

     QMapIterator<RootItem::Importance, QList<Message>> j(msg_cache.m_cachedStatesImportant);

     // Save the actual data important/not important.
     while (j.hasNext()) {
     j.next();
     auto key = j.key();
     QList<Message> messages = j.value();

     if (!messages.isEmpty()) {
      QStringList custom_ids; custom_ids.reserve(messages.size());

      for (const Message& msg : messages) {
        custom_ids.append(msg.m_customId);
      }

      if (network()->markMessagesStarred(key, custom_ids, networkProxy()) != QNetworkReply::NetworkError::NoError &&
          !ignore_errors) {
        addMessageStatesToCache(messages, key);
      }
     }
     }
   */
}

ServiceRoot::LabelOperation NewsBlurServiceRoot::supportedLabelOperations() const {
  return ServiceRoot::LabelOperation::Synchronised;
}

void NewsBlurServiceRoot::updateTitleIcon() {
  setTitle(QSL("%1 (%2)").arg(m_network->username(), NewsBlurEntryPoint().name()));
}

RootItem* NewsBlurServiceRoot::obtainNewTreeForSyncIn() const {
  return m_network->categoriesFeedsLabelsTree(networkProxy());;
}
