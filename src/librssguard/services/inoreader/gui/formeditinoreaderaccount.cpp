// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/inoreader/gui/formeditinoreaderaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "network-web/webfactory.h"
#include "services/inoreader/definitions.h"
#include "services/inoreader/gui/inoreaderaccountdetails.h"
#include "services/inoreader/inoreaderserviceroot.h"
#include "services/inoreader/network/inoreadernetworkfactory.h"

#include <QThread>

FormEditInoreaderAccount::FormEditInoreaderAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("inoreader")), parent), m_details(new InoreaderAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  m_details->m_ui.m_txtUsername->setFocus();
}

void FormEditInoreaderAccount::apply() {
  bool editing_account = !applyInternal<InoreaderServiceRoot>();

  if (!editing_account) {
    // We transfer refresh token to avoid the need to login once more,
    // then we delete testing OAuth service.
    account<InoreaderServiceRoot>()->network()->oauth()->setRefreshToken(m_details->m_oauth->refreshToken());
    account<InoreaderServiceRoot>()->network()->oauth()->setAccessToken(m_details->m_oauth->accessToken());
    account<InoreaderServiceRoot>()->network()->oauth()->setTokensExpireIn(m_details->m_oauth->tokensExpireIn());
    m_details->m_oauth->logout(true);
    m_details->m_oauth->deleteLater();
  }

  account<InoreaderServiceRoot>()->network()->oauth()->setClientId(m_details->m_ui.m_txtAppId->lineEdit()->text());
  account<InoreaderServiceRoot>()->network()->oauth()->setClientSecret(m_details->m_ui.m_txtAppKey->lineEdit()->text());
  account<InoreaderServiceRoot>()->network()->oauth()->setRedirectUrl(m_details->m_ui.m_txtRedirectUrl->lineEdit()->text());

  account<InoreaderServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<InoreaderServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());

  account<InoreaderServiceRoot>()->saveAccountDataToDatabase(!editing_account);
  accept();

  if (editing_account) {
    account<InoreaderServiceRoot>()->completelyRemoveAllData();
    account<InoreaderServiceRoot>()->syncIn();
  }
}

void FormEditInoreaderAccount::setEditableAccount(ServiceRoot* editable_account) {
  FormAccountDetails::setEditableAccount(editable_account);

  if (m_details->m_oauth != nullptr) {
    // We will use live OAuth service for testing.
    m_details->m_oauth->logout(true);
    delete m_details->m_oauth;
    m_details->m_oauth = nullptr;
  }

  m_details->m_oauth = account<InoreaderServiceRoot>()->network()->oauth();
  m_details->hookNetwork();

  // Setup the GUI.
  m_details->m_ui.m_txtAppId->lineEdit()->setText(m_details->m_oauth->clientId());
  m_details->m_ui.m_txtAppKey->lineEdit()->setText(m_details->m_oauth->clientSecret());
  m_details->m_ui.m_txtRedirectUrl->lineEdit()->setText(m_details->m_oauth->redirectUrl());

  m_details->m_ui.m_txtUsername->lineEdit()->setText(account<InoreaderServiceRoot>()->network()->username());
  m_details->m_ui.m_spinLimitMessages->setValue(account<InoreaderServiceRoot>()->network()->batchSize());
}
