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
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("feedly")), parent), m_details(new FeedlyAccountDetails(this)) {
  insertCustomTab(m_details, tr("Service setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditFeedlyAccount::performTest);
  m_details->m_ui.m_txtUsername->setFocus();
}

void FormEditFeedlyAccount::apply() {
  bool editing_account = !applyInternal<FeedlyServiceRoot>();

#if defined (FEEDLY_OFFICIAL_SUPPORT)
  if (!editing_account) {
    // We transfer refresh token to avoid the need to login once more,
    // then we delete testing OAuth service.
    account<FeedlyServiceRoot>()->network()->oauth()->setAccessToken(m_details->m_oauth->accessToken());
    account<FeedlyServiceRoot>()->network()->oauth()->setRefreshToken(m_details->m_oauth->refreshToken());
    account<FeedlyServiceRoot>()->network()->oauth()->setTokensExpireIn(m_details->m_oauth->tokensExpireIn());
    m_details->m_oauth->logout(true);
    m_details->m_oauth->deleteLater();

    // Force live OAuth object to re-start it's redirection handler.
    account<FeedlyServiceRoot>()->network()->oauth()->setRedirectUrl(QString(OAUTH_REDIRECT_URI) +
                                                                     QL1C(':') +
                                                                     QString::number(FEEDLY_API_REDIRECT_URI_PORT));
  }
#endif

  account<FeedlyServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<FeedlyServiceRoot>()->network()->setDownloadOnlyUnreadMessages(m_details->m_ui.m_checkDownloadOnlyUnreadMessages->isChecked());
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

#if defined (FEEDLY_OFFICIAL_SUPPORT)
  if (m_details->m_oauth != nullptr) {
    // We will use live OAuth service for testing.
    m_details->m_oauth->logout(true);
    m_details->m_oauth->deleteLater();
  }

  m_details->m_oauth = account<FeedlyServiceRoot>()->network()->oauth();
  m_details->hookNetwork();
#endif

  m_details->m_ui.m_txtUsername->lineEdit()->setText(account<FeedlyServiceRoot>()->network()->username());
  m_details->m_ui.m_txtDeveloperAccessToken->lineEdit()->setText(account<FeedlyServiceRoot>()->network()->developerAccessToken());
  m_details->m_ui.m_checkDownloadOnlyUnreadMessages->setChecked(account<FeedlyServiceRoot>()->network()->downloadOnlyUnreadMessages());
  m_details->m_ui.m_spinLimitMessages->setValue(account<FeedlyServiceRoot>()->network()->batchSize());
}

void FormEditFeedlyAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
