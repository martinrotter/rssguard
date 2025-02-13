// For license of this file, see <project-root-folder>/LICENSE.md.

#include "src/gui/formeditstandardaccount.h"

#include "src/gui/standardaccountdetails.h"
#include "src/standardserviceentrypoint.h"
#include "src/standardserviceroot.h"

FormEditStandardAccount::FormEditStandardAccount(QWidget* parent)
  : FormAccountDetails(StandardServiceEntryPoint().icon(), parent),
    m_standardDetails(new StandardAccountDetails(this)) {

  insertCustomTab(m_standardDetails, tr("Account setup"), 0);
  activateTab(0);
}

FormEditStandardAccount::~FormEditStandardAccount() = default;

void FormEditStandardAccount::loadAccountData() {
  FormAccountDetails::loadAccountData();

  StandardServiceRoot* acc = account<StandardServiceRoot>();

  if (m_creatingNew) {
    m_standardDetails->m_ui.m_txtTitle->setText(StandardServiceRoot::defaultTitle());
  }
  else {
    m_standardDetails->m_ui.m_txtTitle->setText(acc->title());
  }

  m_standardDetails->m_ui.m_btnIcon->setIcon(acc->fullIcon());
  m_standardDetails->m_ui.m_spinFeedSpacing->setValue(acc->spacingSameHostsRequests());
}

void FormEditStandardAccount::apply() {
  FormAccountDetails::apply();

  StandardServiceRoot* acc = account<StandardServiceRoot>();

  acc->setIcon(m_standardDetails->m_ui.m_btnIcon->icon());
  acc->setTitle(m_standardDetails->m_ui.m_txtTitle->text());
  acc->setSpacingSameHostsRequests(m_standardDetails->m_ui.m_spinFeedSpacing->value());

  m_account->saveAccountDataToDatabase();
  m_account->itemChanged({m_account});

  accept();
}
