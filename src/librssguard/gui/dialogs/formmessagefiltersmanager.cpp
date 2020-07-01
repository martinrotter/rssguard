// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formmessagefiltersmanager.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/webfactory.h"

FormMessageFiltersManager::FormMessageFiltersManager(QWidget* parent) : QDialog(parent) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this, qApp->icons()->fromTheme(QSL("view-list-details")));

  m_ui.m_btnAddNew->setIcon(qApp->icons()->fromTheme(QSL("list-add")));
  m_ui.m_btnRemoveSelected->setIcon(qApp->icons()->fromTheme(QSL("list-remove")));
  m_ui.m_txtScript->setFont(QFontDatabase::systemFont(QFontDatabase::SystemFont::FixedFont));

  connect(m_ui.m_btnDetailedHelp, &QPushButton::clicked, this, []() {
    qApp->web()->openUrlInExternalBrowser(MSG_FILTERING_HELP);
  });
}
