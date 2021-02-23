// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/gmail/gui/formeditgmailaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/webfactory.h"
#include "services/gmail/definitions.h"
#include "services/gmail/gmailserviceroot.h"
#include "services/gmail/gui/gmailaccountdetails.h"

FormEditGmailAccount::FormEditGmailAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("gmail")), parent), m_details(new GmailAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  m_details->m_ui.m_txtUsername->setFocus();
}

void FormEditGmailAccount::apply() {
  bool editing_account = !applyInternal<GmailServiceRoot>();

  if (!editing_account) {
    // We transfer refresh token to avoid the need to login once more,
    // then we delete testing OAuth service.
    account<GmailServiceRoot>()->network()->oauth()->setRefreshToken(m_details->m_oauth->refreshToken());
    account<GmailServiceRoot>()->network()->oauth()->setAccessToken(m_details->m_oauth->accessToken());
    account<GmailServiceRoot>()->network()->oauth()->setTokensExpireIn(m_details->m_oauth->tokensExpireIn());
    m_details->m_oauth->logout(true);
    m_details->m_oauth->deleteLater();
  }

  account<GmailServiceRoot>()->network()->oauth()->setClientId(m_details->m_ui.m_txtAppId->lineEdit()->text());
  account<GmailServiceRoot>()->network()->oauth()->setClientSecret(m_details->m_ui.m_txtAppKey->lineEdit()->text());
  account<GmailServiceRoot>()->network()->oauth()->setRedirectUrl(m_details->m_ui.m_txtRedirectUrl->lineEdit()->text());

  account<GmailServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<GmailServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());

  account<GmailServiceRoot>()->saveAccountDataToDatabase(!editing_account);
  accept();

  if (editing_account) {
    account<GmailServiceRoot>()->completelyRemoveAllData();
    account<GmailServiceRoot>()->syncIn();
  }
}

void FormEditGmailAccount::setEditableAccount(ServiceRoot* editable_account) {
  FormAccountDetails::setEditableAccount(editable_account);

  if (m_details->m_oauth != nullptr) {
    // We will use live OAuth service for testing.
    m_details->m_oauth->logout(true);
    m_details->m_oauth->deleteLater();
  }

  m_details->m_oauth = account<GmailServiceRoot>()->network()->oauth();
  m_details->hookNetwork();

  // Setup the GUI.
  m_details->m_ui.m_txtAppId->lineEdit()->setText(m_details->m_oauth->clientId());
  m_details->m_ui.m_txtAppKey->lineEdit()->setText(m_details->m_oauth->clientSecret());
  m_details->m_ui.m_txtRedirectUrl->lineEdit()->setText(m_details->m_oauth->redirectUrl());

  m_details->m_ui.m_txtUsername->lineEdit()->setText(account<GmailServiceRoot>()->network()->username());
  m_details->m_ui.m_spinLimitMessages->setValue(account<GmailServiceRoot>()->network()->batchSize());
}
