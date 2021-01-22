// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/tt-rss/gui/formeditttrssaccount.h"

#include "miscellaneous/iconfactory.h"
#include "services/tt-rss/definitions.h"
#include "services/tt-rss/gui/ttrssaccountdetails.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "services/tt-rss/ttrssserviceroot.h"

FormEditTtRssAccount::FormEditTtRssAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("tt-rss")), parent), m_details(new TtRssAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditTtRssAccount::performTest);
  m_details->m_ui.m_txtUrl->setFocus();
}

void FormEditTtRssAccount::apply() {
  bool editing_account = !applyInternal<TtRssServiceRoot>();

  account<TtRssServiceRoot>()->network()->setUrl(m_details->m_ui.m_txtUrl->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setPassword(m_details->m_ui.m_txtPassword->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setAuthIsUsed(m_details->m_ui.m_gbHttpAuthentication->isChecked());
  account<TtRssServiceRoot>()->network()->setAuthUsername(m_details->m_ui.m_txtHttpUsername->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setAuthPassword(m_details->m_ui.m_txtHttpPassword->lineEdit()->text());
  account<TtRssServiceRoot>()->network()->setForceServerSideUpdate(m_details->m_ui.m_checkServerSideUpdate->isChecked());
  account<TtRssServiceRoot>()->network()->setDownloadOnlyUnreadMessages(m_details->m_ui.m_checkDownloadOnlyUnreadMessages->isChecked());

  account<TtRssServiceRoot>()->saveAccountDataToDatabase(!editing_account);
  accept();

  if (editing_account) {
    account<TtRssServiceRoot>()->network()->logout(m_account->networkProxy());
    account<TtRssServiceRoot>()->completelyRemoveAllData();
    account<TtRssServiceRoot>()->syncIn();
  }
}

void FormEditTtRssAccount::setEditableAccount(ServiceRoot* editable_account) {
  FormAccountDetails::setEditableAccount(editable_account);

  TtRssServiceRoot* existing_root = qobject_cast<TtRssServiceRoot*>(editable_account);

  m_details->m_ui.m_gbHttpAuthentication->setChecked(existing_root->network()->authIsUsed());
  m_details->m_ui.m_txtHttpPassword->lineEdit()->setText(existing_root->network()->authPassword());
  m_details->m_ui.m_txtHttpUsername->lineEdit()->setText(existing_root->network()->authUsername());
  m_details->m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->username());
  m_details->m_ui.m_txtPassword->lineEdit()->setText(existing_root->network()->password());
  m_details->m_ui.m_txtUrl->lineEdit()->setText(existing_root->network()->url());
  m_details->m_ui.m_checkServerSideUpdate->setChecked(existing_root->network()->forceServerSideUpdate());
  m_details->m_ui.m_checkDownloadOnlyUnreadMessages->setChecked(existing_root->network()->downloadOnlyUnreadMessages());
}

void FormEditTtRssAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
