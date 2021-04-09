// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/inoreader/gui/formeditinoreaderaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/webfactory.h"
#include "services/inoreader/definitions.h"
#include "services/inoreader/gui/inoreaderaccountdetails.h"
#include "services/inoreader/inoreadernetworkfactory.h"
#include "services/inoreader/inoreaderserviceroot.h"

#include <QThread>

FormEditInoreaderAccount::FormEditInoreaderAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("inoreader")), parent), m_details(new InoreaderAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  m_details->m_ui.m_txtUsername->setFocus();
}

void FormEditInoreaderAccount::apply() {
  FormAccountDetails::apply();

  account<InoreaderServiceRoot>()->network()->oauth()->logout(false);
  account<InoreaderServiceRoot>()->network()->oauth()->setClientId(m_details->m_ui.m_txtAppId->lineEdit()->text());
  account<InoreaderServiceRoot>()->network()->oauth()->setClientSecret(m_details->m_ui.m_txtAppKey->lineEdit()->text());
  account<InoreaderServiceRoot>()->network()->oauth()->setRedirectUrl(m_details->m_ui.m_txtRedirectUrl->lineEdit()->text());

  account<InoreaderServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<InoreaderServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  account<InoreaderServiceRoot>()->network()->setDownloadOnlyUnreadMessages(m_details->m_ui.m_cbDownloadOnlyUnreadMessages->isChecked());

  account<InoreaderServiceRoot>()->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew) {
    account<InoreaderServiceRoot>()->completelyRemoveAllData();
    account<InoreaderServiceRoot>()->start(true);
  }
}

void FormEditInoreaderAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  m_details->m_oauth = account<InoreaderServiceRoot>()->network()->oauth();
  m_details->hookNetwork();

  // Setup the GUI.
  m_details->m_ui.m_txtAppId->lineEdit()->setText(m_details->m_oauth->clientId());
  m_details->m_ui.m_txtAppKey->lineEdit()->setText(m_details->m_oauth->clientSecret());
  m_details->m_ui.m_txtRedirectUrl->lineEdit()->setText(m_details->m_oauth->redirectUrl());

  m_details->m_ui.m_txtUsername->lineEdit()->setText(account<InoreaderServiceRoot>()->network()->username());
  m_details->m_ui.m_spinLimitMessages->setValue(account<InoreaderServiceRoot>()->network()->batchSize());
  m_details->m_ui.m_cbDownloadOnlyUnreadMessages->setChecked(account<InoreaderServiceRoot>()->network()->downloadOnlyUnreadMessages());
}
