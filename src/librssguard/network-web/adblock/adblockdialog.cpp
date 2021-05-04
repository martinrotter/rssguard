// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/adblock/adblockdialog.h"

#include "network-web/adblock/adblockmanager.h"

#include "definitions/definitions.h"
#include "gui/guiutilities.h"
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

  QPushButton* btn_options = m_ui.m_buttonBox->addButton(tr("Options"),
                                                         QDialogButtonBox::ButtonRole::HelpRole);
  auto* menu = new QMenu(btn_options);

  menu->addAction(tr("Learn about writing rules..."), this, &AdBlockDialog::learnAboutAdblock);
  btn_options->setMenu(menu);

  connect(m_ui.m_cbEnable, &QCheckBox::toggled, this, &AdBlockDialog::enableAdBlock);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &AdBlockDialog::saveAndClose);

  load();
  m_ui.m_buttonBox->setFocus();
}

void AdBlockDialog::saveAndClose() {
  m_manager->setFilterLists(m_ui.m_txtPredefined->toPlainText().split(QSL("\n")));
  m_manager->setCustomFilters(m_ui.m_txtCustom->toPlainText().split(QSL("\n")));
  m_manager->updateUnifiedFiltersFile();

  close();
}

void AdBlockDialog::enableAdBlock(bool enable) {
  m_manager->load(false);

  if (enable) {
    load();
  }
}

void AdBlockDialog::learnAboutAdblock() {
  qApp->web()->openUrlInExternalBrowser(QSL(ADBLOCK_HOWTO));
}

void AdBlockDialog::load() {
  if (m_loaded || !m_ui.m_cbEnable->isChecked()) {
    return;
  }

  m_ui.m_txtCustom->setPlainText(m_manager->customFilters().join(QSL("\n")));
  m_ui.m_txtPredefined->setPlainText(m_manager->filterLists().join(QSL("\n")));
}
