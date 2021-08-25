// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/greaderserviceroot.h"

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
#include "services/greader/definitions.h"
#include "services/greader/greaderentrypoint.h"
#include "services/greader/greadernetwork.h"
#include "services/greader/gui/formeditgreaderaccount.h"

GreaderServiceRoot::GreaderServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new GreaderNetwork(this)) {
  setIcon(GreaderEntryPoint().icon());
  m_network->setRoot(this);
}

bool GreaderServiceRoot::isSyncable() const {
  return true;
}

bool GreaderServiceRoot::canBeEdited() const {
  return true;
}

bool GreaderServiceRoot::editViaGui() {
  FormEditGreaderAccount form_pointer(qApp->mainFormWidget());

  form_pointer.addEditAccount(this);
  return true;
}

QVariantHash GreaderServiceRoot::customDatabaseData() const {
  QVariantHash data;

  data["service"] = int(m_network->service());
  data["username"] = m_network->username();
  data["password"] = TextFactory::encrypt(m_network->password());
  data["batch_size"] = m_network->batchSize();
  data["download_only_unread"] = m_network->downloadOnlyUnreadMessages();
  data["intelligent_synchronization"] = m_network->intelligentSynchronization();

  if (m_network->newerThanFilter().isValid()) {
    data["fetch_newer_than"] = m_network->newerThanFilter();
  }

  if (m_network->service() == Service::Inoreader) {
    data["client_id"] = m_network->oauth()->clientId();
    data["client_secret"] = m_network->oauth()->clientSecret();
    data["refresh_token"] = m_network->oauth()->refreshToken();
    data["redirect_uri"] = m_network->oauth()->redirectUrl();
  }
  else {
    data["url"] = m_network->baseUrl();
  }

  return data;
}

void GreaderServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  m_network->setService(GreaderServiceRoot::Service(data["service"].toInt()));
  m_network->setUsername(data["username"].toString());
  m_network->setPassword(TextFactory::decrypt(data["password"].toString()));
  m_network->setBatchSize(data["batch_size"].toInt());
  m_network->setDownloadOnlyUnreadMessages(data["download_only_unread"].toBool());
  m_network->setIntelligentSynchronization(data["intelligent_synchronization"].toBool());

  if (data["fetch_newer_than"].toDate().isValid()) {
    m_network->setNewerThanFilter(data["fetch_newer_than"].toDate());
  }

  if (m_network->service() == Service::Inoreader) {
    m_network->oauth()->setClientId(data["client_id"].toString());
    m_network->oauth()->setClientSecret(data["client_secret"].toString());
    m_network->oauth()->setRefreshToken(data["refresh_token"].toString());
    m_network->oauth()->setRedirectUrl(data["redirect_uri"].toString(), true);

    m_network->setBaseUrl(QSL(GREADER_URL_INOREADER));
  }
  else {
    m_network->setBaseUrl(data["url"].toString());
  }
}

void GreaderServiceRoot::aboutToBeginFeedFetching(const QList<Feed*>& feeds,
                                                  const QHash<QString, QHash<BagOfMessages, QStringList>>& stated_messages,
                                                  const QHash<QString, QStringList>& tagged_messages) {
  if (m_network->intelligentSynchronization()) {
    m_network->prepareFeedFetching(this, feeds, stated_messages, tagged_messages, networkProxy());
  }
  else {
    m_network->clearPrefetchedMessages();
  }
}

QString GreaderServiceRoot::serviceToString(Service service) {
  switch (service) {
    case Service::FreshRss:
      return QSL("FreshRSS");

    case Service::Bazqux:
      return QSL("Bazqux");

    case Service::Reedah:
      return QSL("Reedah");

    case Service::TheOldReader:
      return QSL("The Old Reader");

    case Service::Inoreader:
      return QSL("Inoreader");

    default:
      return tr("Other services");
  }
}

