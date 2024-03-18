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

  if (m_creatingNew) {
    m_standardDetails->m_ui.m_txtTitle->setText(StandardServiceRoot::defaultTitle());
  }
  else {
    m_standardDetails->m_ui.m_txtTitle->setText(m_account->title());
  }

  m_standardDetails->m_ui.m_btnIcon->setIcon(m_account->fullIcon());
}

void FormEditStandardAccount::apply() {
  FormAccountDetails::apply();

  m_account->setIcon(m_standardDetails->m_ui.m_btnIcon->icon());
  m_account->setTitle(m_standardDetails->m_ui.m_txtTitle->text());

  m_account->saveAccountDataToDatabase();
  m_account->itemChanged({m_account});

  accept();
}
