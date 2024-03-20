// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formeditgreaderaccount.h"

#include "src/greadernetwork.h"
#include "src/greaderserviceroot.h"
#include "src/gui/greaderaccountdetails.h"

#include <librssguard/miscellaneous/iconfactory.h>
#include <librssguard/network-web/oauth2service.h>

FormEditGreaderAccount::FormEditGreaderAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("google")), parent), m_details(new GreaderAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditGreaderAccount::performTest);

  m_details->m_ui.m_txtUrl->setFocus();
}

void FormEditGreaderAccount::apply() {
  FormAccountDetails::apply();

  GreaderServiceRoot* existing_root = account<GreaderServiceRoot>();
  bool using_another_acc = m_details->m_ui.m_txtUsername->lineEdit()->text() != existing_root->network()->username() ||
                           m_details->service() != existing_root->network()->service() ||
                           m_details->m_ui.m_txtUrl->lineEdit()->text() != existing_root->network()->baseUrl();

  existing_root->network()->setBaseUrl(m_details->m_ui.m_txtUrl->lineEdit()->text());
  existing_root->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  existing_root->network()->setPassword(m_details->m_ui.m_txtPassword->lineEdit()->text());
  existing_root->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  existing_root->network()->setDownloadOnlyUnreadMessages(m_details->m_ui.m_cbDownloadOnlyUnreadMessages->isChecked());
  existing_root->network()->setService(m_details->service());
  existing_root->network()->setIntelligentSynchronization(m_details->m_ui.m_cbNewAlgorithm->isChecked());
  existing_root->network()->setNewerThanFilter(m_details->m_ui.m_dateNewerThan->date());

  existing_root->network()->oauth()->logout(true);

  if (existing_root->network()->service() == GreaderServiceRoot::Service::Inoreader) {
    existing_root->network()->oauth()->setClientId(m_details->m_ui.m_txtAppId->lineEdit()->text());
    existing_root->network()->oauth()->setClientSecret(m_details->m_ui.m_txtAppKey->lineEdit()->text());
    existing_root->network()->oauth()->setRedirectUrl(m_details->m_ui.m_txtRedirectUrl->lineEdit()->text(), true);
  }

  existing_root->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew) {
    if (using_another_acc) {
      existing_root->completelyRemoveAllData();
    }

    existing_root->start(true);
  }
}

void FormEditGreaderAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  GreaderServiceRoot* existing_root = account<GreaderServiceRoot>();

  setWindowIcon(existing_root->icon());

  m_details->setService(existing_root->network()->service());
  m_details->m_oauth = existing_root->network()->oauth();
  m_details->hookNetwork();

  m_details->m_ui.m_txtAppId->lineEdit()->setText(m_details->m_oauth->clientId());
  m_details->m_ui.m_txtAppKey->lineEdit()->setText(m_details->m_oauth->clientSecret());
  m_details->m_ui.m_txtRedirectUrl->lineEdit()->setText(m_details->m_oauth->redirectUrl());

  m_details->m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->username());
  m_details->m_ui.m_txtPassword->lineEdit()->setText(existing_root->network()->password());
  m_details->m_ui.m_txtUrl->lineEdit()->setText(existing_root->network()->baseUrl());
  m_details->m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());
  m_details->m_ui.m_cbDownloadOnlyUnreadMessages->setChecked(existing_root->network()->downloadOnlyUnreadMessages());
  m_details->m_ui.m_cbNewAlgorithm->setChecked(existing_root->network()->intelligentSynchronization());
  m_details->m_ui.m_dateNewerThan->setDate(existing_root->network()->newerThanFilter());
}

void FormEditGreaderAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
