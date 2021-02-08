// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/gui/formeditfeedlyaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/feedly/definitions.h"
#include "services/feedly/feedlynetwork.h"
#include "services/feedly/feedlyserviceroot.h"
#include "services/feedly/gui/feedlyaccountdetails.h"

#if defined (FEEDLY_OFFICIAL_SUPPORT)
#include "network-web/oauth2service.h"
#endif

FormEditFeedlyAccount::FormEditFeedlyAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("google")), parent), m_details(new FeedlyAccountDetails(this)) {
  insertCustomTab(m_details, tr("Service setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditFeedlyAccount::performTest);
  m_details->m_ui.m_txtUsername->setFocus();
}

void FormEditFeedlyAccount::apply() {
  bool editing_account = !applyInternal<FeedlyServiceRoot>();

#if defined (FEEDLY_OFFICIAL_SUPPORT)
  // We copy credentials from testing OAuth to live OAuth.
  account<FeedlyServiceRoot>()->network()->oauth()->setAccessToken(m_details->m_oauth->accessToken());
  account<FeedlyServiceRoot>()->network()->oauth()->setRefreshToken(m_details->m_oauth->refreshToken());
  account<FeedlyServiceRoot>()->network()->oauth()->setTokensExpireIn(m_details->m_oauth->tokensExpireIn());
#endif

  account<FeedlyServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<FeedlyServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  account<FeedlyServiceRoot>()->network()->setDeveloperAccessToken(m_details->m_ui.m_txtDeveloperAccessToken->lineEdit()->text());

  account<FeedlyServiceRoot>()->saveAccountDataToDatabase(!editing_account);
  accept();

  if (editing_account) {
    account<FeedlyServiceRoot>()->completelyRemoveAllData();
    account<FeedlyServiceRoot>()->syncIn();
  }
}

void FormEditFeedlyAccount::setEditableAccount(ServiceRoot* editable_account) {
  FormAccountDetails::setEditableAccount(editable_account);

  FeedlyServiceRoot* existing_root = account<FeedlyServiceRoot>();

#if defined (FEEDLY_OFFICIAL_SUPPORT)
  m_details->m_oauth = account<FeedlyServiceRoot>()->network()->oauth();
  m_details->hookNetwork();
#endif

  m_details->m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->username());
  m_details->m_ui.m_txtDeveloperAccessToken->lineEdit()->setText(existing_root->network()->developerAccessToken());
  m_details->m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());
}

void FormEditFeedlyAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
