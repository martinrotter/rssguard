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

#include "services/tt-rss/gui/formeditfeed.h"

#include "services/abstract/category.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssfeed.h"
#include "services/tt-rss/ttrsscategory.h"
#include "services/tt-rss/ttrssserviceroot.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/application.h"

#include <QClipboard>
#include <QMimeData>
#include <QTimer>


FormEditFeed::FormEditFeed(TtRssServiceRoot *root, QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormEditFeed), m_root(root), m_loadedFeed(NULL) {
  m_ui->setupUi(this);
  initialize();

  connect(m_ui->m_txtUrl->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(onUrlChanged(QString)));
  connect(m_ui->m_gbAuthentication, SIGNAL(toggled(bool)), this, SLOT(onAuthenticationSwitched()));
  connect(m_ui->m_cmbAutoUpdateType, SIGNAL(currentIndexChanged(int)), this, SLOT(onAutoUpdateTypeChanged(int)));
  connect(m_ui->m_buttonBox, SIGNAL(accepted()), this, SLOT(performAction()));
}

FormEditFeed::~FormEditFeed() {
}

int FormEditFeed::execForEdit(TtRssFeed *input_feed) {
  loadCategories(m_root->getSubTreeCategories(), m_root);
  loadFeed(input_feed);
  m_ui->m_txtUrl->lineEdit()->setFocus();

  return QDialog::exec();
}

int FormEditFeed::execForAdd(const QString &url) {
  if (!url.isEmpty()) {
    m_ui->m_txtUrl->lineEdit()->setText(url);
  }
  else if (Application::clipboard()->mimeData()->hasText()) {
    m_ui->m_txtUrl->lineEdit()->setText(Application::clipboard()->text());
  }

  loadCategories(m_root->getSubTreeCategories(), m_root);
  loadFeed(NULL);
  m_ui->m_txtUrl->lineEdit()->setFocus();

  return QDialog::exec();
}

void FormEditFeed::onAuthenticationSwitched() {
  onUsernameChanged(m_ui->m_txtUsername->lineEdit()->text());
  onPasswordChanged(m_ui->m_txtPassword->lineEdit()->text());
}

void FormEditFeed::onAutoUpdateTypeChanged(int new_index) {
  const Feed::AutoUpdateType auto_update_type = static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(new_index).toInt());

  switch (auto_update_type) {
    case Feed::DontAutoUpdate:
    case Feed::DefaultAutoUpdate:
      m_ui->m_spinAutoUpdateInterval->setEnabled(false);
      break;

    case Feed::SpecificAutoUpdate:
    default:
      m_ui->m_spinAutoUpdateInterval->setEnabled(true);
  }
}

void FormEditFeed::performAction() {
  if (m_loadedFeed != NULL) {
    // Edit given feed.
    saveFeed();
  }
  else {
    addNewFeed();
  }

  accept();
}

void FormEditFeed::onUrlChanged(const QString &new_url) {
  if (QRegExp(URL_REGEXP).exactMatch(new_url)) {
    // New url is well-formed.
    m_ui->m_txtUrl->setStatus(LineEditWithStatus::Ok, tr("The URL is ok."));
  }
  else if (!new_url.isEmpty()) {
    // New url is not well-formed but is not empty on the other hand.
    m_ui->m_txtUrl->setStatus(LineEditWithStatus::Warning, tr("The URL does not meet standard pattern. Does your URL start with \"http://\" or \"https://\" prefix."));
  }
  else {
    // New url is empty.
    m_ui->m_txtUrl->setStatus(LineEditWithStatus::Error, tr("The URL is empty."));
  }
}

void FormEditFeed::onUsernameChanged(const QString &new_username) {
  const bool is_username_ok = !m_ui->m_gbAuthentication->isChecked() || !new_username.isEmpty();

  m_ui->m_txtUsername->setStatus(is_username_ok ?
                                   LineEditWithStatus::Ok :
                                   LineEditWithStatus::Warning,
                                 is_username_ok ?
                                   tr("Username is ok or it is not needed.") :
                                   tr("Username is empty."));
}

void FormEditFeed::onPasswordChanged(const QString &new_password) {
  const bool is_password_ok = !m_ui->m_gbAuthentication->isChecked() || !new_password.isEmpty();

  m_ui->m_txtPassword->setStatus(is_password_ok ?
                                   LineEditWithStatus::Ok :
                                   LineEditWithStatus::Warning,
                                 is_password_ok ?
                                   tr("Password is ok or it is not needed.") :
                                   tr("Password is empty."));
}

