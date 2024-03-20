// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formeditnextcloudaccount.h"

#include "src/gui/nextcloudaccountdetails.h"
#include "src/nextcloudnetworkfactory.h"
#include "src/nextcloudserviceroot.h"

#include <librssguard/miscellaneous/iconfactory.h>

FormEditNextcloudAccount::FormEditNextcloudAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("nextcloud")), parent),
    m_details(new NextcloudAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditNextcloudAccount::performTest);

  m_details->m_ui.m_txtUrl->setFocus();
}

void FormEditNextcloudAccount::apply() {
  FormAccountDetails::apply();

  bool using_another_acc =
    m_details->m_ui.m_txtUsername->lineEdit()->text() != account<NextcloudServiceRoot>()->network()->authUsername() ||
    m_details->m_ui.m_txtUrl->lineEdit()->text() != account<NextcloudServiceRoot>()->network()->url();

  account<NextcloudServiceRoot>()->network()->setUrl(m_details->m_ui.m_txtUrl->lineEdit()->text());
  account<NextcloudServiceRoot>()->network()->setAuthUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<NextcloudServiceRoot>()->network()->setAuthPassword(m_details->m_ui.m_txtPassword->lineEdit()->text());
  account<NextcloudServiceRoot>()->network()->setForceServerSideUpdate(m_details->m_ui.m_checkServerSideUpdate
                                                                         ->isChecked());
  account<NextcloudServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  account<NextcloudServiceRoot>()
    ->network()
    ->setDownloadOnlyUnreadMessages(m_details->m_ui.m_checkDownloadOnlyUnreadMessages->isChecked());

  account<NextcloudServiceRoot>()->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew && using_another_acc) {
    account<NextcloudServiceRoot>()->completelyRemoveAllData();
    account<NextcloudServiceRoot>()->start(true);
  }
}

void FormEditNextcloudAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  NextcloudServiceRoot* existing_root = account<NextcloudServiceRoot>();

  m_details->m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->authUsername());
  m_details->m_ui.m_txtPassword->lineEdit()->setText(existing_root->network()->authPassword());
  m_details->m_ui.m_txtUrl->lineEdit()->setText(existing_root->network()->url());
  m_details->m_ui.m_checkDownloadOnlyUnreadMessages->setChecked(existing_root->network()->downloadOnlyUnreadMessages());
  m_details->m_ui.m_checkServerSideUpdate->setChecked(existing_root->network()->forceServerSideUpdate());
  m_details->m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());
}

void FormEditNextcloudAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
