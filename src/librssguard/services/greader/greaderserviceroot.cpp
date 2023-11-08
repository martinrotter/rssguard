// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/greaderserviceroot.h"

#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/textfactory.h"
#include "network-web/oauth2service.h"
#include "services/greader/definitions.h"
#include "services/greader/greaderentrypoint.h"
#include "services/greader/greadernetwork.h"
#include "services/greader/gui/formeditgreaderaccount.h"

GreaderServiceRoot::GreaderServiceRoot(RootItem* parent) : ServiceRoot(parent), m_network(new GreaderNetwork(this)) {
  setIcon(GreaderEntryPoint().icon());
  m_network->setRoot(this);
}

bool GreaderServiceRoot::isSyncable() const {
  return true;
}

bool GreaderServiceRoot::canBeEdited() const {
  return true;
}

FormAccountDetails* GreaderServiceRoot::accountSetupDialog() const {
  return new FormEditGreaderAccount(qApp->mainFormWidget());
}

void GreaderServiceRoot::editItems(const QList<RootItem*>& items) {
  if (items.first()->kind() == RootItem::Kind::ServiceRoot) {
    QScopedPointer<FormEditGreaderAccount> p(qobject_cast<FormEditGreaderAccount*>(accountSetupDialog()));

    p->addEditAccount(this);
    return;
  }

  ServiceRoot::editItems(items);
}

QVariantHash GreaderServiceRoot::customDatabaseData() const {
  QVariantHash data;

  data[QSL("service")] = int(m_network->service());
  data[QSL("username")] = m_network->username();
  data[QSL("password")] = TextFactory::encrypt(m_network->password());
  data[QSL("batch_size")] = m_network->batchSize();
  data[QSL("download_only_unread")] = m_network->downloadOnlyUnreadMessages();
  data[QSL("intelligent_synchronization")] = m_network->intelligentSynchronization();

  if (m_network->newerThanFilter().isValid()) {
    data[QSL("fetch_newer_than")] = m_network->newerThanFilter();
  }

  if (m_network->service() == Service::Inoreader) {
    data[QSL("client_id")] = m_network->oauth()->clientId();
    data[QSL("client_secret")] = m_network->oauth()->clientSecret();
    data[QSL("refresh_token")] = m_network->oauth()->refreshToken();
    data[QSL("redirect_uri")] = m_network->oauth()->redirectUrl();
  }
  else {
    data[QSL("url")] = m_network->baseUrl();
  }

  return data;
}

void GreaderServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  m_network->setService(GreaderServiceRoot::Service(data[QSL("service")].toInt()));
  m_network->setUsername(data[QSL("username")].toString());
  m_network->setPassword(TextFactory::decrypt(data[QSL("password")].toString()));
  m_network->setBatchSize(data[QSL("batch_size")].toInt());
  m_network->setDownloadOnlyUnreadMessages(data[QSL("download_only_unread")].toBool());
  m_network->setIntelligentSynchronization(data[QSL("intelligent_synchronization")].toBool());

  if (data[QSL("fetch_newer_than")].toDate().isValid()) {
    m_network->setNewerThanFilter(data[QSL("fetch_newer_than")].toDate());
  }

  if (m_network->service() == Service::Inoreader) {
    m_network->oauth()->setClientId(data[QSL("client_id")].toString());
    m_network->oauth()->setClientSecret(data[QSL("client_secret")].toString());
    m_network->oauth()->setRefreshToken(data[QSL("refresh_token")].toString());
    m_network->oauth()->setRedirectUrl(data[QSL("redirect_uri")].toString(), true);

    m_network->setBaseUrl(QSL(GREADER_URL_INOREADER));
  }
  else {
    m_network->setBaseUrl(data[QSL("url")].toString());
  }
}

void GreaderServiceRoot::aboutToBeginFeedFetching(const QList<Feed*>& feeds,
                                                  const QHash<QString, QHash<BagOfMessages, QStringList>>&
                                                    stated_messages,
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

    case Service::Miniflux:
      return QSL("Miniflux");

    default:
      return tr("Other services");
  }
}

QList<Message> GreaderServiceRoot::obtainNewMessages(Feed* feed,
                                                     const QHash<ServiceRoot::BagOfMessages, QStringList>&
                                                       stated_messages,
                                                     const QHash<QString, QStringList>& tagged_messages) {
  QList<Message> msgs;

  if (m_network->intelligentSynchronization()) {
    msgs =
      m_network->getMessagesIntelligently(this, feed->customId(), stated_messages, tagged_messages, networkProxy());
  }
  else {
    msgs = m_network->streamContents(this, feed->customId(), networkProxy());
  }

  return msgs;
}

bool GreaderServiceRoot::wantsBaggedIdsOfExistingMessages() const {
  return m_network->intelligentSynchronization();
}

void GreaderServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadRootFromDatabase<Category, Feed>(this);
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
      QStringList custom_ids = customIDsOfMessages(messages);

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
        if (network()->editLabels(label_custom_id, true, messages, networkProxy()) !=
              QNetworkReply::NetworkError::NoError &&
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
        if (network()->editLabels(label_custom_id, false, messages, networkProxy()) !=
              QNetworkReply::NetworkError::NoError &&
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
  setTitle(QSL("%1 (%2)").arg(TextFactory::extractUsernameFromEmail(m_network->username()),
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

    case Service::Miniflux:
      setIcon(qApp->icons()->miscIcon(QSL("miniflux")));
      break;

    default:
      setIcon(GreaderEntryPoint().icon());
      break;
  }
}

RootItem* GreaderServiceRoot::obtainNewTreeForSyncIn() const {
  return m_network->categoriesFeedsLabelsTree(true, networkProxy());
}