void FormEditFeed::initialize() {
  setWindowIcon(qApp->icons()->fromTheme(QSL("application-rss+xml")));
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint | Qt::WindowTitleHint);

  // Setup auto-update options.
  m_ui->m_spinAutoUpdateInterval->setValue(DEFAULT_AUTO_UPDATE_INTERVAL);
  m_ui->m_cmbAutoUpdateType->addItem(tr("Auto-update using global interval"), QVariant::fromValue((int) Feed::DefaultAutoUpdate));
  m_ui->m_cmbAutoUpdateType->addItem(tr("Auto-update every"), QVariant::fromValue((int) Feed::SpecificAutoUpdate));
  m_ui->m_cmbAutoUpdateType->addItem(tr("Do not auto-update at all"), QVariant::fromValue((int) Feed::DontAutoUpdate));

  setTabOrder(m_ui->m_txtUrl->lineEdit(), m_ui->m_cmbAutoUpdateType);
  setTabOrder(m_ui->m_cmbAutoUpdateType, m_ui->m_spinAutoUpdateInterval);
  setTabOrder(m_ui->m_spinAutoUpdateInterval, m_ui->m_gbAuthentication);
  setTabOrder(m_ui->m_gbAuthentication, m_ui->m_txtUsername->lineEdit());
  setTabOrder(m_ui->m_txtUsername->lineEdit(), m_ui->m_txtPassword->lineEdit());
  setTabOrder(m_ui->m_txtPassword->lineEdit(), m_ui->m_buttonBox);

  m_ui->m_txtUrl->lineEdit()->setPlaceholderText(tr("Full feed url including scheme"));
  m_ui->m_txtUsername->lineEdit()->setPlaceholderText(tr("Username"));
  m_ui->m_txtPassword->lineEdit()->setPlaceholderText(tr("Password"));

  onAuthenticationSwitched();
}

void FormEditFeed::loadFeed(TtRssFeed *input_feed) {
  m_loadedFeed = input_feed;

  if (input_feed != NULL) {
    setWindowTitle(tr("Edit existing feed"));

    // Tiny Tiny RSS does not support editing of these features.
    // User can edit only individual auto-update statuses.
    m_ui->m_gbAuthentication->setEnabled(false);
    m_ui->m_txtUrl->setEnabled(false);
    m_ui->m_lblUrl->setEnabled(false);
    m_ui->m_lblParentCategory->setEnabled(false);
    m_ui->m_cmbParentCategory->setEnabled(false);

    m_ui->m_cmbParentCategory->setCurrentIndex(m_ui->m_cmbParentCategory->findData(QVariant::fromValue((void*) input_feed->parent())));
    m_ui->m_cmbAutoUpdateType->setCurrentIndex(m_ui->m_cmbAutoUpdateType->findData(QVariant::fromValue((int) input_feed->autoUpdateType())));
    m_ui->m_spinAutoUpdateInterval->setValue(input_feed->autoUpdateInitialInterval());
  }
  else {
    setWindowTitle(tr("Add new feed"));

    // Tiny Tiny RSS does not support editing of these features.
    // User can edit only individual auto-update statuses.
    m_ui->m_gbAuthentication->setEnabled(true);
    m_ui->m_txtUrl->setEnabled(true);
    m_ui->m_lblUrl->setEnabled(true);
    m_ui->m_lblParentCategory->setEnabled(true);
    m_ui->m_cmbParentCategory->setEnabled(true);
  }
}

void FormEditFeed::saveFeed() {
  // User edited auto-update status. Save it.
  TtRssFeed *new_feed_data = new TtRssFeed();

  new_feed_data->setAutoUpdateType(static_cast<Feed::AutoUpdateType>(m_ui->m_cmbAutoUpdateType->itemData(m_ui->m_cmbAutoUpdateType->currentIndex()).toInt()));
  new_feed_data->setAutoUpdateInitialInterval(m_ui->m_spinAutoUpdateInterval->value());

  m_loadedFeed->editItself(new_feed_data);
  delete new_feed_data;
}

void FormEditFeed::addNewFeed() {
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
                         QSystemTrayIcon::Critical, qApp->mainForm(), true);
  }
}

void FormEditFeed::loadCategories(const QList<Category*> categories, RootItem *root_item) {
  m_ui->m_cmbParentCategory->addItem(root_item->icon(),
                                     root_item->title(),
                                     QVariant::fromValue((void*) root_item));

  foreach (Category *category, categories) {
    m_ui->m_cmbParentCategory->addItem(category->icon(),
                                       category->title(),
                                       QVariant::fromValue((void*) category));
  }
}
