// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gmailserviceroot.h"

#include "src/definitions.h"
#include "src/gmailentrypoint.h"
#include "src/gmailnetworkfactory.h"
#include "src/gui/emailpreviewer.h"
#include "src/gui/formaddeditemail.h"
#include "src/gui/formeditgmailaccount.h"

#include <librssguard/database/databasequeries.h>
#include <librssguard/exceptions/feedfetchexception.h>
#include <librssguard/miscellaneous/application.h>
#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/network-web/oauth2service.h>
#include <librssguard/services/abstract/labelsnode.h>

GmailServiceRoot::GmailServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new GmailNetworkFactory(this)), m_actionReply(nullptr) {
  m_network->setService(this);
  setIcon(GmailEntryPoint().icon());
}

GmailServiceRoot::~GmailServiceRoot() {
  if (!m_emailPreview.isNull()) {
    m_emailPreview->deleteLater();
  }
}

void GmailServiceRoot::updateTitle() {
  setTitle(TextFactory::extractUsernameFromEmail(m_network->username()) + QSL(" (Gmail)"));
}

QStringList GmailServiceRoot::getAllGmailRecipients(const QSqlDatabase& db) {
  int account_id = accountId();
  SqlQuery q(db);
  QStringList rec;

  q.prepare(QSL("SELECT author "
                "FROM Messages "
                "WHERE account_id = :account_id;"));
  q.bindValue(QSL(":account_id"), account_id);
  q.exec();

  while (q.next()) {
    auto aut = q.value(0).toString();

    if (aut.isEmpty()) {
      continue;
    }

    rec.append(aut);
  }

  rec.removeDuplicates();
  rec.sort(Qt::CaseSensitivity::CaseInsensitive);

  return rec;
}

void GmailServiceRoot::replyToEmail() {
  FormAddEditEmail(this, qApp->mainFormWidget()).show(FormAddEditEmail::Mode::Reply, &m_replyToMessage);
}

RootItem* GmailServiceRoot::obtainNewTreeForSyncIn() const {
  auto* root = new RootItem();
  Feed* inbox = new Feed(tr("Inbox"),
                         QSL(GMAIL_SYSTEM_LABEL_INBOX),
                         qApp->icons()->fromTheme(QSL("mail-inbox"), QSL("mail-inbox-symbolic")),
                         root);

  inbox->setKeepOnTop(true);

  root->appendChild(inbox);
  root
    ->appendChild(new Feed(tr("Sent"), QSL(GMAIL_SYSTEM_LABEL_SENT), qApp->icons()->fromTheme(QSL("mail-sent")), root));
  root->appendChild(new Feed(tr("Drafts"),
                             QSL(GMAIL_SYSTEM_LABEL_DRAFT),
                             qApp->icons()->fromTheme(QSL("gtk-edit")),
                             root));
  root->appendChild(new Feed(tr("Spam"),
                             QSL(GMAIL_SYSTEM_LABEL_SPAM),
                             qApp->icons()->fromTheme(QSL("mail-mark-junk")),
                             root));

  auto* lblroot = new LabelsNode(root);
  auto labels = m_network->labels(true, networkProxy());

  lblroot->setChildItems(labels);
  root->appendChild(lblroot);

  return root;
}

void GmailServiceRoot::writeNewEmail() {
  FormAddEditEmail(this, qApp->mainFormWidget()).show(FormAddEditEmail::Mode::SendNew);
}

QVariantHash GmailServiceRoot::customDatabaseData() const {
  QVariantHash data = ServiceRoot::customDatabaseData();

  data[QSL("username")] = m_network->username();
  data[QSL("batch_size")] = m_network->batchSize();
  data[QSL("download_only_unread")] = m_network->downloadOnlyUnreadMessages();
  data[QSL("client_id")] = m_network->oauth()->clientId();
  data[QSL("client_secret")] = m_network->oauth()->clientSecret();
  data[QSL("refresh_token")] = m_network->oauth()->refreshToken();
  data[QSL("redirect_uri")] = m_network->oauth()->redirectUrl();

  return data;
}

void GmailServiceRoot::setCustomDatabaseData(const QVariantHash& data) {
  ServiceRoot::setCustomDatabaseData(data);

  m_network->setUsername(data[QSL("username")].toString());
  m_network->setBatchSize(data[QSL("batch_size")].toInt());
  m_network->setDownloadOnlyUnreadMessages(data[QSL("download_only_unread")].toBool());
  m_network->oauth()->setClientId(data[QSL("client_id")].toString());
  m_network->oauth()->setClientSecret(data[QSL("client_secret")].toString());
  m_network->oauth()->setRefreshToken(data[QSL("refresh_token")].toString());
  m_network->oauth()->setRedirectUrl(data[QSL("redirect_uri")].toString(), true);
}

QList<Message> GmailServiceRoot::obtainNewMessages(Feed* feed,
                                                   const QHash<ServiceRoot::BagOfMessages, QStringList>&
                                                     stated_messages,
                                                   const QHash<QString, QStringList>& tagged_messages) {
  Q_UNUSED(tagged_messages)

  Feed::Status error = Feed::Status::Normal;
  QList<Message> messages = network()->messages(feed, stated_messages, error, networkProxy());

  if (error != Feed::Status::NewMessages && error != Feed::Status::Normal) {
    throw FeedFetchException(error);
  }

  return messages;
}

