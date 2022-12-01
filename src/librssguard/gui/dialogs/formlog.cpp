// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formlog.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QScrollBar>

FormLog::FormLog(QWidget* parent) : QDialog(parent) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this,
                                      qApp->icons()->fromTheme(QSL("dialog-information")),
                                      tr("Application log"));

  setWindowFlags(Qt::WindowType::WindowMinimizeButtonHint | windowFlags());
}

FormLog::~FormLog() {}

void FormLog::appendLogMessage(const QString& message) {
  m_ui.m_txtLog->appendPlainText(message);
  m_ui.m_txtLog->verticalScrollBar()->setValue(m_ui.m_txtLog->verticalScrollBar()->maximum());
}