QList<Message> GreaderServiceRoot::obtainNewMessages(Feed* feed,
                                                     const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                                     const QHash<QString, QStringList>& tagged_messages) {
  Feed::Status error = Feed::Status::Normal;
  QList<Message> msgs;

  if (m_network->intelligentSynchronization()) {
    msgs = m_network->getMessagesIntelligently(this,
                                               feed->customId(),
                                               stated_messages,
                                               tagged_messages,
                                               error,
                                               networkProxy());
  }
  else {
    msgs = m_network->streamContents(this, feed->customId(), error, networkProxy());
  }

  if (error != Feed::Status::NewMessages && error != Feed::Status::Normal) {
    throw FeedFetchException(error);
  }
  else {
    return msgs;
  }
}

bool GreaderServiceRoot::wantsBaggedIdsOfExistingMessages() const {
  return m_network->intelligentSynchronization();
}

void GreaderServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadFromDatabase<Category, Feed>(this);
    loadCacheFromFile();
  }

  updateTitleIcon();

  if (getSubTreeFeeds().isEmpty()) {
    if (m_network->service() == Service::Inoreader) {
      m_network->oauth()->login([this]() {
        syncIn();
      });
    }
    else {
      syncIn();
    }
  }
  else if (m_network->service() == Service::Inoreader) {
    m_network->oauth()->login();
  }
}

QString GreaderServiceRoot::code() const {
  return GreaderEntryPoint().code();
}

void GreaderServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

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
      QStringList custom_ids;

      for (const Message& msg : messages) {
        custom_ids.append(msg.m_customId);
      }

      if (network()->markMessagesStarred(key, custom_ids, networkProxy()) != QNetworkReply::NetworkError::NoError &&
          !ignore_errors) {
        addMessageStatesToCache(messages, key);
      }
    }
  }

  if (m_network->service() != Service::TheOldReader) {
    // NOTE: The Old Reader does not support labels.
    QMapIterator<QString, QStringList> k(msg_cache.m_cachedLabelAssignments);

    // Assign label for these messages.
    while (k.hasNext()) {
      k.next();
      auto label_custom_id = k.key();
      QStringList messages = k.value();

      if (!messages.isEmpty()) {
        if (network()->editLabels(label_custom_id, true, messages, networkProxy()) != QNetworkReply::NetworkError::NoError &&
            !ignore_errors) {
          addLabelsAssignmentsToCache(messages, label_custom_id, true);
        }
      }
    }

    QMapIterator<QString, QStringList> l(msg_cache.m_cachedLabelDeassignments);

    // Remove label from these messages.
    while (l.hasNext()) {
      l.next();
      auto label_custom_id = l.key();
      QStringList messages = l.value();

      if (!messages.isEmpty()) {
        if (network()->editLabels(label_custom_id, false, messages, networkProxy()) != QNetworkReply::NetworkError::NoError &&
            !ignore_errors) {
          addLabelsAssignmentsToCache(messages, label_custom_id, false);
        }
      }
    }
  }
}

ServiceRoot::LabelOperation GreaderServiceRoot::supportedLabelOperations() const {
  return ServiceRoot::LabelOperation::Synchronised;
}

void GreaderServiceRoot::updateTitleIcon() {
  setTitle(QString("%1 (%2)").arg(TextFactory::extractUsernameFromEmail(m_network->username()),
                                  GreaderServiceRoot::serviceToString(m_network->service())));

  switch (m_network->service()) {
    case Service::TheOldReader:
      setIcon(qApp->icons()->miscIcon(QSL("theoldreader")));
      break;

    case Service::FreshRss:
      setIcon(qApp->icons()->miscIcon(QSL("freshrss")));
      break;

    case Service::Bazqux:
      setIcon(qApp->icons()->miscIcon(QSL("bazqux")));
      break;

    case Service::Reedah:
      setIcon(qApp->icons()->miscIcon(QSL("reedah")));
      break;

    case Service::Inoreader:
      setIcon(qApp->icons()->miscIcon(QSL("inoreader")));
      break;

    default:
      setIcon(GreaderEntryPoint().icon());
      break;
  }
}

RootItem* GreaderServiceRoot::obtainNewTreeForSyncIn() const {
  return m_network->categoriesFeedsLabelsTree(true, networkProxy());
}
