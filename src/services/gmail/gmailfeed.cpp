// For license of this file, see <object-root-folder>/LICENSE.md.

#include "services/gmail/gmailfeed.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/gmail/gmailserviceroot.h"
#include "services/gmail/network/gmailnetworkfactory.h"

GmailFeed::GmailFeed(RootItem* parent) : Feed(parent) {}

GmailFeed::GmailFeed(const QString& title, const QString& custom_id, const QIcon& icon, RootItem* parent) : GmailFeed(parent) {
  setTitle(title);
  setCustomId(custom_id);
  setIcon(icon);
}

GmailFeed::GmailFeed(const QSqlRecord& record) : Feed(record) {}

GmailServiceRoot* GmailFeed::serviceRoot() const {
  return qobject_cast<GmailServiceRoot*>(getParentServiceRoot());
}

QList<Message> GmailFeed::obtainNewMessages(bool* error_during_obtaining) {
  Feed::Status error;

  // TODO: dodělat
  QList<Message> messages;/* = serviceRoot()->network()->messages(customId(), error);

                             setStatus(error);

                             if (error == Feed::Status::NetworkError || error == Feed::Status::AuthError) {
                           * error_during_obtaining = true;
                             }*/

  return messages;
}
