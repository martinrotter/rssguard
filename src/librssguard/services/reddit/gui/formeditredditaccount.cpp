// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/reddit/gui/formeditredditaccount.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/reddit/gui/redditaccountdetails.h"
#include "services/reddit/redditserviceroot.h"

FormEditRedditAccount::FormEditRedditAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("reddit")), parent), m_details(new RedditAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  m_details->m_ui.m_txtUsername->setFocus();
  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, [this]() {
    m_details->testSetup(m_proxyDetails->proxy());
  });
}

void FormEditRedditAccount::apply() {
  FormAccountDetails::apply();

  bool using_another_acc =
    m_details->m_ui.m_txtUsername->lineEdit()->text() != account<RedditServiceRoot>()->network()->username();

  // Make sure that the data copied from GUI are used for brand new login.
  account<RedditServiceRoot>()->network()->oauth()->logout(false);
  account<RedditServiceRoot>()->network()->oauth()->setClientId(m_details->m_ui.m_txtAppId->lineEdit()->text());
  account<RedditServiceRoot>()->network()->oauth()->setClientSecret(m_details->m_ui.m_txtAppKey->lineEdit()->text());
  account<RedditServiceRoot>()->network()->oauth()->setRedirectUrl(m_details->m_ui.m_txtRedirectUrl->lineEdit()->text(),
                                                                   true);

  account<RedditServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<RedditServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  account<RedditServiceRoot>()->network()->setDownloadOnlyUnreadMessages(m_details->m_ui.m_cbDownloadOnlyUnreadMessages
                                                                           ->isChecked());

  account<RedditServiceRoot>()->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew) {
    if (using_another_acc) {
      account<RedditServiceRoot>()->completelyRemoveAllData();
    }

    account<RedditServiceRoot>()->start(true);
  }
}

void FormEditRedditAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  m_details->m_oauth = account<RedditServiceRoot>()->network()->oauth();
  m_details->hookNetwork();

  // Setup the GUI.
  m_details->m_ui.m_txtAppId->lineEdit()->setText(m_details->m_oauth->clientId());
  m_details->m_ui.m_txtAppKey->lineEdit()->setText(m_details->m_oauth->clientSecret());
  m_details->m_ui.m_txtRedirectUrl->lineEdit()->setText(m_details->m_oauth->redirectUrl());

  m_details->m_ui.m_txtUsername->lineEdit()->setText(account<RedditServiceRoot>()->network()->username());
  m_details->m_ui.m_spinLimitMessages->setValue(account<RedditServiceRoot>()->network()->batchSize());
  m_details->m_ui.m_cbDownloadOnlyUnreadMessages
    ->setChecked(account<RedditServiceRoot>()->network()->downloadOnlyUnreadMessages());
}
