// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/greader/gui/formeditgreaderaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/greader/definitions.h"
#include "services/greader/greadernetwork.h"
#include "services/greader/greaderserviceroot.h"
#include "services/greader/gui/greaderaccountdetails.h"

FormEditGreaderAccount::FormEditGreaderAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("google")), parent), m_details(new GreaderAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditGreaderAccount::performTest);

  m_details->m_ui.m_txtUrl->setFocus();
}

void FormEditGreaderAccount::apply() {
  FormAccountDetails::apply();

  account<GreaderServiceRoot>()->network()->setBaseUrl(m_details->m_ui.m_txtUrl->lineEdit()->text());
  account<GreaderServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<GreaderServiceRoot>()->network()->setPassword(m_details->m_ui.m_txtPassword->lineEdit()->text());
  account<GreaderServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  account<GreaderServiceRoot>()->network()->setDownloadOnlyUnreadMessages(m_details->m_ui.m_cbDownloadOnlyUnreadMessages->isChecked());
  account<GreaderServiceRoot>()->network()->setService(m_details->service());

  account<GreaderServiceRoot>()->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew) {
    account<GreaderServiceRoot>()->completelyRemoveAllData();
    account<GreaderServiceRoot>()->start(true);
  }
}

void FormEditGreaderAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  GreaderServiceRoot* existing_root = account<GreaderServiceRoot>();

  m_details->setService(existing_root->network()->service());
  m_details->m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->username());
  m_details->m_ui.m_txtPassword->lineEdit()->setText(existing_root->network()->password());
  m_details->m_ui.m_txtUrl->lineEdit()->setText(existing_root->network()->baseUrl());
  m_details->m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());
  m_details->m_ui.m_cbDownloadOnlyUnreadMessages->setChecked(existing_root->network()->downloadOnlyUnreadMessages());
}

void FormEditGreaderAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
