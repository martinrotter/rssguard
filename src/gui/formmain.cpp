#include "gui/formmain.h"
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

void FormMain::cleanupResources() {
  Settings::deleteSettings();
}

void FormMain::createConnections() {
  connect(qApp, &QCoreApplication::aboutToQuit, this, &FormMain::cleanupResources);
}
