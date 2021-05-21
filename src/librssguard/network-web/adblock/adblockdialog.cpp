// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockdialog.h"

#include "network-web/adblock/adblockmanager.h"

#include "definitions/definitions.h"
#include "exceptions/applicationexception.h"
#include "gui/guiutilities.h"
#include "gui/messagebox.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/webfactory.h"

#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>

AdBlockDialog::AdBlockDialog(QWidget* parent)
  : QDialog(parent), m_manager(qApp->web()->adBlock()), m_loaded(false) {
  m_ui.setupUi(this);
  m_ui.m_cbEnable->setChecked(m_manager->isEnabled());

  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->miscIcon(ADBLOCK_ICON_ACTIVE),
                                      tr("AdBlock configuration"));

  connect(m_ui.m_btnHelp, &QPushButton::clicked, this, [=]() {
    qApp->web()->openUrlInExternalBrowser(QSL(ADBLOCK_HOWTO));
  });
  connect(m_ui.m_btnTest, &QPushButton::clicked, this, &AdBlockDialog::testConfiguration);
  connect(m_ui.m_cbEnable, &QCheckBox::toggled, this, &AdBlockDialog::enableAdBlock);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &AdBlockDialog::saveAndClose);

  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_btnHelp->setIcon(qApp->icons()->fromTheme(QSL("help-about")));
  m_ui.m_btnTest->setIcon(qApp->icons()->fromTheme(QSL("media-playback-start")));
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("No test executed yet."),
                                  tr("No test executed yet."));

  load();
  m_ui.m_buttonBox->setFocus();
}

void AdBlockDialog::saveAndClose() {
  m_manager->setFilterLists(m_ui.m_txtPredefined->toPlainText().split(QSL("\n")));
  m_manager->setCustomFilters(m_ui.m_txtCustom->toPlainText().split(QSL("\n")));

  try {
    m_manager->updateUnifiedFiltersFile();
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_ADBLOCK
                << "Failed to write unified filters to file or re-start server, error:"
                << QUOTE_W_SPACE_DOT(ex.message());

    MessageBox::show(this,
                     QMessageBox::Icon::Critical,
                     tr("Cannot enable AdBlock"),
                     tr("There is some error in AdBlock component and it cannot be enabled. "
                        "Check error message below (or application debug log) for more information."),
                     {},
                     ex.message());
  }

  close();
}

void AdBlockDialog::enableAdBlock(bool enable) {
  m_manager->load(false);

  if (enable) {
    load();
  }
}

void AdBlockDialog::testConfiguration() {
  try {
    m_manager->setFilterLists(m_ui.m_txtPredefined->toPlainText().split(QSL("\n")));
    m_manager->setCustomFilters(m_ui.m_txtCustom->toPlainText().split(QSL("\n")));
    m_manager->updateUnifiedFiltersFile();
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok, tr("You are good to go."), tr("OK!"));
  }
  catch (const ApplicationException& ex) {
    qCriticalNN << LOGSEC_ADBLOCK
                << "Test of configuration failed:"
                << QUOTE_W_SPACE_DOT(ex.message());
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                    tr("There is error, check application log for more details and "
                                       "head to online documentation. Also make sure that Node.js is installed."
                                       "\n\nError: %1").arg(ex.message()),
                                    tr("ERROR!"));

  }
}

void AdBlockDialog::load() {
  if (m_loaded) {
    return;
  }

  m_ui.m_txtCustom->setPlainText(m_manager->customFilters().join(QSL("\n")));
  m_ui.m_txtPredefined->setPlainText(m_manager->filterLists().join(QSL("\n")));
}
