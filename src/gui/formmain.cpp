#include "gui/formmain.h"
#include "core/settings.h"
#include "qtsingleapplication/qtsingleapplication.h"


FormMain::FormMain(QWidget *parent) : QMainWindow(parent), m_ui(new Ui::FormMain) {
  m_ui->setupUi(this);

  // for testing purposes
  m_ui->toolButton->setIcon(QIcon::fromTheme("application-exit"));

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

void FormMain::createConnections() {
  // Menu "File" connections.
  connect(m_ui->m_actionQuit, &QAction::triggered, this, &FormMain::quit);

  // General connections.
  connect(qApp, &QCoreApplication::aboutToQuit, this, &FormMain::cleanupResources);
}
