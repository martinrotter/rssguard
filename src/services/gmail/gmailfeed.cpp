// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "services/gmail/gmailfeed.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/gmail/gmailserviceroot.h"
#include "services/gmail/network/gmailnetworkfactory.h"

GmailFeed::GmailFeed(RootItem* parent) : Feed(parent) {}

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
