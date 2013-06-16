// for testing
#include <QMessageBox>

#include "gui/formmain.h"
#include "gui/formsettings.h"
#include "gui/themefactory.h"
#include "core/settings.h"
#include "qtsingleapplication/qtsingleapplication.h"


FormMain::FormMain(QWidget *parent) : QMainWindow(parent), m_ui(new Ui::FormMain) {
  m_ui->setupUi(this);

  createConnections();
}

FormMain::~FormMain() {
  delete m_ui;
}

void FormMain::processExecutionMessage(const QString &message) {
  // TODO: Implement proper reaction when application is launched more than once.
  qDebug("Received '%s' execution message from another application instance.",
         qPrintable(message));
}

void FormMain::quit() {
  qDebug("Quitting the application.");
  qApp->quit();
}

void FormMain::cleanupResources() {
  qDebug("Cleaning up resources before the application exits.");
}

#if defined(Q_OS_LINUX)
bool FormMain::event(QEvent *event) {
  if (event->type() == ThemeFactoryEvent::type()) {
    // Handle the change of icon theme.
    setupIcons();
    return true;
  }

  return QMainWindow::event(event);
}

void FormMain::setupIcons() {
  // NOTE: Call QIcon::fromTheme for all needed widgets here.
}
#endif

void FormMain::createConnections() {
  // Menu "File" connections.
  connect(m_ui->m_actionQuit, &QAction::triggered, this, &FormMain::quit);

  // Menu "Tools" connections.
  connect(m_ui->m_actionSettings, &QAction::triggered, this, &FormMain::showSettings);

  // General connections.
  connect(qApp, &QCoreApplication::aboutToQuit, this, &FormMain::cleanupResources);
}

void FormMain::showSettings() {
  FormSettings form_settings(this);
  form_settings.exec();
}
