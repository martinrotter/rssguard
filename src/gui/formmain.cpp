#include "gui/formmain.h"

#include "core/defs.h"
#include "core/settings.h"
#include "core/systemfactory.h"
#include "core/databasefactory.h"
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
#include <QTimer>


FormMain *FormMain::s_instance;

FormMain::FormMain(QWidget *parent)
  : QMainWindow(parent), m_ui(new Ui::FormMain) {
  m_ui->setupUi(this);

  // Initialize singleton.
  s_instance = this;

  m_statusBar = new StatusBar(this);
  setStatusBar(m_statusBar);

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
}

FormMain *FormMain::instance() {
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
             m_ui->m_actionMarkFeedsAsRead <<
             m_ui->m_actionMarkFeedsAsUnread <<
             m_ui->m_actionClearFeeds <<
             m_ui->m_actionMarkSelectedMessagesAsRead <<
             m_ui->m_actionMarkSelectedMessagesAsUnread <<
             m_ui->m_actionSwitchImportanceOfSelectedMessages <<
             m_ui->m_actionDeleteSelectedMessages <<
             m_ui->m_actionUpdateAllFeeds <<
             m_ui->m_actionUpdateSelectedFeedsCategories <<
             m_ui->m_actionEditSelectedFeedCategory <<
             m_ui->m_actionDeleteSelectedFeedsCategories;

  return actions;
}

