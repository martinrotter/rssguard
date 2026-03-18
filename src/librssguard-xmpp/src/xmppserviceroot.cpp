// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppserviceroot.h"

#include "src/definitions.h"
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
  return nullptr; // new FormEditXmppAccount(qApp->mainFormWidget());
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
    // QScopedPointer<FormEditXmppAccount> p(qobject_cast<FormEditXmppAccount*>(accountSetupDialog()));

    // p->addEditAccount(this);
    return;
  }

  ServiceRoot::editItems(items);
}

QVariantHash XmppServiceRoot::customDatabaseData() const {
  QVariantHash data = ServiceRoot::customDatabaseData();

  data[QSL("username")] = m_network->username();
  data[QSL("password")] = TextFactory::encrypt(m_network->password());
  data[QSL("domain")] = m_network->domain();

  return data;
}

void XmppServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  ServiceRoot::setCustomDatabaseData(data);

  m_network->setUsername(data[QSL("username")].toString());
  m_network->setPassword(TextFactory::decrypt(data[QSL("password")].toString()));
  m_network->setDomain(data[QSL("domain")].toString());
}

void XmppServiceRoot::aboutToBeginFeedFetching(const QList<Feed*>& feeds,
                                               const QHash<QString, QHash<BagOfMessages, QStringList>>& stated_messages,
                                               const QHash<QString, QStringList>& tagged_messages) {}

QList<Message> XmppServiceRoot::obtainNewMessages(Feed* feed,
                                                  const QHash<ServiceRoot::BagOfMessages, QStringList>& stated_messages,
                                                  const QHash<QString, QStringList>& tagged_messages) {
  QList<Message> msgs;

  return msgs;
}

bool XmppServiceRoot::wantsBaggedIdsOfExistingMessages() const {
  return true;
}

void XmppServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadRootFromDatabase<Category, XmppFeed>(this);
  }

  if (getSubTreeFeeds().isEmpty()) {
    syncIn();
  }
}

QString XmppServiceRoot::code() const {
  return XmppEntryPoint().code();
}

QList<QAction*> XmppServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    ServiceRoot::serviceMenu();
  }

  return m_serviceMenu;
}

QString XmppServiceRoot::additionalTooltip() const {
  QString source_str = tr("User: %1\n").arg(m_network->username());
  return source_str + ServiceRoot::additionalTooltip();
}

ServiceRoot::LabelOperation XmppServiceRoot::supportedLabelOperations() const {
  return ServiceRoot::LabelOperation::Synchronised;
}

bool XmppServiceRoot::supportsFeedAdding() const {
  return false;
}

RootItem* XmppServiceRoot::obtainNewTreeForSyncIn() const {
  return nullptr;
}
