// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/inoreader/inoreaderserviceroot.h"

#include "database/databasequeries.h"
#include "exceptions/feedfetchexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/recyclebin.h"
#include "services/inoreader/gui/formeditinoreaderaccount.h"
#include "services/inoreader/inoreaderentrypoint.h"
#include "services/inoreader/inoreadernetworkfactory.h"

#include <QThread>

InoreaderServiceRoot::InoreaderServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new InoreaderNetworkFactory(this)) {
  m_network->setService(this);
  setIcon(InoreaderEntryPoint().icon());
}

InoreaderServiceRoot::~InoreaderServiceRoot() = default;

void InoreaderServiceRoot::updateTitle() {
  setTitle(TextFactory::extractUsernameFromEmail(m_network->username()) + QSL(" (Inoreader)"));
}

ServiceRoot::LabelOperation InoreaderServiceRoot::supportedLabelOperations() const {
  return ServiceRoot::LabelOperation(0);
}

QVariantHash InoreaderServiceRoot::customDatabaseData() const {
  QVariantHash data;

  data["username"] = m_network->username();
  data["download_only_unread"] = m_network->downloadOnlyUnreadMessages();
  data["batch_size"] = m_network->batchSize();
  data["client_id"] = m_network->oauth()->clientId();
  data["client_secret"] = m_network->oauth()->clientSecret();
  data["refresh_token"] = m_network->oauth()->refreshToken();
  data["redirect_uri"] = m_network->oauth()->redirectUrl();

  return data;
}

void InoreaderServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  m_network->setUsername(data["username"].toString());
  m_network->setBatchSize(data["batch_size"].toInt());
  m_network->setDownloadOnlyUnreadMessages(data["download_only_unread"].toBool());
  m_network->oauth()->setClientId(data["client_id"].toString());
  m_network->oauth()->setClientSecret(data["client_secret"].toString());
  m_network->oauth()->setRefreshToken(data["refresh_token"].toString());
  m_network->oauth()->setRedirectUrl(data["redirect_uri"].toString());
}

QList<Message> InoreaderServiceRoot::obtainNewMessages(const QList<Feed*>& feeds,
                                                       const QHash<QString, QHash<ServiceRoot::BagOfMessages, QStringList>>& stated_messages,
                                                       const QHash<QString, QStringList>& tagged_messages) {
  Q_UNUSED(stated_messages)
  Q_UNUSED(tagged_messages)

  QList<Message> messages;

  for (Feed* feed : feeds) {
    Feed::Status error = Feed::Status::Normal;

    messages << network()->messages(this, feed->customId(), error);

    if (error == Feed::Status::NetworkError || error == Feed::Status::AuthError) {
      throw FeedFetchException(error);
    }
  }

  return messages;
}

bool InoreaderServiceRoot::isSyncable() const {
  return true;
}

bool InoreaderServiceRoot::canBeEdited() const {
  return true;
}

bool InoreaderServiceRoot::editViaGui() {
  FormEditInoreaderAccount form_pointer(qApp->mainFormWidget());

  form_pointer.addEditAccount(this);
  return true;
}

bool InoreaderServiceRoot::supportsFeedAdding() const {
  return false;
}

bool InoreaderServiceRoot::supportsCategoryAdding() const {
  return false;
}

void InoreaderServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadFromDatabase<Category, Feed>(this);
    loadCacheFromFile();
  }

  updateTitle();

  if (getSubTreeFeeds().isEmpty()) {
    m_network->oauth()->login([this]() {
      syncIn();
    });
  }
  else {
    m_network->oauth()->login();
  }
}

QString InoreaderServiceRoot::code() const {
  return InoreaderEntryPoint().code();
}

QString InoreaderServiceRoot::additionalTooltip() const {
  return tr("Authentication status: %1\n"
            "Login tokens expiration: %2").arg(network()->oauth()->isFullyLoggedIn() ? tr("logged-in") : tr("NOT logged-in"),
                                               network()->oauth()->tokensExpireIn().isValid() ?
                                               network()->oauth()->tokensExpireIn().toString() : QSL("-"));
}

RootItem* InoreaderServiceRoot::obtainNewTreeForSyncIn() const {
  auto tree = m_network->feedsCategories(true);

  if (tree != nullptr) {
    auto* lblroot = new LabelsNode(tree);
    auto labels = m_network->getLabels();

    lblroot->setChildItems(labels);
    tree->appendChild(lblroot);

    return tree;
  }
  else {
    return nullptr;
  }
}

void InoreaderServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

  // Save the actual data read/unread.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      if (network()->markMessagesRead(key, ids) != QNetworkReply::NetworkError::NoError && !ignore_errors) {
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

      if (network()->markMessagesStarred(key, custom_ids) != QNetworkReply::NetworkError::NoError && !ignore_errors) {
        addMessageStatesToCache(messages, key);
      }
    }
  }

  QMapIterator<QString, QStringList> k(msg_cache.m_cachedLabelAssignments);

  // Assign label for these messages.
  while (k.hasNext()) {
    k.next();
    auto label_custom_id = k.key();
    QStringList messages = k.value();

    if (!messages.isEmpty()) {
      if (network()->editLabels(label_custom_id, true, messages) != QNetworkReply::NetworkError::NoError && !ignore_errors) {
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
      if (network()->editLabels(label_custom_id, false, messages) != QNetworkReply::NetworkError::NoError && !ignore_errors) {
        addLabelsAssignmentsToCache(messages, label_custom_id, false);
      }
    }
  }
}
