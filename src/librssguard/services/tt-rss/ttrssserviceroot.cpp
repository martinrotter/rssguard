// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/ttrssserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/networkfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/recyclebin.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/formeditttrssaccount.h"
#include "services/tt-rss/gui/formttrssfeeddetails.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"

#include <QClipboard>
#include <QPair>
#include <QSqlTableModel>

TtRssServiceRoot::TtRssServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new TtRssNetworkFactory()) {
  setIcon(TtRssServiceEntryPoint().icon());
}

TtRssServiceRoot::~TtRssServiceRoot() {
  delete m_network;
}

ServiceRoot::LabelOperation TtRssServiceRoot::supportedLabelOperations() const {
  return ServiceRoot::LabelOperation(0);
}

void TtRssServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)
  loadFromDatabase();
  loadCacheFromFile();

  if (childCount() <= 3) {
    syncIn();
  }
}

void TtRssServiceRoot::stop() {
  m_network->logout(networkProxy());
  qDebugNN << LOGSEC_TTRSS
           << "Stopping Tiny Tiny RSS account, logging out with result"
           << QUOTE_W_SPACE_DOT(m_network->lastError());
}

QString TtRssServiceRoot::code() const {
  return TtRssServiceEntryPoint().code();
}

bool TtRssServiceRoot::isSyncable() const {
  return true;
}

bool TtRssServiceRoot::editViaGui() {
  QScopedPointer<FormEditTtRssAccount> form_pointer(new FormEditTtRssAccount(qApp->mainFormWidget()));

  form_pointer->addEditAccount(this);
  return true;
}

bool TtRssServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  // Remove extra entry in "Tiny Tiny RSS accounts list" and then delete
  // all the categories/feeds and messages.
  if (DatabaseQueries::deleteTtRssAccount(database, accountId())) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}

bool TtRssServiceRoot::supportsFeedAdding() const {
  return true;
}

bool TtRssServiceRoot::supportsCategoryAdding() const {
  return false;
}

void TtRssServiceRoot::addNewFeed(RootItem* selected_item, const QString& url) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(tr("Cannot add item"),
                         tr("Cannot add feed because another critical operation is ongoing."),
                         QSystemTrayIcon::MessageIcon::Warning,
                         qApp->mainFormWidget(),
                         true);

    return;
  }

  QScopedPointer<FormTtRssFeedDetails> form_pointer(new FormTtRssFeedDetails(this, qApp->mainFormWidget()));

  form_pointer->addFeed(selected_item, url);
  qApp->feedUpdateLock()->unlock();
}

bool TtRssServiceRoot::canBeEdited() const {
  return true;
}

bool TtRssServiceRoot::canBeDeleted() const {
  return true;
}

void TtRssServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

  // Save the actual data read/unread.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      auto res = network()->updateArticles(ids,
                                           UpdateArticle::OperatingField::Unread,
                                           key == RootItem::ReadStatus::Unread
                                           ? UpdateArticle::Mode::SetToTrue
                                           : UpdateArticle::Mode::SetToFalse,
                                           networkProxy());

      if (!ignore_errors && (network()->lastError() != QNetworkReply::NetworkError::NoError || res.hasError())) {
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
      QStringList ids = customIDsOfMessages(messages);
      auto res = network()->updateArticles(ids,
                                           UpdateArticle::OperatingField::Starred,
                                           key == RootItem::Importance::Important
                                           ? UpdateArticle::Mode::SetToTrue
                                           : UpdateArticle::Mode::SetToFalse,
                                           networkProxy());

      if (!ignore_errors && (network()->lastError() != QNetworkReply::NetworkError::NoError || res.hasError())) {
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
      auto res = network()->setArticleLabel(messages, label_custom_id, true, networkProxy());

      if (!ignore_errors && (network()->lastError() != QNetworkReply::NetworkError::NoError || res.hasError())) {
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
      auto res = network()->setArticleLabel(messages, label_custom_id, false, networkProxy());

      if (!ignore_errors && (network()->lastError() != QNetworkReply::NetworkError::NoError || res.hasError())) {
        addLabelsAssignmentsToCache(messages, label_custom_id, false);
      }
    }
  }
}

QString TtRssServiceRoot::additionalTooltip() const {
  return tr("Username: %1\nServer: %2\n"
            "Last error: %3\nLast login on: %4").arg(m_network->username(),
                                                     m_network->url(),
                                                     NetworkFactory::networkErrorText(m_network->lastError()),
                                                     m_network->lastLoginTime().isValid()
                                                     ? QLocale().toString(m_network->lastLoginTime(), QLocale::FormatType::ShortFormat)
                                                     : QSL("-"));
}

TtRssNetworkFactory* TtRssServiceRoot::network() const {
  return m_network;
}

void TtRssServiceRoot::saveAccountDataToDatabase(bool creating_new) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (!creating_new) {
    // We are overwritting previously saved data.
    if (DatabaseQueries::overwriteTtRssAccount(database, m_network->username(), m_network->password(),
                                               m_network->authIsUsed(), m_network->authUsername(),
                                               m_network->authPassword(), m_network->url(),
                                               m_network->forceServerSideUpdate(), m_network->downloadOnlyUnreadMessages(),
                                               accountId())) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    if (DatabaseQueries::createTtRssAccount(database, accountId(), m_network->username(),
                                            m_network->password(), m_network->authIsUsed(),
                                            m_network->authUsername(), m_network->authPassword(),
                                            m_network->url(), m_network->forceServerSideUpdate(),
                                            m_network->downloadOnlyUnreadMessages())) {
      updateTitle();
    }
  }
}

void TtRssServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories<Category>(database, accountId());
  Assignment feeds = DatabaseQueries::getFeeds<TtRssFeed>(database, qApp->feedReader()->messageFilters(), accountId());
  auto labels = DatabaseQueries::getLabels(database, accountId());

  performInitialAssembly(categories, feeds, labels);
}

void TtRssServiceRoot::updateTitle() {
  QString host = QUrl(m_network->url()).host();

  if (host.isEmpty()) {
    host = m_network->url();
  }

  setTitle(m_network->username() + QSL(" (Tiny Tiny RSS)"));
}

RootItem* TtRssServiceRoot::obtainNewTreeForSyncIn() const {
  TtRssGetFeedsCategoriesResponse feed_cats = m_network->getFeedsCategories(networkProxy());
  TtRssGetLabelsResponse labels = m_network->getLabels(networkProxy());

  if (m_network->lastError() == QNetworkReply::NoError) {
    auto* tree = feed_cats.feedsCategories(true, m_network->url());
    auto* lblroot = new LabelsNode(tree);

    lblroot->setChildItems(labels.labels());
    tree->appendChild(lblroot);

    return tree;
  }
  else {
    return nullptr;
  }
}
