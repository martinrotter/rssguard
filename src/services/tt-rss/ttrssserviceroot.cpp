// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/tt-rss/ttrssserviceroot.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "gui/dialogs/formmain.h"
#include "services/tt-rss/ttrssserviceentrypoint.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/gui/formeditaccount.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QPointer>


TtRssServiceRoot::TtRssServiceRoot(RootItem *parent)
  : ServiceRoot(parent), m_actionSyncIn(NULL), m_serviceMenu(QList<QAction*>()), m_network(new TtRssNetworkFactory) {
  setIcon(TtRssServiceEntryPoint().icon());
  setCreationDate(QDateTime::currentDateTime());
}

TtRssServiceRoot::~TtRssServiceRoot() {
  if (m_network != NULL) {
    delete m_network;
  }
}

void TtRssServiceRoot::start() {
  if (childItems().isEmpty()) {
    syncIn();
  }
}

void TtRssServiceRoot::stop() {

}

QString TtRssServiceRoot::code() {
  return SERVICE_CODE_TT_RSS;
}

bool TtRssServiceRoot::editViaGui() {
  QPointer<FormEditAccount> form_pointer = new FormEditAccount(qApp->mainForm());
  form_pointer.data()->execForEdit(this);
  delete form_pointer.data();
  return false;
}

bool TtRssServiceRoot::deleteViaGui() {
  QSqlDatabase connection = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);

  // Remove extra entry in "Tiny Tiny RSS accounts list" and then delete
  // all the categories/feeds and messages.
  if (!QSqlQuery(connection).exec(QString("DELETE FROM TtRssAccounts WHERE id = %1;").arg(accountId()))) {
    return false;
  }
  else {
    return ServiceRoot::deleteViaGui();
  }
}

bool TtRssServiceRoot::canBeEdited() {
  return true;
}

bool TtRssServiceRoot::canBeDeleted() {
  return true;
}

QVariant TtRssServiceRoot::data(int column, int role) const {
  switch (role) {
    case Qt::ToolTipRole:
      if (column == FDS_MODEL_TITLE_INDEX) {
        return tr("Tiny Tiny RSS\n\nAccount ID: %3\nUsername: %1\nServer: %2").arg(m_network->username(),
                                                                                   m_network->url(),
                                                                                   QString::number(accountId()));
      }
      else {
        return ServiceRoot::data(column, role);
      }

    default:
      return ServiceRoot::data(column, role);
  }
}

QList<QAction*> TtRssServiceRoot::addItemMenu() {
  return QList<QAction*>();
}

RecycleBin *TtRssServiceRoot::recycleBin() {
  return NULL;
}

bool TtRssServiceRoot::loadMessagesForItem(RootItem *item, QSqlTableModel *model) {
  return false;
}

QList<QAction*> TtRssServiceRoot::serviceMenu() {
  if (m_serviceMenu.isEmpty()) {
    m_actionSyncIn = new QAction(qApp->icons()->fromTheme(QSL("item-sync")), tr("Sync in"), this);

    connect(m_actionSyncIn, SIGNAL(triggered()), this, SLOT(syncIn()));

    m_serviceMenu.append(m_actionSyncIn);
  }

  return m_serviceMenu;
}

QList<QAction*> TtRssServiceRoot::contextMenu() {
  return serviceMenu();
}

bool TtRssServiceRoot::onBeforeSetMessagesRead(RootItem *selected_item, QList<int> message_db_ids, RootItem::ReadStatus read) {
  return false;
}

bool TtRssServiceRoot::onAfterSetMessagesRead(RootItem *selected_item, QList<int> message_db_ids, RootItem::ReadStatus read) {
  return false;
}

bool TtRssServiceRoot::onBeforeSwitchMessageImportance(RootItem *selected_item, QList<QPair<int, RootItem::Importance> > changes) {
  return false;
}

bool TtRssServiceRoot::onAfterSwitchMessageImportance(RootItem *selected_item, QList<QPair<int, RootItem::Importance> > changes) {
  return false;
}

bool TtRssServiceRoot::onBeforeMessagesDelete(RootItem *selected_item, QList<int> message_db_ids) {
  return false;
}

bool TtRssServiceRoot::onAfterMessagesDelete(RootItem *selected_item, QList<int> message_db_ids) {
  return false;
}

bool TtRssServiceRoot::onBeforeMessagesRestoredFromBin(RootItem *selected_item, QList<int> message_db_ids) {
  return false;
}

bool TtRssServiceRoot::onAfterMessagesRestoredFromBin(RootItem *selected_item, QList<int> message_db_ids) {
  return false;
}

TtRssNetworkFactory *TtRssServiceRoot::network() const {
  return m_network;
}

void TtRssServiceRoot::saveToDatabase() {
  if (accountId() != NO_PARENT_CATEGORY) {
    // We are overwritting previously saved data.
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
    QSqlQuery query(database);

    query.prepare("UPDATE TtRssAccounts "
                  "SET username = :username, password = :password, url = :url "
                  "WHERE id = :id;");
    query.bindValue(":username", m_network->username());
    query.bindValue(":password", m_network->password());
    query.bindValue(":url", m_network->url());
    query.bindValue(":id", accountId());

    if (query.exec()) {
      updateTitle();
      itemChanged(QList<RootItem*>() << this);
    }
  }
  else {
    // We are probably saving newly added account.
    QSqlDatabase database = qApp->database()->connection(metaObject()->className(), DatabaseFactory::FromSettings);
    QSqlQuery query(database);

    // First obtain the ID, which can be assigned to this new account.
    if (!query.exec("SELECT max(id) FROM Accounts;") || !query.next()) {
      return;
    }

    int id_to_assing = query.value(0).toInt() + 1;

    bool saved = query.exec(QString("INSERT INTO Accounts (id, type) VALUES (%1, '%2');").arg(QString::number(id_to_assing),
                                                                                              SERVICE_CODE_TT_RSS)) &&
                 query.exec(QString("INSERT INTO TtRssAccounts (id, username, password, url) VALUES (%1, '%2', '%3', '%4');").arg(QString::number(id_to_assing),
                                                                                                                                  network()->username(),
                                                                                                                                  network()->password(),
                                                                                                                                  network()->url()));

    if (saved) {
      setAccountId(id_to_assing);
      updateTitle();
    }
  }
}

void TtRssServiceRoot::loadFromDatabase() {
  // TODO: Load feeds/categories from DB.
}

void TtRssServiceRoot::updateTitle() {
  QString host = QUrl(m_network->url()).host();

  if (host.isEmpty()) {
    host = m_network->url();
  }

  setTitle(m_network->username() + QL1S("@") + host);
}

void TtRssServiceRoot::syncIn() {
  // TODO: provede stažení kanálů/kategorií
  // ze serveru, a sloučení s aktuálními
  // neprovádí aktualizace kanálů ani stažení počtu nepřečtených zpráv
  QNetworkReply::NetworkError err;


  QList<RootItem*> aa = m_network->getFeedsCategories(err).feedsCategories();
}
