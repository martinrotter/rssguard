// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gmailserviceroot.h"

#include "database/databasequeries.h"
#include "exceptions/feedfetchexception.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/recyclebin.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailentrypoint.h"
#include "services/gmail/gmailnetworkfactory.h"
#include "services/gmail/gui/emailpreviewer.h"
#include "services/gmail/gui/formaddeditemail.h"
#include "services/gmail/gui/formeditgmailaccount.h"

#include <QFileDialog>

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

void GmailServiceRoot::replyToEmail() {
  FormAddEditEmail(this, qApp->mainFormWidget()).execForReply(&m_replyToMessage);
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

  return root;
}

void GmailServiceRoot::writeNewEmail() {
  FormAddEditEmail(this, qApp->mainFormWidget()).execForAdd();
}

QVariantHash GmailServiceRoot::customDatabaseData() const {
  QVariantHash data;

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
  Q_UNUSED(stated_messages)
  Q_UNUSED(tagged_messages)

  Feed::Status error = Feed::Status::Normal;
  QList<Message> messages = network()->messages(feed->customId(), stated_messages, error, networkProxy());

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

bool GmailServiceRoot::editViaGui() {
  FormEditGmailAccount form_pointer(qApp->mainFormWidget());

  form_pointer.addEditAccount(this);
  return true;
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
    syncIn();
  }

  auto chi = childItems();

  for (RootItem* feed : qAsConst(chi)) {
    if (feed->customId() == QL1S(GMAIL_SYSTEM_LABEL_INBOX)) {
      feed->setKeepOnTop(true);
    }
  }

  m_network->oauth()->login();
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
}

bool GmailServiceRoot::displaysEnclosures() const {
  return false;
}
