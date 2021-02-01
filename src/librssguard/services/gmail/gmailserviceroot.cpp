// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gmailserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/recyclebin.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailentrypoint.h"
#include "services/gmail/gmailfeed.h"
#include "services/gmail/gui/formaddeditemail.h"
#include "services/gmail/gui/formdownloadattachment.h"
#include "services/gmail/gui/formeditgmailaccount.h"
#include "services/gmail/network/gmailnetworkfactory.h"

#include <QFileDialog>

GmailServiceRoot::GmailServiceRoot(RootItem* parent)
  : ServiceRoot(parent), m_network(new GmailNetworkFactory(this)), m_actionReply(nullptr) {
  m_network->setService(this);
  setIcon(GmailEntryPoint().icon());
}

void GmailServiceRoot::updateTitle() {
  setTitle(TextFactory::extractUsernameFromEmail(m_network->username()) + QSL(" (Gmail)"));
}

void GmailServiceRoot::replyToEmail() {
  FormAddEditEmail(this, qApp->mainFormWidget()).execForReply(&m_replyToMessage);
}

RootItem* GmailServiceRoot::obtainNewTreeForSyncIn() const {
  auto* root = new RootItem();
  GmailFeed* inbox = new GmailFeed(tr("Inbox"), QSL(GMAIL_SYSTEM_LABEL_INBOX), qApp->icons()->fromTheme(QSL("mail-inbox")), root);

  inbox->setKeepOnTop(true);

  root->appendChild(inbox);
  root->appendChild(new GmailFeed(tr("Sent"), QSL(GMAIL_SYSTEM_LABEL_SENT), qApp->icons()->fromTheme(QSL("mail-sent")), root));
  root->appendChild(new GmailFeed(tr("Drafts"), QSL(GMAIL_SYSTEM_LABEL_DRAFT), qApp->icons()->fromTheme(QSL("gtk-edit")), root));
  root->appendChild(new GmailFeed(tr("Spam"), QSL(GMAIL_SYSTEM_LABEL_SPAM), qApp->icons()->fromTheme(QSL("mail-mark-junk")), root));

  return root;
}

void GmailServiceRoot::writeNewEmail() {
  FormAddEditEmail(this, qApp->mainFormWidget()).execForAdd();
}

void GmailServiceRoot::loadFromDatabase() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());
  Assignment categories = DatabaseQueries::getCategories<Category>(database, accountId());
  Assignment feeds = DatabaseQueries::getFeeds<GmailFeed>(database, qApp->feedReader()->messageFilters(), accountId());
  auto labels = DatabaseQueries::getLabels(database, accountId());

  performInitialAssembly(categories, feeds, labels);

  for (RootItem* feed : childItems()) {
    if (feed->customId() == QL1S("INBOX")) {
      feed->setKeepOnTop(true);
    }
  }
}

void GmailServiceRoot::saveAccountDataToDatabase(bool creating_new) {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (!creating_new) {
    if (DatabaseQueries::overwriteGmailAccount(database, m_network->username(),
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
    if (DatabaseQueries::createGmailAccount(database,
                                            accountId(),
                                            m_network->username(),
                                            m_network->oauth()->clientId(),
                                            m_network->oauth()->clientSecret(),
                                            m_network->oauth()->redirectUrl(),
                                            m_network->oauth()->refreshToken(),
                                            m_network->batchSize())) {
      updateTitle();
    }
  }
}

bool GmailServiceRoot::downloadAttachmentOnMyOwn(const QUrl& url) const {
  QString str_url = url.toString();
  QString attachment_id = str_url.mid(str_url.indexOf(QL1C('?')) + 1);
  QStringList parts = attachment_id.split(QL1S(GMAIL_ATTACHMENT_SEP));
  QString file = QFileDialog::getSaveFileName(qApp->mainFormWidget(), tr("Select attachment destination file"),
                                              qApp->homeFolder() + QDir::separator() + parts.at(0));

  if (!file.isEmpty() && parts.size() == 3) {
    Downloader* down = network()->downloadAttachment(parts.at(1), parts.at(2), networkProxy());
    FormDownloadAttachment form(file, down, qApp->mainFormWidget());

    form.exec();
    return true;
  }
  else {
    return false;
  }
}

QList<QAction*> GmailServiceRoot::contextMenuMessagesList(const QList<Message>& messages) {
  if (messages.size() == 1) {
    m_replyToMessage = messages.at(0);

    if (m_actionReply == nullptr) {
      m_actionReply = new QAction(qApp->icons()->fromTheme(QSL("mail-reply-sender")), tr("Reply to this message"), this);
      connect(m_actionReply, &QAction::triggered, this, &GmailServiceRoot::replyToEmail);
    }

    return { m_actionReply };
  }
  else {
    return {};
  }
}

QList<QAction*> GmailServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    ServiceRoot::serviceMenu();

    QAction* act_new_email = new QAction(qApp->icons()->fromTheme(QSL("mail-message-new")), tr("Write new e-mail message"), this);

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
  Q_UNUSED(freshly_activated)

  loadFromDatabase();
  loadCacheFromFile();

  if (childCount() <= 3) {
    syncIn();
  }
  else {
    m_network->oauth()->login();
  }
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

void GmailServiceRoot::saveAllCachedData(bool ignore_errors) {
  auto msg_cache = takeMessageCache();
  QMapIterator<RootItem::ReadStatus, QStringList> i(msg_cache.m_cachedStatesRead);

  // Save the actual data read/unread.
  while (i.hasNext()) {
    i.next();
    auto key = i.key();
    QStringList ids = i.value();

    if (!ids.isEmpty()) {
      if (network()->markMessagesRead(key, ids, networkProxy()) !=
          QNetworkReply::NetworkError::NoError &&
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

      if (network()->markMessagesStarred(key, custom_ids, networkProxy()) !=
          QNetworkReply::NetworkError::NoError &&
          !ignore_errors) {
        addMessageStatesToCache(messages, key);
      }
    }
  }
}

bool GmailServiceRoot::canBeDeleted() const {
  return true;
}

bool GmailServiceRoot::deleteViaGui() {
  QSqlDatabase database = qApp->database()->connection(metaObject()->className());

  if (DatabaseQueries::deleteGmailAccount(database, accountId())) {
    return ServiceRoot::deleteViaGui();
  }
  else {
    return false;
  }
}
