// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formaccountdetails.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

FormAccountDetails::FormAccountDetails(const QIcon& icon, QWidget* parent) : QDialog(parent), m_account(nullptr) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this, icon.isNull()
                                      ? qApp->icons()->fromTheme(QSL("emblem-system"))
                                      : icon);
  createConnections();
}

void FormAccountDetails::apply() {
  if (m_account != nullptr) {
    // Perform last-time operations before account is changed.
    auto* cached_account = dynamic_cast<CacheForServiceRoot*>(m_account);

    if (cached_account != nullptr) {
      qWarningNN << LOGSEC_CORE << "Last-time account cache saving before account gets changed.";
      cached_account->saveAllCachedData(true);
    }
  }
}

void FormAccountDetails::insertCustomTab(QWidget* custom_tab, const QString& title, int index) {
  m_ui.m_tabWidget->insertTab(index, custom_tab, title);
}

void FormAccountDetails::activateTab(int index) {
  m_ui.m_tabWidget->setCurrentIndex(index);
}

void FormAccountDetails::clearTabs() {
  m_ui.m_tabWidget->clear();
}

void FormAccountDetails::setEditableAccount(ServiceRoot* editable_account) {
  setWindowTitle(tr("Edit account '%1'").arg(editable_account->title()));
  m_account = editable_account;
}

void FormAccountDetails::createConnections() {
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormAccountDetails::apply);
}
