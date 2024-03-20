// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formeditttrssaccount.h"

#include "src/gui/ttrssaccountdetails.h"
#include "src/ttrssnetworkfactory.h"
#include "src/ttrssserviceroot.h"

#include <librssguard/miscellaneous/iconfactory.h>

FormEditTtRssAccount::FormEditTtRssAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("tt-rss")), parent), m_details(new TtRssAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditTtRssAccount::performTest);
  m_details->m_ui.m_txtUrl->setFocus();
}

void FormEditTtRssAccount::apply() {
  FormAccountDetails::apply();

  bool using_another_acc =
    m_details->m_ui.m_txtUsername->lineEdit()->text() != account<TtRssServiceRoot>()->network()->username() ||
    m_details->m_ui.m_txtUrl->lineEdit()->text() != account<TtRssServiceRoot>()->network()->url();

  account<TtRssServiceRoot>()->network()->logout(m_account->networkProxy());
  account<TtRssServiceRoot>()->network()->setUrl(m_details->m_ui.m_txtUrl->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setPassword(m_details->m_ui.m_txtPassword->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setAuthIsUsed(m_details->m_ui.m_gbHttpAuthentication->isChecked());
  account<TtRssServiceRoot>()->network()->setAuthUsername(m_details->m_ui.m_txtHttpUsername->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setAuthPassword(m_details->m_ui.m_txtHttpPassword->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  account<TtRssServiceRoot>()->network()->setIntelligentSynchronization(m_details->m_ui.m_cbNewAlgorithm->isChecked());
  account<TtRssServiceRoot>()->network()->setForceServerSideUpdate(m_details->m_ui.m_checkServerSideUpdate
                                                                     ->isChecked());
  account<TtRssServiceRoot>()
    ->network()
    ->setDownloadOnlyUnreadMessages(m_details->m_ui.m_checkDownloadOnlyUnreadMessages->isChecked());

  account<TtRssServiceRoot>()->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew && using_another_acc) {
    account<TtRssServiceRoot>()->completelyRemoveAllData();
    account<TtRssServiceRoot>()->start(true);
  }
}

void FormEditTtRssAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  TtRssServiceRoot* existing_root = account<TtRssServiceRoot>();

  m_details->m_ui.m_gbHttpAuthentication->setChecked(existing_root->network()->authIsUsed());
  m_details->m_ui.m_txtHttpPassword->lineEdit()->setText(existing_root->network()->authPassword());
  m_details->m_ui.m_txtHttpUsername->lineEdit()->setText(existing_root->network()->authUsername());
  m_details->m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->username());
  m_details->m_ui.m_txtPassword->lineEdit()->setText(existing_root->network()->password());
  m_details->m_ui.m_txtUrl->lineEdit()->setText(existing_root->network()->url());
  m_details->m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());
  m_details->m_ui.m_checkServerSideUpdate->setChecked(existing_root->network()->forceServerSideUpdate());
  m_details->m_ui.m_checkDownloadOnlyUnreadMessages->setChecked(existing_root->network()->downloadOnlyUnreadMessages());
  m_details->m_ui.m_cbNewAlgorithm->setChecked(existing_root->network()->intelligentSynchronization());
}

void FormEditTtRssAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
