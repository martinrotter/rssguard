// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formaccountdetails.h"

#include "ui_accountdetails.h"
#include "ui_formaccountdetails.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/searchsnode.h"
#include "services/abstract/serviceroot.h"
#include "services/abstract/unreadnode.h"

FormAccountDetails::FormAccountDetails(const QIcon& icon, QWidget* parent)
  : QDialog(parent), m_proxyDetails(new NetworkProxyDetails(this)), m_accountDetails(new AccountDetails(this)),
    m_account(nullptr), m_creatingNew(false), m_ui(new Ui::FormAccountDetails()) {
  m_ui->setupUi(this);

  insertCustomTab(m_accountDetails, tr("Miscellaneous"), 0);
  insertCustomTab(m_proxyDetails, tr("Network proxy"), 1);

  GuiUtilities::applyDialogProperties(*this, icon.isNull() ? qApp->icons()->fromTheme(QSL("emblem-system")) : icon);
  createConnections();
}

FormAccountDetails::~FormAccountDetails() = default;

void FormAccountDetails::apply() {
  m_account->setNodeShowImportant(m_accountDetails->m_ui->m_cbImportant->isChecked());
  m_account->setNodeShowLabels(m_accountDetails->m_ui->m_cbLabels->isChecked());
  m_account->setNodeShowProbes(m_accountDetails->m_ui->m_cbProbes->isChecked());
  m_account->setNodeShowUnread(m_accountDetails->m_ui->m_cbUnread->isChecked());

  m_account->setNetworkProxy(m_proxyDetails->proxy());

  if (!m_creatingNew) {
    m_account->itemChanged({m_account->importantNode(),
                            m_account->labelsNode(),
                            m_account->unreadNode(),
                            m_account->probesNode()});
  }
}

void FormAccountDetails::insertCustomTab(QWidget* custom_tab, const QString& title, int index) {
  m_ui->m_tabWidget->insertTab(index, custom_tab, title);
}

void FormAccountDetails::activateTab(int index) {
  m_ui->m_tabWidget->setCurrentIndex(index);
}

void FormAccountDetails::clearTabs() {
  m_ui->m_tabWidget->clear();
}

void FormAccountDetails::loadAccountData() {
  if (m_creatingNew) {
    setWindowTitle(tr("Add new account"));
  }
  else {
    setWindowTitle(tr("Edit account \"%1\"").arg(m_account->title()));

    // Perform last-time operations before account is changed.
    auto* cached_account = dynamic_cast<CacheForServiceRoot*>(m_account);

    if (cached_account != nullptr) {
      qWarningNN << LOGSEC_CORE << "Last-time account cache saving before account could be edited.";
      cached_account->saveAllCachedData(true);
    }
  }

  m_accountDetails->m_ui->m_cbImportant->setChecked(m_account->nodeShowImportant());
  m_accountDetails->m_ui->m_cbLabels->setChecked(m_account->nodeShowLabels());
  m_accountDetails->m_ui->m_cbProbes->setChecked(m_account->nodeShowProbes());
  m_accountDetails->m_ui->m_cbUnread->setChecked(m_account->nodeShowUnread());

  m_proxyDetails->setProxy(m_account->networkProxy());
}

void FormAccountDetails::createConnections() {
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormAccountDetails::apply);
}
