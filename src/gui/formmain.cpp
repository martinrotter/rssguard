#include <QCloseEvent>
#include <QMessageBox>

#include "gui/formmain.h"
#include "gui/formabout.h"
#include "gui/formsettings.h"
#include "gui/webbrowser.h"
#include "gui/themefactory.h"
#include "gui/systemtrayicon.h"
#include "gui/tabbar.h"
#include "core/settings.h"
#include "core/defs.h"
#include "qtsingleapplication/qtsingleapplication.h"


FormMain *FormMain::s_instance;

FormMain::FormMain(QWidget *parent) : QMainWindow(parent), m_ui(new Ui::FormMain) {
  m_ui->setupUi(this);

  // Initialize singleton.
  s_instance = this;

  // Prepare main window.
  prepareMenus();

  m_ui->m_tabWidget->initializeTabs();

  // Establish connections.
  createConnections();

  setupIcons();
}

FormMain::~FormMain() {
  delete m_ui;

  if (SystemTrayIcon::isSystemTrayAvailable()) {
    delete m_trayMenu;
    qDebug("Deleting tray icon menu.");
  }
}

FormMain *FormMain::getInstance() {
  return s_instance;
}

QMenu *FormMain::getTrayMenu() {
  return m_trayMenu;
}

QList<QAction*> FormMain::getActions() {
  QList<QAction*> actions;
  actions << m_ui->m_actionImport << m_ui->m_actionExport <<
             m_ui->m_actionSettings << m_ui->m_actionQuit <<
             m_ui->m_actionFullscreen << m_ui->m_actionAboutGuard;
  return actions;
}

void FormMain::prepareTabs() {
}

void FormMain::addEmptyBrowser() {
  addBrowser(false, true);
}

void FormMain::addLinkedBrowser() {

}

void FormMain::addBrowser(bool move_after_current,
                          bool make_active,
                          const QUrl &initial_url) {
  // Create new WebBrowser.
  WebBrowser *browser = new WebBrowser(m_ui->m_tabWidget);
  int final_index;

  if (move_after_current) {
    // Insert web browser after current tab.
    final_index = m_ui->m_tabWidget->insertTab(m_ui->m_tabWidget->currentIndex() + 1,
                                               browser,
                                               QIcon(),
                                               tr("Web browser"),
                                               TabBar::Closable);
  }
  else {
    // Add new browser as the last tab.
    final_index = m_ui->m_tabWidget->addTab(browser,
                                            QIcon(),
                                            tr("Web browser"),
                                            TabBar::Closable);
  }

  // Load initial web page if desired.
  if (initial_url.isValid()) {
    browser->navigateToUrl(initial_url);
  }

  // Make new web browser active if desired.
  if (make_active) {
    m_ui->m_tabWidget->setCurrentIndex(final_index);
  }
}

void FormMain::prepareMenus() {
  // Setup menu for tray icon.
  if (SystemTrayIcon::isSystemTrayAvailable()) {
#if defined(Q_OS_WIN)
    m_trayMenu = new TrayIconMenu(APP_NAME, this);
#else
    m_trayMenu = new QMenu(APP_NAME, this);
#endif

    // Add needed items to the menu.
    m_trayMenu->addAction(m_ui->m_actionSettings);
    m_trayMenu->addAction(m_ui->m_actionQuit);

    qDebug("Creating tray icon menu.");
  }
}

void FormMain::processExecutionMessage(const QString &message) {
  qDebug("Received '%s' execution message from another application instance.",
         qPrintable(message));
  display();
}

void FormMain::quit() {
  qDebug("Quitting the application.");
  qApp->quit();
}

void FormMain::switchFullscreenMode(bool turn_fullscreen_on) {
  if (turn_fullscreen_on) {
    showFullScreen();
  } else {

    showNormal();
  }
}

void FormMain::switchVisibility() {
  if (isVisible()) {
    hide();
  }
  else {
    display();
  }
}

void FormMain::display() {
  // Make sure window is not minimized.
  setWindowState(windowState() & ~Qt::WindowMinimized);

  // Display the window and make sure it is raised on top.
  show();
  activateWindow();
  raise();

  // Raise alert event. Check the documentation for more info on this.
  QtSingleApplication::alert(this);
}

void FormMain::cleanupResources() {
  qDebug("Cleaning up resources before the application exits.");
}

bool FormMain::event(QEvent *event) {
  if (event->type() == ThemeFactoryEvent::type()) {
    // Handle the change of icon theme.
    setupIcons();
    event->accept();
    return true;
  }

  return QMainWindow::event(event);
}

void FormMain::setupIcons() {
  // Setup icons of this main window.
  m_ui->m_actionSettings->setIcon(ThemeFactory::getInstance()->fromTheme("preferences-system"));
  m_ui->m_actionQuit->setIcon(ThemeFactory::getInstance()->fromTheme("application-exit"));
  m_ui->m_actionAboutGuard->setIcon(ThemeFactory::getInstance()->fromTheme("help-about"));
  m_ui->m_actionImport->setIcon(ThemeFactory::getInstance()->fromTheme("document-import"));
  m_ui->m_actionExport->setIcon(ThemeFactory::getInstance()->fromTheme("document-export"));
  m_ui->m_actionFullscreen->setIcon(ThemeFactory::getInstance()->fromTheme("view-fullscreen"));

  // Setup icons for underlying components: opened web browsers...
  foreach (WebBrowser *browser, WebBrowser::runningWebBrowsers()) {
    browser->setupIcons();
  }

  // Find tab, which contains "Feeds" page and reload its icon.
  for (int index = 0; index < m_ui->m_tabWidget->count(); index++) {
    if (m_ui->m_tabWidget->tabBar()->tabType(index) == TabBar::FeedReader) {
      m_ui->m_tabWidget->setTabIcon(index, QIcon(APP_ICON_PATH));
      break;
    }
  }
}

void FormMain::createConnections() {
  // Menu "File" connections.
  connect(m_ui->m_actionQuit, &QAction::triggered, this, &FormMain::quit);

  // Menu "View" connections.
  connect(m_ui->m_actionFullscreen, &QAction::triggered, this, &FormMain::switchFullscreenMode);

  // Menu "Tools" connections.
  connect(m_ui->m_actionSettings, &QAction::triggered, this, &FormMain::showSettings);

  // Menu "Help" connections.
  connect(m_ui->m_actionAboutGuard, &QAction::triggered, this, &FormMain::showAbout);

  // General connections.
  connect(qApp, &QCoreApplication::aboutToQuit, this, &FormMain::cleanupResources);

  // TabWidget connections.
  connect(m_ui->m_tabWidget->tabBar(), &TabBar::emptySpaceDoubleClicked,
          this, &FormMain::addEmptyBrowser);
}

void FormMain::closeEvent(QCloseEvent *event) {
  if (SystemTrayIcon::isSystemTrayActivated()) {
    if (Settings::getInstance()->value(APP_CFG_GUI,
                                       "close_win_action",
                                       0).toInt() == 0) {
      // User selected to minimize the application if its main
      // window gets closed and tray icon is activated.
      hide();
      event->ignore();
    }
    else {
      // User selected to quit the application if its main
      // window gets closed and tray icon is activated.
      qApp->quit();
    }
  }
}

void FormMain::showAbout() {
  FormAbout(this).exec();
}

void FormMain::showSettings() {
  FormSettings(this).exec();
}
