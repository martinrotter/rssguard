// For license of this file, see <project-root-folder>/LICENSE.md.

#include "services/abstract/gui/formaccountdetails.h"

#include "exceptions/applicationexception.h"
#include "gui/guiutilities.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "services/abstract/importantnode.h"
#include "services/abstract/labelsnode.h"
#include "services/abstract/searchsnode.h"
#include "services/abstract/serviceroot.h"
#include "services/abstract/unreadnode.h"

#include "ui_accountdetails.h"
#include "ui_formaccountdetails.h"

#include <QScrollArea>

FormAccountDetails::FormAccountDetails(const QIcon& icon, QWidget* parent)
  : QDialog(parent), m_ui(new Ui::FormAccountDetails()), m_proxyDetails(new NetworkProxyDetails(this)),
    m_accountDetails(new AccountDetails(this)), m_account(nullptr), m_creatingNew(false) {
  m_ui->setupUi(this);

  m_proxyDetails->setup(false, true);

  insertScrollableCustomTab(m_accountDetails, tr("Miscellaneous"), 0);
  insertScrollableCustomTab(m_proxyDetails, tr("Network proxy"), 1);

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

void FormAccountDetails::rollBack() {}

void FormAccountDetails::acceptIfPossible() {
  try {
    apply();
    accept();
  }
  catch (const ApplicationException& ex) {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot save account properties"),
                          tr("Cannot save changes: %1").arg(ex.message()),
                          QSystemTrayIcon::MessageIcon::Critical},
                         {},
                         {},
                         this);
  }
}

void FormAccountDetails::insertCustomTab(QWidget* custom_tab, const QString& title, int index) {
  m_ui->m_tabWidget->insertTab(index, custom_tab, title);
}

void FormAccountDetails::insertScrollableCustomTab(QWidget* custom_tab, const QString& title, int index) {
  auto* scroll_area = new QScrollArea(m_ui->m_tabWidget);
  scroll_area->setFrameShape(QFrame::Shape::NoFrame);
  scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarPolicy::ScrollBarAlwaysOff);
  scroll_area->setWidgetResizable(true);
  scroll_area->setWidget(custom_tab);

  insertCustomTab(scroll_area, title, index);
}

void FormAccountDetails::activateTab(int index) {
  m_ui->m_tabWidget->setCurrentIndex(index);
}

void FormAccountDetails::clearTabs() {
  m_ui->m_tabWidget->clear();
}

bool FormAccountDetails::loadAccountData() {
  if (m_creatingNew) {
    setWindowTitle(tr("Add new account"));
  }
  else {
    setWindowTitle(tr("Edit account \"%1\"").arg(m_account->title()));

    // Perform last-time operations before account is changed.
    auto* cached_account = dynamic_cast<CacheForServiceRoot*>(m_account);

    if (cached_account != nullptr && cached_account->hasCachedData()) {
      qWarningNN << LOGSEC_CORE << "Last-time account cache saving before account could be edited.";

      if (!cached_account->saveAllCachedData()) {
        const auto answer =
          MsgBox::show(this,
                       QMessageBox::Icon::Warning,
                       tr("Unsynchronized article changes"),
                       tr("This account has local article changes which could not be synchronized."),
                       tr("If you continue editing the account, these pending local article changes will be discarded."),
                       {},
                       QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::Cancel,
                       QMessageBox::StandardButton::Cancel);

        if (answer == QMessageBox::StandardButton::Yes) {
          cached_account->clearCachedData();
        }
        else {
          return false;
        }
      }
    }
  }

  m_accountDetails->m_ui->m_cbImportant->setChecked(m_account->nodeShowImportant());
  m_accountDetails->m_ui->m_cbLabels->setChecked(m_account->nodeShowLabels());
  m_accountDetails->m_ui->m_cbProbes->setChecked(m_account->nodeShowProbes());
  m_accountDetails->m_ui->m_cbUnread->setChecked(m_account->nodeShowUnread());

  m_proxyDetails->setProxy(m_account->networkProxy());

  return true;
}

void FormAccountDetails::createConnections() {
  connect(m_ui->m_buttonBox, &QDialogButtonBox::accepted, this, &FormAccountDetails::acceptIfPossible);
  connect(m_ui->m_buttonBox, &QDialogButtonBox::rejected, this, &FormAccountDetails::rollBack);
}