StatusBar *FormMain::statusBar() {
  return m_statusBar;
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
      SystemTrayIcon::instance()->showMessage(APP_NAME,
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
  qDebug("OS asked application to commit its data.");

  manager.setRestartHint(QSessionManager::RestartNever);
  manager.release();
}

void FormMain::onSaveState(QSessionManager &manager) {
  qDebug("OS asked application to save its state.");

  manager.setRestartHint(QSessionManager::RestartNever);
  manager.release();
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

  DatabaseFactory::instance()->saveMemoryDatabase();

  saveSize();
}

bool FormMain::event(QEvent *event) {
  return QMainWindow::event(event);
}

void FormMain::setupIcons() {
  // Setup icons of this main window.
  m_ui->m_actionSettings->setIcon(IconThemeFactory::instance()->fromTheme("preferences-system"));
  m_ui->m_actionQuit->setIcon(IconThemeFactory::instance()->fromTheme("application-exit"));
  m_ui->m_actionAboutGuard->setIcon(IconThemeFactory::instance()->fromTheme("help-about"));
  m_ui->m_actionImport->setIcon(IconThemeFactory::instance()->fromTheme("document-import"));
  m_ui->m_actionExport->setIcon(IconThemeFactory::instance()->fromTheme("document-export"));
  m_ui->m_actionFullscreen->setIcon(IconThemeFactory::instance()->fromTheme("view-fullscreen"));

  // Web browser.
  m_ui->m_actionAddBrowser->setIcon(IconThemeFactory::instance()->fromTheme("list-add"));
  m_ui->m_actionCloseCurrentTab->setIcon(IconThemeFactory::instance()->fromTheme("list-remove"));
  m_ui->m_actionCloseAllTabs->setIcon(IconThemeFactory::instance()->fromTheme("list-remove"));
  m_ui->m_menuCurrentTab->setIcon(IconThemeFactory::instance()->fromTheme("go-home"));

  // Feeds/messages.
  m_ui->m_actionUpdateAllFeeds->setIcon(IconThemeFactory::instance()->fromTheme("document-save-as"));
  m_ui->m_actionUpdateSelectedFeedsCategories->setIcon(IconThemeFactory::instance()->fromTheme("document-save"));
  m_ui->m_actionClearFeeds->setIcon(IconThemeFactory::instance()->fromTheme("mail-mark-junk"));
  m_ui->m_actionDeleteSelectedFeedsCategories->setIcon(IconThemeFactory::instance()->fromTheme("edit-delete"));
  m_ui->m_actionDeleteSelectedMessages->setIcon(IconThemeFactory::instance()->fromTheme("mail-mark-junk"));
  m_ui->m_actionAddNewCategory->setIcon(IconThemeFactory::instance()->fromTheme("document-new"));
  m_ui->m_actionAddNewFeed->setIcon(IconThemeFactory::instance()->fromTheme("document-new"));
  m_ui->m_actionEditSelectedFeedCategory->setIcon(IconThemeFactory::instance()->fromTheme("gnome-other"));
  m_ui->m_actionMarkAllFeedsRead->setIcon(IconThemeFactory::instance()->fromTheme("mail-mark-not-junk"));
  m_ui->m_actionMarkFeedsAsRead->setIcon(IconThemeFactory::instance()->fromTheme("mail-mark-not-junk"));
  m_ui->m_actionMarkFeedsAsUnread->setIcon(IconThemeFactory::instance()->fromTheme("mail-mark-important"));
  m_ui->m_actionMarkFeedsAsRead->setIcon(IconThemeFactory::instance()->fromTheme("mail-mark-not-junk"));
  m_ui->m_actionMarkSelectedMessagesAsRead->setIcon(IconThemeFactory::instance()->fromTheme("mail-mark-not-junk"));
  m_ui->m_actionMarkSelectedMessagesAsUnread->setIcon(IconThemeFactory::instance()->fromTheme("mail-mark-important"));
  m_ui->m_actionSwitchImportanceOfSelectedMessages->setIcon(IconThemeFactory::instance()->fromTheme("favorites"));
  m_ui->m_actionOpenSelectedSourceArticlesInternally->setIcon(IconThemeFactory::instance()->fromTheme("document-open"));
  m_ui->m_actionOpenSelectedSourceArticlesExternally->setIcon(IconThemeFactory::instance()->fromTheme("document-open"));
  m_ui->m_actionOpenSelectedMessagesInternally->setIcon(IconThemeFactory::instance()->fromTheme("document-open"));
  m_ui->m_actionViewSelectedItemsNewspaperMode->setIcon(IconThemeFactory::instance()->fromTheme("document-multiple"));

  // Setup icons for underlying components: opened web browsers...
  foreach (WebBrowser *browser, WebBrowser::runningWebBrowsers()) {
    browser->setupIcons();
  }

  // Setup icons on TabWidget too.
  m_ui->m_tabWidget->setupIcons();
}

void FormMain::loadSize() {
  QRect screen = qApp->desktop()->screenGeometry();
  Settings *settings = Settings::instance();

  // Reload main window size & position.
  resize(settings->value(APP_CFG_GUI,
                         "window_size",
                         size()).toSize());
  move(settings->value(APP_CFG_GUI,
                       "window_position",
                       screen.center() - rect().center()).toPoint());

  // Adjust dimensions of "feeds & messages" widget.
  m_ui->m_tabWidget->feedMessageViewer()->loadSize();
}

void FormMain::saveSize() {
  Settings *settings = Settings::instance();

  settings->setValue(APP_CFG_GUI, "window_position", pos());
  settings->setValue(APP_CFG_GUI, "window_size", size());
  m_ui->m_tabWidget->feedMessageViewer()->saveSize();
}

void FormMain::createConnections() {
  // Status bar connections.
  connect(m_statusBar->fullscreenSwitcher(), SIGNAL(clicked()),
          m_ui->m_actionFullscreen, SLOT(trigger()));

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

void FormMain::changeEvent(QEvent *event) {
  switch (event->type()) {
    case QEvent::WindowStateChange: {
      if (SystemTrayIcon::isSystemTrayActivated()) {
        if (this->windowState() & Qt::WindowMinimized) {
          QTimer::singleShot(250, this, SLOT(hide()));
        }
      }

      break;
    }

    default:
      break;
  }

  QMainWindow::changeEvent(event);
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
