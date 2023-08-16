// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/standard/gui/formeditstandardaccount.h"

#include "services/standard/standardserviceentrypoint.h"

FormEditStandardAccount::FormEditStandardAccount(QWidget* parent)
  : FormAccountDetails(StandardServiceEntryPoint().icon(), parent) {}

void FormEditStandardAccount::apply() {
  FormAccountDetails::apply();

  m_account->saveAccountDataToDatabase();
  accept();
}
