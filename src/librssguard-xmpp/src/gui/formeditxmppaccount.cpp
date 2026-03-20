// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formeditxmppaccount.h"

#include "src/gui/xmppaccountdetails.h"
#include "src/xmppnetwork.h"
#include "src/xmppserviceroot.h"

#include <librssguard/miscellaneous/iconfactory.h>

FormEditXmppAccount::FormEditXmppAccount(QWidget* parent)
  : FormAccountDetails(qApp->icons()->miscIcon(QSL("xmpp")), parent), m_details(new XmppAccountDetails(this)) {
  insertCustomTab(m_details, tr("Server setup"), 0);
  activateTab(0);

  connect(m_details->m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditXmppAccount::performTest);
  m_details->m_ui.m_txtHost->setFocus();
}

void FormEditXmppAccount::apply() {
  FormAccountDetails::apply();

  bool using_another_acc =
    m_details->m_ui.m_txtUsername->lineEdit()->text() != account<XmppServiceRoot>()->network()->username() ||
    m_details->m_ui.m_txtHost->lineEdit()->text() != account<XmppServiceRoot>()->network()->domain() ||
    m_proxyDetails->proxy() != account<XmppServiceRoot>()->networkProxy();

  // account<XmppServiceRoot>()->network()->logout(m_account->networkProxy());
  account<XmppServiceRoot>()->network()->setDomain(m_details->m_ui.m_txtHost->lineEdit()->text());
  account<XmppServiceRoot>()->network()->setUsername(m_details->m_ui.m_txtUsername->lineEdit()->text());
  account<XmppServiceRoot>()->network()->setPassword(m_details->m_ui.m_txtPassword->lineEdit()->text());
  account<XmppServiceRoot>()->network()->setExtraServices(m_details->m_ui.m_txtAdditionalServices->toPlainText()
                                                            .split(QL1C('\n')));

  account<XmppServiceRoot>()->saveAccountDataToDatabase();
  accept();

  if (!m_creatingNew && using_another_acc) {
    account<XmppServiceRoot>()->completelyRemoveAllData();
    account<XmppServiceRoot>()->start(true);
  }
}

void FormEditXmppAccount::rollBack() {
  FormAccountDetails::rollBack();

  /*
  // No data is saved.
  if (m_creatingNew) {
    return;
  }

  account<XmppServiceRoot>()->start(false);
*/
}

void FormEditXmppAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  XmppServiceRoot* existing_root = account<XmppServiceRoot>();

  m_details->m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->username());
  m_details->m_ui.m_txtPassword->lineEdit()->setText(existing_root->network()->password());
  m_details->m_ui.m_txtHost->lineEdit()->setText(existing_root->network()->domain());
  m_details->m_ui.m_txtAdditionalServices
    ->setPlainText(existing_root->network()->extraServices().join(TextFactory::newline()));
}

void FormEditXmppAccount::performTest() {
  m_details->performTest(m_proxyDetails->proxy());
}
