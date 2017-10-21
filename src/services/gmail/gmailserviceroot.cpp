// For license of this file, see <object-root-folder>/LICENSE.md.

#include "services/gmail/gmailserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/abstract/recyclebin.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailentrypoint.h"
#include "services/gmail/gmailfeed.h"
#include "services/gmail/network/gmailnetworkfactory.h"

GmailServiceRoot::GmailServiceRoot(GmailNetworkFactory* network, RootItem* parent) : ServiceRoot(parent),
  CacheForServiceRoot(), m_serviceMenu(QList<QAction*>()), m_network(network) {
  if (network == nullptr) {
    m_network = new GmailNetworkFactory(this);
  }
  else {
    m_network->setParent(this);
  }

  m_network->setService(this);
  setIcon(GmailEntryPoint().icon());
}

GmailServiceRoot::~GmailServiceRoot() {}

void GmailServiceRoot::updateTitle() {
  setTitle(m_network->userName() + QSL(" (Gmail)"));
}

RootItem* GmailServiceRoot::obtainNewTreeForSyncIn() const {
  RootItem* root = new RootItem();
  GmailFeed* inbox = new GmailFeed(tr("Inbox"), QSL(GMAIL_SYSTEM_LABEL_INBOX), qApp->icons()->fromTheme(QSL("mail-inbox")), root);

  inbox->setKeepOnTop(true);

  root->appendChild(inbox);
  root->appendChild(new GmailFeed(tr("Sent"), QSL(GMAIL_SYSTEM_LABEL_SENT), qApp->icons()->fromTheme(QSL("mail-sent")), root));
  root->appendChild(new GmailFeed(tr("Drafts"), QSL(GMAIL_SYSTEM_LABEL_DRAFT), qApp->icons()->fromTheme(QSL("gtk-edit")), root));
  root->appendChild(new GmailFeed(tr("Spam"), QSL(GMAIL_SYSTEM_LABEL_SPAM), qApp->icons()->fromTheme(QSL("mail-mark-junk")), root));

  return root;
}

void GmailServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
  Assignment categories = DatabaseQueries::getCategories(database, accountId());
  Assignment feeds = DatabaseQueries::getGmailFeeds(database, accountId());

  // All data are now obtained, lets create the hierarchy.
  assembleCategories(categories);
  assembleFeeds(feeds);

  foreach (RootItem* feed, childItems()) {
    if (feed->customId() == QL1S("INBOX")) {
      feed->setKeepOnTop(true);
    }
  }

  // As the last item, add recycle bin, which is needed.
  appendChild(recycleBin());
  updateCounts(true);
}

void GmailServiceRoot::saveAccountDataToDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (accountId() != NO_PARENT_CATEGORY) {
    if (DatabaseQueries::overwriteGmailAccount(database, m_network->userName(),
                                               m_network->oauth()->clientId(),
                                               m_network->oauth()->clientSecret(),
                                               m_network->oauth()->redirectUrl(),
                                               m_network->oauth()->refreshToken(),
                                               m_network->batchSize(),
                                               accountId())) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    bool saved;
    int id_to_assign = DatabaseQueries::createAccount(database, code(), &saved);

    if (saved) {
      if (DatabaseQueries::createGmailAccount(database, id_to_assign,
                                              m_network->userName(),
                                              m_network->oauth()->clientId(),
                                              m_network->oauth()->clientSecret(),
                                              m_network->oauth()->redirectUrl(),
                                              m_network->oauth()->refreshToken(),
                                              m_network->batchSize())) {
        setId(id_to_assign);
        setAccountId(id_to_assign);
        updateTitle();
      }
    }
  }
}

bool GmailServiceRoot::canBeEdited() const {
  return true;
}

bool GmailServiceRoot::editViaGui() {
  //FormEditInoreaderAccount form_pointer(qApp->mainFormWidget());
  // TODO: dodělat
  //form_pointer.execForEdit(this);
  return true;
}

bool GmailServiceRoot::supportsFeedAdding() const {
  return true;
}

bool GmailServiceRoot::supportsCategoryAdding() const {
  return false;
}

void GmailServiceRoot::start(bool freshly_activated) {
  Q_UNUSED(freshly_activated)

  loadFromDatabase();
  loadCacheFromFile(accountId());

  m_network->oauth()->login();

  if (childCount() <= 1) {
    syncIn();
  }
}

void GmailServiceRoot::stop() {
  saveCacheToFile(accountId());
}

QString GmailServiceRoot::code() const {
  return GmailEntryPoint().code();
}

QString GmailServiceRoot::additionalTooltip() const {
  return tr("Authentication status: %1\n"
            "Login tokens expiration: %2").arg(network()->oauth()->isFullyLoggedIn() ? tr("logged-in") : tr("NOT logged-in"),
                                               network()->oauth()->tokensExpireIn().isValid() ?
                                               network()->oauth()->tokensExpireIn().toString() : QSL("-"));
}

void GmailServiceRoot::saveAllCachedData(bool async) {
  QPair<QMap<RootItem::ReadStatus, QStringList>, QMap<RootItem::Importance, QList<Message>>> msgCache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msgCache.first);

  // Save the actual data read/unread.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      // TODO: dodělat
      //network()->markMessagesRead(key, ids, async);
    }
  }

  QMapIterator<RootItem::Importance, QList<Message>> j(msgCache.second);

  // Save the actual data important/not important.
  while (j.hasNext()) {
    j.next();
    auto key = j.key();

    QList<Message> messages = j.value();

    if (!messages.isEmpty()) {
      QStringList custom_ids;

      foreach (const Message& msg, messages) {
        custom_ids.append(msg.m_customId);
      }

      // TODO: dodělat
      //network()->markMessagesStarred(key, custom_ids, async);
    }
  }
}

bool GmailServiceRoot::canBeDeleted() const {
  return true;
}

bool GmailServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  if (DatabaseQueries::deleteGmailAccount(database, accountId())) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}
