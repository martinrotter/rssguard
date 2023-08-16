// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/owncloud/gui/formeditowncloudaccount.h"

#include "miscellaneous/iconfactory.h"
#include "services/owncloud/gui/owncloudaccountdetails.h"
#include "services/owncloud/owncloudnetworkfactory.h"
#include "services/owncloud/owncloudserviceroot.h"

FormEditOwnCloudAccount::FormEditOwnCloudAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("nextcloud")), parent), m_details(new OwnCloudAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditOwnCloudAccount::performTest);

  m_details->m_ui.m_txtUrl->setFocus();
}

void FormEditOwnCloudAccount::apply() {
  FormAccountDetails::apply();

  bool using_another_acc =
    m_details->m_ui.m_txtUsername->lineEdit()->text() != account<OwnCloudServiceRoot>()->network()->authUsername() ||
    m_details->m_ui.m_txtUrl->lineEdit()->text() != account<OwnCloudServiceRoot>()->network()->url();

  account<OwnCloudServiceRoot>()->network()->setUrl(m_details->m_ui.m_txtUrl->lineEdit()->text());
  account<OwnCloudServiceRoot>()->network()->setAuthUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<OwnCloudServiceRoot>()->network()->setAuthPassword(m_details->m_ui.m_txtPassword->lineEdit()->text());
  account<OwnCloudServiceRoot>()->network()->setForceServerSideUpdate(m_details->m_ui.m_checkServerSideUpdate
                                                                        ->isChecked());
  account<OwnCloudServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  account<OwnCloudServiceRoot>()
    ->network()
    ->setDownloadOnlyUnreadMessages(m_details->m_ui.m_checkDownloadOnlyUnreadMessages->isChecked());

  account<OwnCloudServiceRoot>()->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew && using_another_acc) {
    account<OwnCloudServiceRoot>()->completelyRemoveAllData();
    account<OwnCloudServiceRoot>()->start(true);
  }
}

void FormEditOwnCloudAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  OwnCloudServiceRoot* existing_root = account<OwnCloudServiceRoot>();

  m_details->m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->authUsername());
  m_details->m_ui.m_txtPassword->lineEdit()->setText(existing_root->network()->authPassword());
  m_details->m_ui.m_txtUrl->lineEdit()->setText(existing_root->network()->url());
  m_details->m_ui.m_checkDownloadOnlyUnreadMessages->setChecked(existing_root->network()->downloadOnlyUnreadMessages());
  m_details->m_ui.m_checkServerSideUpdate->setChecked(existing_root->network()->forceServerSideUpdate());
  m_details->m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());
}

void FormEditOwnCloudAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
