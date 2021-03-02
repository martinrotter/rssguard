// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formaccountdetails.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/serviceroot.h"

FormAccountDetails::FormAccountDetails(const QIcon& icon, QWidget* parent)
  : QDialog(parent), m_proxyDetails(new NetworkProxyDetails(this)), m_account(nullptr), m_creatingNew(false) {
  m_ui.setupUi(this);

  insertCustomTab(m_proxyDetails, tr("Network proxy"), 0);
  GuiUtilities::applyDialogProperties(*this, icon.isNull()
                                      ? qApp->icons()->fromTheme(QSL("emblem-system"))
                                      : icon);
  createConnections();
}

void FormAccountDetails::apply() {
  m_account->setNetworkProxy(m_proxyDetails->proxy());
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

void FormAccountDetails::loadAccountData() {
  if (m_creatingNew) {
    setWindowTitle(tr("Add new account"));
  }
  else {
    setWindowTitle(tr("Edit account '%1'").arg(m_account->title()));

    // Perform last-time operations before account is changed.
    auto* cached_account = dynamic_cast<CacheForServiceRoot*>(m_account);

    if (cached_account != nullptr) {
      qWarningNN << LOGSEC_CORE << "Last-time account cache saving before account could be edited.";
      cached_account->saveAllCachedData(true);
    }
  }

  m_proxyDetails->setProxy(m_account->networkProxy());
}

void FormAccountDetails::createConnections() {
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormAccountDetails::apply);
}
