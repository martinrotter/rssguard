// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/newsblur/gui/formeditnewsbluraccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"
#include "services/newsblur/definitions.h"
#include "services/newsblur/gui/newsbluraccountdetails.h"
#include "services/newsblur/newsblurnetwork.h"
#include "services/newsblur/newsblurserviceroot.h"

FormEditNewsBlurAccount::FormEditNewsBlurAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("newsblur")), parent), m_details(new NewsBlurAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditNewsBlurAccount::performTest);

  m_details->m_ui.m_txtUrl->setFocus();
}

void FormEditNewsBlurAccount::apply() {
  FormAccountDetails::apply();

  NewsBlurServiceRoot* existing_root = account<NewsBlurServiceRoot>();
  bool using_another_acc =
    m_details->m_ui.m_txtUsername->lineEdit()->text() != existing_root->network()->username() ||
    m_details->m_ui.m_txtUrl->lineEdit()->text() != existing_root->network()->baseUrl();

  existing_root->network()->setBaseUrl(m_details->m_ui.m_txtUrl->lineEdit()->text());
  existing_root->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  existing_root->network()->setPassword(m_details->m_ui.m_txtPassword->lineEdit()->text());
  existing_root->network()->setBatchSize(m_details->m_ui.m_spinLimitMessages->value());
  existing_root->network()->setDownloadOnlyUnreadMessages(m_details->m_ui.m_cbDownloadOnlyUnreadMessages->isChecked());

  existing_root->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew) {
    if (using_another_acc) {
      existing_root->completelyRemoveAllData();
    }

    existing_root->start(true);
  }
}

void FormEditNewsBlurAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  NewsBlurServiceRoot* existing_root = account<NewsBlurServiceRoot>();

  m_details->m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->username());
  m_details->m_ui.m_txtPassword->lineEdit()->setText(existing_root->network()->password());
  m_details->m_ui.m_txtUrl->lineEdit()->setText(existing_root->network()->baseUrl());
  m_details->m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());
  m_details->m_ui.m_cbDownloadOnlyUnreadMessages->setChecked(existing_root->network()->downloadOnlyUnreadMessages());
}

void FormEditNewsBlurAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
