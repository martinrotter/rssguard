// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/tt-rss/gui/formttrssfeeddetails.h"

#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssserviceroot.h"
#include "services/tt-rss/ttrsscategory.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "miscellaneous/application.h"

#include <QTimer>


FormTtRssFeedDetails::FormTtRssFeedDetails(ServiceRoot *service_root, QWidget *parent)
  : FormFeedDetails(service_root, parent) {
  m_ui->m_spinAutoUpdateInterval->setEnabled(false);
  m_ui->m_cmbAutoUpdateType->setEnabled(false);
  m_ui->m_cmbType->setEnabled(false);
  m_ui->m_cmbEncoding->setEnabled(false);
  m_ui->m_btnFetchMetadata->setEnabled(false);
  m_ui->m_btnIcon->setEnabled(false);
  m_ui->m_txtTitle->setEnabled(false);
  m_ui->m_txtDescription->setEnabled(false);
}

void FormTtRssFeedDetails::apply() {
  if (m_editableFeed != nullptr) {
    // User edited auto-update status. Save it.
    TtRssFeed *new_feed_data = new TtRssFeed();

    new_feed_data->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(m_ui->m_cmbAutoUpdateType->currentIndex()).toInt()));
    new_feed_data->setAutoUpdateInitialInterval(m_ui->m_spinAutoUpdateInterval->value());

    qobject_cast<TtRssFeed*>(m_editableFeed)->editItself(new_feed_data);

    delete new_feed_data;
  }
  else {
    RootItem *parent = static_cast<RootItem*>(m_ui->m_cmbParentCategory->itemData(m_ui->m_cmbParentCategory->currentIndex()).value<void*>());
    TtRssServiceRoot *root = parent->kind() == RootItemKind::Category ?
                               qobject_cast<TtRssCategory*>(parent)->serviceRoot() :
                               qobject_cast<TtRssServiceRoot*>(parent);
    const int category_id = parent->kind() == RootItemKind::ServiceRoot ?
                              0 :
                              qobject_cast<TtRssCategory*>(parent)->customId();
    const TtRssSubscribeToFeedResponse response = root->network()->subscribeToFeed(m_ui->m_txtUrl->lineEdit()->text(),
                                                                                   category_id,
                                                                                   m_ui->m_gbAuthentication->isChecked(),
                                                                                   m_ui->m_txtUsername->lineEdit()->text(),
                                                                                   m_ui->m_txtPassword->lineEdit()->text());

    if (response.code() == STF_INSERTED) {
      // Feed was added online.
      accept();
      qApp->showGuiMessage(tr("Feed added"), tr("Feed was added, triggering sync in now."), QSystemTrayIcon::Information);
      QTimer::singleShot(100, root, SLOT(syncIn()));
    }
    else {
      reject();
      qApp->showGuiMessage(tr("Cannot add feed"),
                           tr("Feed was not added due to error."),
                           QSystemTrayIcon::Critical, qApp->mainFormWidget(), true);
    }
  }

  accept();
}

void FormTtRssFeedDetails::setEditableFeed(Feed *editable_feed) {
  m_ui->m_cmbAutoUpdateType->setEnabled(true);

  FormFeedDetails::setEditableFeed(editable_feed);

  // Tiny Tiny RSS does not support editing of these features.
  // User can edit only individual auto-update statuses.
  m_ui->m_gbAuthentication->setEnabled(false);
  m_ui->m_txtUrl->setEnabled(false);
  m_ui->m_lblParentCategory->setEnabled(false);
  m_ui->m_cmbParentCategory->setEnabled(false);
}
