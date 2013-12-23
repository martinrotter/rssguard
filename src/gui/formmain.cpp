#include "gui/formmain.h"

#include "core/defs.h"
#include "core/settings.h"
#include "core/systemfactory.h"
#include "gui/formabout.h"
#include "gui/formsettings.h"
#include "gui/webbrowser.h"
#include "gui/iconthemefactory.h"
#include "gui/systemtrayicon.h"
#include "gui/tabbar.h"
#include "gui/statusbar.h"
#include "gui/feedmessageviewer.h"
#include "qtsingleapplication/qtsingleapplication.h"

#include <QCloseEvent>
#include <QMessageBox>
#include <QSessionManager>
#include <QRect>
#include <QDesktopWidget>
#include <QReadWriteLock>


FormMain *FormMain::s_instance;

FormMain::FormMain(QWidget *parent) : QMainWindow(parent), m_ui(new Ui::FormMain) {
  m_ui->setupUi(this);

  // Initialize singleton.
  s_instance = this;

  setStatusBar(new StatusBar(this));

  // Prepare main window and tabs.
  prepareMenus();

  // Establish connections.
  createConnections();

  // Prepare tabs.
  m_ui->m_tabWidget->initializeTabs();

  setupIcons();
  loadSize();
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

TabWidget *FormMain::getTabWidget() {
  return m_ui->m_tabWidget;
}

QList<QAction*> FormMain::getActions() {
  QList<QAction*> actions;

  // Add basic actions.
  actions << m_ui->m_actionImport << m_ui->m_actionExport <<
             m_ui->m_actionSettings << m_ui->m_actionQuit <<
             m_ui->m_actionFullscreen;

  // Add web browser actions
  actions << m_ui->m_actionAddBrowser << m_ui->m_actionCloseCurrentTab <<
             m_ui->m_actionCloseAllTabs;

  // Add feeds/messages actions.
  actions << m_ui->m_actionOpenSelectedSourceArticlesExternally <<
             m_ui->m_actionOpenSelectedSourceArticlesInternally <<
             m_ui->m_actionOpenSelectedMessagesInternally <<
             m_ui->m_actionMarkAllMessagesAsRead <<
             m_ui->m_actionMarkAllMessagesAsUnread <<
             m_ui->m_actionDeleteAllMessages <<
             m_ui->m_actionMarkSelectedMessagesAsRead <<
             m_ui->m_actionMarkSelectedMessagesAsUnread <<
             m_ui->m_actionSwitchImportanceOfSelectedMessages <<
             m_ui->m_actionDeleteSelectedMessages <<
             m_ui->m_actionUpdateAllFeeds <<
             m_ui->m_actionUpdateSelectedFeeds <<
             m_ui->m_actionEditSelectedFeed <<
             m_ui->m_actionDeleteSelectedFeeds;

  return actions;
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

  if (message == APP_IS_RUNNING) {
    if (SystemTrayIcon::isSystemTrayActivated()) {
      SystemTrayIcon::getInstance()->showMessage(APP_NAME,
                                                 tr("Application is already running."),
                                                 QSystemTrayIcon::Information,
                                                 TRAY_ICON_BUBBLE_TIMEOUT);
    }

    display();
  }
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

void FormMain::onCommitData(QSessionManager &manager) { 
  Q_UNUSED(manager)
  qDebug("OS asked application to commit its data.");
}

void FormMain::onSaveState(QSessionManager &manager) {
  Q_UNUSED(manager)
  qDebug("OS asked application to save its state.");
}

void FormMain::onAboutToQuit() {
  // Make sure that we obtain close lock
  // BEFORE even trying to quit the application.
  if (SystemFactory::getInstance()->applicationCloseLock()->tryLockForWrite(CLOSE_LOCK_TIMEOUT)) {
    // Application obtained permission to close
    // in a safety way.
    qDebug("Close lock obtained safely.");
  }
  else {
    // Request for write lock timed-out. This means
    // that some critical action can be processed right now.
    qDebug("Close lock timed-out.");
  }

  qDebug("Cleaning up resources and saving application state.");

  m_ui->m_tabWidget->feedMessageViewer()->quitDownloader();
  saveSize();
}

bool FormMain::event(QEvent *event) {
  return QMainWindow::event(event);
}

void FormMain::setupIcons() {
  // Setup icons of this main window.
  m_ui->m_actionSettings->setIcon(IconThemeFactory::getInstance()->fromTheme("preferences-system"));
  m_ui->m_actionQuit->setIcon(IconThemeFactory::getInstance()->fromTheme("application-exit"));
  m_ui->m_actionAboutGuard->setIcon(IconThemeFactory::getInstance()->fromTheme("help-about"));
  m_ui->m_actionImport->setIcon(IconThemeFactory::getInstance()->fromTheme("document-import"));
  m_ui->m_actionExport->setIcon(IconThemeFactory::getInstance()->fromTheme("document-export"));
  m_ui->m_actionFullscreen->setIcon(IconThemeFactory::getInstance()->fromTheme("view-fullscreen"));

  // Web browser.
  m_ui->m_actionAddBrowser->setIcon(IconThemeFactory::getInstance()->fromTheme("list-add"));
  m_ui->m_actionCloseCurrentTab->setIcon(IconThemeFactory::getInstance()->fromTheme("list-remove"));
  m_ui->m_actionCloseAllTabs->setIcon(IconThemeFactory::getInstance()->fromTheme("list-remove"));
  m_ui->m_menuCurrentTab->setIcon(IconThemeFactory::getInstance()->fromTheme("go-home"));

  // Feeds/messages.
  m_ui->m_actionUpdateAllFeeds->setIcon(IconThemeFactory::getInstance()->fromTheme("view-refresh"));
  m_ui->m_actionUpdateSelectedFeeds->setIcon(IconThemeFactory::getInstance()->fromTheme("view-refresh"));
  m_ui->m_actionDeleteAllMessages->setIcon(IconThemeFactory::getInstance()->fromTheme("edit-delete"));

  m_ui->m_actionDeleteSelectedFeeds->setIcon(IconThemeFactory::getInstance()->fromTheme("edit-delete"));
  m_ui->m_actionDeleteSelectedMessages->setIcon(IconThemeFactory::getInstance()->fromTheme("edit-delete"));

  m_ui->m_actionAddNewCategory->setIcon(IconThemeFactory::getInstance()->fromTheme("document-new"));
  m_ui->m_actionAddNewFeed->setIcon(IconThemeFactory::getInstance()->fromTheme("document-new"));
  m_ui->m_actionEditSelectedFeed->setIcon(IconThemeFactory::getInstance()->fromTheme("document-properties"));
  m_ui->m_actionMarkAllMessagesAsRead->setIcon(IconThemeFactory::getInstance()->fromTheme("mail-mark-read"));
  m_ui->m_actionMarkAllMessagesAsUnread->setIcon(IconThemeFactory::getInstance()->fromTheme("mail-mark-unread"));
  m_ui->m_actionMarkFeedsAsRead->setIcon(IconThemeFactory::getInstance()->fromTheme("mail-mark-read"));
  m_ui->m_actionMarkSelectedMessagesAsRead->setIcon(IconThemeFactory::getInstance()->fromTheme("mail-mark-read"));
  m_ui->m_actionMarkSelectedMessagesAsUnread->setIcon(IconThemeFactory::getInstance()->fromTheme("mail-mark-unread"));
  m_ui->m_actionSwitchImportanceOfSelectedMessages->setIcon(IconThemeFactory::getInstance()->fromTheme("mail-mark-important"));
  m_ui->m_actionOpenSelectedSourceArticlesInternally->setIcon(IconThemeFactory::getInstance()->fromTheme("document-open"));
  m_ui->m_actionOpenSelectedSourceArticlesExternally->setIcon(IconThemeFactory::getInstance()->fromTheme("document-open"));
  m_ui->m_actionOpenSelectedMessagesInternally->setIcon(IconThemeFactory::getInstance()->fromTheme("document-open"));

  // Setup icons for underlying components: opened web browsers...
  foreach (WebBrowser *browser, WebBrowser::runningWebBrowsers()) {
    browser->setupIcons();
  }

  // Setup icons on TabWidget too.
  m_ui->m_tabWidget->setupIcons();
}

void FormMain::loadSize() {
  QRect screen = qApp->desktop()->screenGeometry();

  // Reload main window size & position.
  resize(Settings::getInstance()->value(APP_CFG_GUI,
                                        "window_size",
                                        size()).toSize());
  move(Settings::getInstance()->value(APP_CFG_GUI,
                                      "window_position",
                                      screen.center() - rect().center()).toPoint());

  // Adjust dimensions of "feeds & messages" widget.
  m_ui->m_tabWidget->feedMessageViewer()->loadSize();
}

void FormMain::saveSize() {
  Settings::getInstance()->setValue(APP_CFG_GUI, "window_position", pos());
  Settings::getInstance()->setValue(APP_CFG_GUI, "window_size", size());
  m_ui->m_tabWidget->feedMessageViewer()->saveSize();
}

void FormMain::createConnections() {
  // Core connections.
  connect(qApp, SIGNAL(commitDataRequest(QSessionManager&)),
          this, SLOT(onCommitData(QSessionManager&)));
  connect(qApp, SIGNAL(saveStateRequest(QSessionManager&)),
          this, SLOT(onSaveState(QSessionManager&)));

  // Menu "File" connections.
  connect(m_ui->m_actionQuit, SIGNAL(triggered()), this, SLOT(quit()));

  // Menu "View" connections.
  connect(m_ui->m_actionFullscreen, SIGNAL(triggered(bool)), this, SLOT(switchFullscreenMode(bool)));

  // Menu "Tools" connections.
  connect(m_ui->m_actionSettings, SIGNAL(triggered()), this, SLOT(showSettings()));

  // Menu "Help" connections.
  connect(m_ui->m_actionAboutGuard, SIGNAL(triggered()), this, SLOT(showAbout()));

  // General connections.
  connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(onAboutToQuit()));

  // Menu "Web browser" connections.
  connect(m_ui->m_tabWidget, SIGNAL(currentChanged(int)),
          this, SLOT(loadWebBrowserMenu(int)));
  connect(m_ui->m_actionCloseCurrentTab, SIGNAL(triggered()),
          m_ui->m_tabWidget, SLOT(closeCurrentTab()));
  connect(m_ui->m_actionAddBrowser, SIGNAL(triggered()),
          m_ui->m_tabWidget, SLOT(addEmptyBrowser()));
  connect(m_ui->m_actionCloseAllTabs, SIGNAL(triggered()),
          m_ui->m_tabWidget, SLOT(closeAllTabsExceptCurrent()));
}

void FormMain::loadWebBrowserMenu(int index) {
  WebBrowser *active_browser = m_ui->m_tabWidget->widget(index)->webBrowser();

  m_ui->m_menuCurrentTab->clear();
  if (active_browser != NULL) {
    m_ui->m_menuCurrentTab->addActions(active_browser->globalMenu());

    if (m_ui->m_menuCurrentTab->actions().size() == 0) {
      m_ui->m_menuCurrentTab->insertAction(NULL, m_ui->m_actionNoActions);
    }
  }

  m_ui->m_actionCloseCurrentTab->setEnabled(m_ui->m_tabWidget->tabBar()->tabType(index) == TabBar::Closable);
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
  QPointer<FormAbout> form_pointer = new FormAbout(this);
  form_pointer.data()->exec();
  delete form_pointer.data();
}

void FormMain::showSettings() {
  QPointer<FormSettings> form_pointer = new FormSettings(this);

  if (form_pointer.data()->exec() == QDialog::Accepted) {
    // User applied new settings, reload neede components.
    m_ui->m_tabWidget->checkTabBarVisibility();
  }

  delete form_pointer.data();
}
