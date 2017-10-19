// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
// Copyright (C) 2010-2014 by David Rosca <nowrep@gmail.com>
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

#include "services/gmail/gmailserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/databasequeries.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/abstract/recyclebin.h"
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

void GmailServiceRoot::loadFromDatabase() {
  GmailFeed* inbox = new GmailFeed(tr("Inbox"), QSL("INBOX"), qApp->icons()->fromTheme(QSL("mail-inbox")), this);

  inbox->setKeepOnTop(true);

  appendChild(inbox);
  appendChild(new GmailFeed(tr("Sent"), QSL("SENT"), qApp->icons()->fromTheme(QSL("mail-sent")), this));
  appendChild(new GmailFeed(tr("Drafts"), QSL("DRAFT"), qApp->icons()->fromTheme(QSL("gtk-edit")), this));
  appendChild(new GmailFeed(tr("Spam"), QSL("SPAM"), qApp->icons()->fromTheme(QSL("mail-mark-junk")), this));

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
