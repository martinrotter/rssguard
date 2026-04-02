// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppserviceroot.h"

#include "src/definitions.h"
#include "src/gui/formeditxmppaccount.h"
#include "src/xmppentrypoint.h"
#include "src/xmppfeed.h"
#include "src/xmppnetwork.h"
#include "src/xmppubsubpmanager.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/gui/dialogs/filedialog.h>
#include <librssguard/gui/messagebox.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/mutex.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/network-web/oauth2service.h>
#include <qtlinq/qtlinq.h>

#include <QAction>
#include <QXmppUtils.h>

XmppServiceRoot::XmppServiceRoot(RootItem* parent) : ServiceRoot(parent), m_network(new XmppNetwork(this)) {
  setIcon(XmppEntryPoint().icon());
}

bool XmppServiceRoot::isSyncable() const {
  return true;
}

bool XmppServiceRoot::canBeEdited() const {
  return true;
}

FormAccountDetails* XmppServiceRoot::accountSetupDialog() const {
  return new FormEditXmppAccount(qApp->mainFormWidget());
}

void XmppServiceRoot::editItems(const QList<RootItem*>& items) {
  auto feeds = qlinq::from(items).ofType<Feed*>();

  if (!feeds.isEmpty()) {
    /*QScopedPointer<FormXmppFeedDetails> form_pointer(new FormXmppFeedDetails(this,
                                                                             nullptr,
                                                                             {},
                                                                             qApp->mainFormWidget()));

    form_pointer->addEditFeed<XmppFeed>(feeds.toList());
    */
    return;
  }

  if (items.first()->kind() == RootItem::Kind::ServiceRoot) {
    QScopedPointer<FormEditXmppAccount> p(qobject_cast<FormEditXmppAccount*>(accountSetupDialog()));
    p->addEditAccount(this);
    return;
  }

  ServiceRoot::editItems(items);
}

QVariantHash XmppServiceRoot::customDatabaseData() const {
  QVariantHash data = ServiceRoot::customDatabaseData();

  data[QSL("username")] = m_network->username();
  data[QSL("password")] = TextFactory::encrypt(m_network->password());
  data[QSL("domain")] = m_network->domain();
  data[QSL("services")] = m_network->extraNodes();

  return data;
}

void XmppServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  ServiceRoot::setCustomDatabaseData(data);

  m_network->setUsername(data[QSL("username")].toString());
  m_network->setPassword(TextFactory::decrypt(data[QSL("password")].toString()));
  m_network->setDomain(data[QSL("domain")].toString());
  m_network->setExtraNodes(data[QSL("services")].toStringList());
}

void XmppServiceRoot::aboutToBeginFeedFetching(const QList<Feed*>& feeds,
                                               const QHash<QString, QHash<BagOfMessages, QStringList>>& stated_messages,
                                               const QHash<QString, QStringList>& tagged_messages) {}

QList<Message> XmppServiceRoot::obtainNewMessages(Feed* feed,
                                                  const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                                  const QHash<QString, QStringList>& tagged_messages) {
  Q_UNUSED(tagged_messages)

  QList<Message> msgs;
  auto* xmpp_node = qobject_cast<XmppFeed*>(feed);
  auto async_articles = xmpp_node->articles();

  auto new_async_articles =
    qlinq::from(async_articles)
      .where([&](const Message& msg) {
        return !stated_messages.value(ServiceRoot::BagOfMessages::Read).contains(msg.m_customId) &&
               !stated_messages.value(ServiceRoot::BagOfMessages::Unread).contains(msg.m_customId) &&
               !stated_messages.value(ServiceRoot::BagOfMessages::Starred).contains(msg.m_customId);
      })
      .toList();

  msgs.append(new_async_articles);

  // Clear it all.
  xmpp_node->setArticles({});

  return msgs;
}

bool XmppServiceRoot::wantsBaggedIdsOfExistingMessages() const {
  return true;
}

void XmppServiceRoot::requestSyncIn() {
  if (m_syncInRunning) {
    return;
  }

  ServiceRoot::requestSyncIn();

  m_network->obtainServicesNodesTree();
}

void XmppServiceRoot::onRealTimeArticleObtained(const QString& service,
                                                const QString& node,
                                                const Message& message,
                                                XmppFeed* feed) {
  feed = feed == nullptr ? findFeed(service, node) : feed;

  if (feed == nullptr) {
    qApp->showGuiMessage(Notification::Event::ArticlesFetchingError,
                         GuiMessage(tr("Cannot store article"),
                                    tr("Cannot save article obtained via push notification because its feed does not "
                                       "exist. Tray to refresh list of feeds."),
                                    QSystemTrayIcon::MessageIcon::Critical));
  }
  else {
    feed->storeRealTimeArticle(message);

    if (!feed->isSwitchedOff()) {
      emit feedFetchRequested({feed});
    }
  }
}

XmppFeed* XmppServiceRoot::findFeed(const QString& service, const QString& node) const {
  RootItem* feed = getItemFromSubTree([&](const RootItem* item) {
    return item->kind() == RootItem::Kind::Feed && item->customId() == node && item->parent() != nullptr &&
           item->parent()->customId() == service;
  });

  return qobject_cast<XmppFeed*>(feed);
}

XmppFeed* XmppServiceRoot::findFeed(const QString& jid, XmppFeed::Type type) const {
  QString bare_jid = QXmppUtils::jidToBareJid(jid);

  RootItem* feed = getItemFromSubTree([&](const RootItem* item) {
    return item->kind() == RootItem::Kind::Feed && item->customId() == bare_jid &&
           qobject_cast<const XmppFeed*>(item)->type() == type;
  });

  return qobject_cast<XmppFeed*>(feed);
}

void XmppServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadRootFromDatabase<Category, XmppFeed>(this);
  }

  updateTitle();
  m_network->connectToServer();
}

void XmppServiceRoot::stop() {
  m_network->disconnectFromServer();
}

QString XmppServiceRoot::code() const {
  return XmppEntryPoint().code();
}

QList<QAction*> XmppServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    ServiceRoot::serviceMenu();

    m_actReconnect = new QAction(qApp->icons()->fromTheme(QSL("view-refresh")), tr("&Reconnect"), this);
    connect(m_actReconnect, &QAction::triggered, m_network, &XmppNetwork::reconnect);

    m_serviceMenu.append(m_actReconnect);
  }

  return m_serviceMenu;
}

QString XmppServiceRoot::additionalTooltip() const {
  QString source_str = tr("User: %1\nStatus: %2\nSupported XEPs: %3")
                         .arg(m_network->username(), m_network->clientState(), m_network->xeps().join(QSL(", ")));
  return source_str + QSL("\n\n") + ServiceRoot::additionalTooltip();
}

bool XmppServiceRoot::supportsFeedAdding() const {
  return false;
}

void XmppServiceRoot::updateTitle() {
  setTitle(m_network->username());
}
