// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/xmppserviceroot.h"

#include "src/definitions.h"
#include "src/xmppentrypoint.h"
#include "src/xmppfeed.h"
#include "src/xmppnetwork.h"
#include "src/gui/formeditxmppaccount.h"
#include "src/gui/formxmppfeeddetails.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/definitions/definitions.h>
#include <librssguard/gui/dialogs/filedialog.h>
#include <librssguard/gui/messagebox.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/miscellaneous/mutex.h>
#include <librssguard/miscellaneous/textfactory.h>
#include <librssguard/network-web/oauth2service.h>

XmppServiceRoot::XmppServiceRoot(RootItem* parent) : ServiceRoot(parent), m_network(new XmppNetwork(this)) {
  setIcon(XmppEntryPoint().icon());
  m_network->setRoot(this);
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
    QScopedPointer<FormXmppFeedDetails> form_pointer(new FormXmppFeedDetails(this,
                                                                                   nullptr,
                                                                                   {},
                                                                                   qApp->mainFormWidget()));

    form_pointer->addEditFeed<XmppFeed>(feeds.toList());
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

void XmppServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  ServiceRoot::setCustomDatabaseData(data);

  m_network->setService(XmppServiceRoot::Service(data[QSL("service")].toInt()));
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

    m_network->setBaseUrl(QSL(XMPP_URL_INOREADER));
  }
  else {
    m_network->setBaseUrl(data[QSL("url")].toString());
  }
}

void XmppServiceRoot::aboutToBeginFeedFetching(const QList<Feed*>& feeds,
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

QString XmppServiceRoot::serviceToString(Service service) {
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

void XmppServiceRoot::importFeeds() {
  const QString filter_opml20 = tr("OPML 2.0 files (*.opml *.xml)");
  const QString selected_file = FileDialog::openFileName(qApp->mainFormWidget(),
                                                         tr("Select file for feeds import"),
                                                         qApp->homeFolder(),
                                                         {},
                                                         filter_opml20,
                                                         nullptr,
                                                         GENERAL_REMEMBERED_PATH);

  if (!QFile::exists(selected_file)) {
    return;
  }

  try {
    m_network->subscriptionImport(IOFactory::readFile(selected_file), networkProxy());
    MsgBox::show({},
                 QMessageBox::Icon::Information,
                 tr("Done"),
                 tr("Data imported successfully. Reloading feed tree."));

    syncIn();
  }
  catch (const ApplicationException& ex) {
    MsgBox::show({}, QMessageBox::Icon::Critical, tr("Cannot import feeds"), tr("Error: %1").arg(ex.message()));
  }
}

void XmppServiceRoot::exportFeeds() {
  const QString the_file = QSL("rssguard_feeds_%1.opml").arg(QDate::currentDate().toString(Qt::DateFormat::ISODate));
  const QString filter_opml20 = tr("OPML 2.0 files (*.opml *.xml)");
  const QString selected_file = FileDialog::saveFileName(qApp->mainFormWidget(),
                                                         tr("Select file for feeds export"),
                                                         qApp->documentsFolder(),
                                                         the_file,
                                                         filter_opml20,
                                                         nullptr,
                                                         GENERAL_REMEMBERED_PATH);

  if (selected_file.isEmpty()) {
    return;
  }

  try {
    QByteArray data = m_network->subscriptionExport(networkProxy());
    IOFactory::writeFile(selected_file, data);

    MsgBox::show({}, QMessageBox::Icon::Information, tr("Done"), tr("Data exported successfully."));
  }
  catch (const ApplicationException& ex) {
    MsgBox::show({}, QMessageBox::Icon::Critical, tr("Cannot export feeds"), tr("Error: %1").arg(ex.message()));
  }
}

QList<Message> XmppServiceRoot::obtainNewMessages(Feed* feed,
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

bool XmppServiceRoot::wantsBaggedIdsOfExistingMessages() const {
  return m_network->intelligentSynchronization();
}

void XmppServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadRootFromDatabase<Category, XmppFeed>(this);
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

QString XmppServiceRoot::code() const {
  return XmppEntryPoint().code();
}

QList<QAction*> XmppServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    ServiceRoot::serviceMenu();

    auto* action_export_feeds = new QAction(qApp->icons()->fromTheme(QSL("document-export")), tr("Export feeds"), this);
    auto* action_import_feeds = new QAction(qApp->icons()->fromTheme(QSL("document-import")), tr("Import feeds"), this);

    connect(action_export_feeds, &QAction::triggered, this, &XmppServiceRoot::exportFeeds);
    connect(action_import_feeds, &QAction::triggered, this, &XmppServiceRoot::importFeeds);

    m_serviceMenu.append(action_export_feeds);
    m_serviceMenu.append(action_import_feeds);
  }

  return m_serviceMenu;
}

QString XmppServiceRoot::additionalTooltip() const {
  QString source_str = QUrl(m_network->baseUrl()).isValid() ? QSL("<a href=\"%1\">%1</a>").arg(m_network->baseUrl())
                                                            : m_network->baseUrl();

  return source_str + QSL("\n\n") + ServiceRoot::additionalTooltip();
}

void XmppServiceRoot::saveAllCachedData(bool ignore_errors) {
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

ServiceRoot::LabelOperation XmppServiceRoot::supportedLabelOperations() const {
  return ServiceRoot::LabelOperation::Synchronised;
}

bool XmppServiceRoot::supportsFeedAdding() const {
  return true;
}

void XmppServiceRoot::addNewFeed(RootItem* selected_item, const QString& url) {
  if (!qApp->feedUpdateLock()->tryLock()) {
    // Lock was not obtained because
    // it is used probably by feed updater or application
    // is quitting.
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot add item"),
                          tr("Cannot add feed because another critical operation is ongoing."),
                          QSystemTrayIcon::MessageIcon::Warning});

    return;
  }

  QScopedPointer<FormXmppFeedDetails> form_pointer(new FormXmppFeedDetails(this,
                                                                                 selected_item,
                                                                                 url,
                                                                                 qApp->mainFormWidget()));

  form_pointer->addEditFeed<XmppFeed>();
  qApp->feedUpdateLock()->unlock();
}

void XmppServiceRoot::updateTitleIcon() {
  setTitle(QSL("%1 (%2)").arg(TextFactory::extractUsernameFromEmail(m_network->username()),
                              XmppServiceRoot::serviceToString(m_network->service())));

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
      setIcon(XmppEntryPoint().icon());
      break;
  }
}

RootItem* XmppServiceRoot::obtainNewTreeForSyncIn() const {
  return m_network->categoriesFeedsLabelsTree(true, networkProxy());
}
