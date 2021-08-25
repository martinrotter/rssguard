// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/feedly/gui/formeditfeedlyaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/feedly/definitions.h"
#include "services/feedly/feedlynetwork.h"
#include "services/feedly/feedlyserviceroot.h"
#include "services/feedly/gui/feedlyaccountdetails.h"

#if defined(FEEDLY_OFFICIAL_SUPPORT)
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
  FormAccountDetails::apply();

#if defined(FEEDLY_OFFICIAL_SUPPORT)
  account<FeedlyServiceRoot>()->network()->oauth()->logout(false);
#endif

  bool using_another_acc =
    m_details->m_ui.m_txtUsername->lineEdit()->text() !=account<FeedlyServiceRoot>()->network()->username();

  account<FeedlyServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<FeedlyServiceRoot>()->network()->setDownloadOnlyUnreadMessages(m_details->m_ui.m_checkDownloadOnlyUnreadMessages->isChecked());
  account<FeedlyServiceRoot>()->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  account<FeedlyServiceRoot>()->network()->setDeveloperAccessToken(m_details->m_ui.m_txtDeveloperAccessToken->lineEdit()->text());

  account<FeedlyServiceRoot>()->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew) {
    if (using_another_acc) {
      account<FeedlyServiceRoot>()->completelyRemoveAllData();
    }

    account<FeedlyServiceRoot>()->start(true);
  }
}

void FormEditFeedlyAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

#if defined(FEEDLY_OFFICIAL_SUPPORT)
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