bool GmailServiceRoot::wantsBaggedIdsOfExistingMessages() const {
  return true;
}

CustomMessagePreviewer* GmailServiceRoot::customMessagePreviewer() {
  if (m_emailPreview.isNull()) {
    m_emailPreview = new EmailPreviewer(this);
  }

  m_emailPreview->webBrowser()->reloadZoomFactor();

  return m_emailPreview.data();
}

QList<QAction*> GmailServiceRoot::contextMenuMessagesList(const QList<Message>& messages) {
  if (messages.size() == 1) {
    m_replyToMessage = messages.at(0);

    if (m_actionReply == nullptr) {
      m_actionReply =
        new QAction(qApp->icons()->fromTheme(QSL("mail-reply-sender")), tr("Reply to this e-mail message"), this);
      connect(m_actionReply, &QAction::triggered, this, &GmailServiceRoot::replyToEmail);
    }

    return {m_actionReply};
  }
  else {
    return {};
  }
}

QList<QAction*> GmailServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    ServiceRoot::serviceMenu();

    QAction* act_new_email =
      new QAction(qApp->icons()->fromTheme(QSL("mail-message-new")), tr("Write new e-mail message"), this);

    connect(act_new_email, &QAction::triggered, this, &GmailServiceRoot::writeNewEmail);
    m_serviceMenu.append(act_new_email);
  }

  return m_serviceMenu;
}

bool GmailServiceRoot::isSyncable() const {
  return true;
}

bool GmailServiceRoot::canBeEdited() const {
  return true;
}

FormAccountDetails* GmailServiceRoot::accountSetupDialog() const {
  return new FormEditGmailAccount(qApp->mainFormWidget());
}

void GmailServiceRoot::editItems(const QList<RootItem*>& items) {
  if (items.first()->kind() == RootItem::Kind::ServiceRoot) {
    QScopedPointer<FormEditGmailAccount> p(qobject_cast<FormEditGmailAccount*>(accountSetupDialog()));

    p->addEditAccount(this);
    return;
  }

  ServiceRoot::editItems(items);
}

bool GmailServiceRoot::supportsFeedAdding() const {
  return false;
}

bool GmailServiceRoot::supportsCategoryAdding() const {
  return false;
}

void GmailServiceRoot::start(bool freshly_activated) {
  if (!freshly_activated) {
    DatabaseQueries::loadRootFromDatabase<Category, Feed>(this);
    loadCacheFromFile();
  }

  updateTitle();

  if (getSubTreeFeeds().isEmpty()) {
    m_network->oauth()->login([this]() {
      syncIn();
    });
  }
  else {
    auto chi = childItems();

    for (RootItem* feed : std::as_const(chi)) {
      if (feed->customId() == QL1S(GMAIL_SYSTEM_LABEL_INBOX)) {
        feed->setKeepOnTop(true);
      }
    }

    m_network->oauth()->login();
  }
}

QString GmailServiceRoot::code() const {
  return GmailEntryPoint().code();
}

QString GmailServiceRoot::additionalTooltip() const {
  return ServiceRoot::additionalTooltip() + QSL("\n") +
         tr("Authentication status: %1\n"
            "Login tokens expiration: %2")
           .arg(network()->oauth()->isFullyLoggedIn() ? tr("logged-in") : tr("NOT logged-in"),
                network()->oauth()->tokensExpireIn().isValid() ? network()->oauth()->tokensExpireIn().toString()
                                                               : QSL("-"));
}

void GmailServiceRoot::saveAllCachedData(bool ignore_errors) {
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

  QMapIterator<QString, QStringList> k(msg_cache.m_cachedLabelAssignments);

  // Assign label for these messages.
  while (k.hasNext()) {
    k.next();
    auto label_custom_id = k.key();
    QStringList messages = k.value();

    if (!messages.isEmpty()) {
      auto res = network()->batchModify(label_custom_id, messages, true, networkProxy());

      if (res != QNetworkReply::NetworkError::NoError) {
        qCriticalNN << LOGSEC_FEEDLY << "Failed to synchronize tag assignments with error:" << QUOTE_W_SPACE(res);

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
      auto res = network()->batchModify(label_custom_id, messages, false, networkProxy());

      if (res != QNetworkReply::NetworkError::NoError) {
        qCriticalNN << LOGSEC_FEEDLY << "Failed to synchronize tag deassignments with error:" << QUOTE_W_SPACE(res);

        addLabelsAssignmentsToCache(messages, label_custom_id, false);
      }
    }
  }
}

bool GmailServiceRoot::displaysEnclosures() const {
  return false;
}

ServiceRoot::LabelOperation GmailServiceRoot::supportedLabelOperations() const {
  return ServiceRoot::LabelOperation::Synchronised;
}

GmailNetworkFactory* GmailServiceRoot::network() const {
  return m_network;
}
