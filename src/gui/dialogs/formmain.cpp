// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/dialogs/formmain.h"

#include "definitions/definitions.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/application.h"
#include "miscellaneous/systemfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/databasefactory.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/webfactory.h"
#include "network-web/webbrowser.h"
#include "gui/feedsview.h"
#include "gui/messagebox.h"
#include "gui/systemtrayicon.h"
#include "gui/tabbar.h"
#include "gui/statusbar.h"
#include "gui/feedmessageviewer.h"
#include "gui/plaintoolbutton.h"
#include "gui/dialogs/formabout.h"
#include "gui/dialogs/formsettings.h"
#include "gui/dialogs/formupdate.h"
#include "gui/dialogs/formbackupdatabasesettings.h"
#include "gui/dialogs/formrestoredatabasesettings.h"
#include "gui/dialogs/formaddaccount.h"
#include "gui/notifications/notification.h"
#include "services/abstract/serviceroot.h"
#include "services/abstract/recyclebin.h"
#include "services/standard/gui/formstandardimportexport.h"

#include <QCloseEvent>
#include <QSessionManager>
#include <QRect>
#include <QScopedPointer>
#include <QDesktopWidget>
#include <QReadWriteLock>
#include <QTimer>
#include <QFileDialog>
#include <QTextStream>

#include "services/owncloud/network/owncloudnetworkfactory.h"


FormMain::FormMain(QWidget *parent, Qt::WindowFlags f)
  : QMainWindow(parent, f), m_ui(new Ui::FormMain) {
  m_ui->setupUi(this);
  qApp->setMainForm(this);

  m_statusBar = new StatusBar(this);
  setStatusBar(m_statusBar);

  // Prepare main window and tabs.
  prepareMenus();

  // Establish connections.
  createConnections();

  // Add these actions to the list of actions of the main window.
  // This allows to use actions via shortcuts
  // even if main menu is not visible.
  addActions(allActions());

  // Prepare tabs.
  m_ui->m_tabWidget->initializeTabs();

  // Setup some appearance of the window.
  setupIcons();
  loadSize();

  // Initialize the web factory.
  WebFactory::instance()->loadState();



  OwnCloudNetworkFactory fac;

  fac.setAuthIsUsed(true);
  fac.setUrl("https://cloud.yarpen.cz");
  fac.setAuthPassword("rssguard135");
  fac.setAuthUsername("rssguard");

  OwnCloudStatusResponse user = fac.status();

  QString ver =  user.version();

  int aaaa = 0;
}

FormMain::~FormMain() {
  qDebug("Destroying FormMain instance.");
}

QList<QAction*> FormMain::allActions() const {
  QList<QAction*> actions;

  // Add basic actions.
  actions << m_ui->m_actionSettings;
  actions << m_ui->m_actionDownloadManager;
  actions << m_ui->m_actionRestoreDatabaseSettings;
  actions << m_ui->m_actionBackupDatabaseSettings;
  actions << m_ui->m_actionRestart;
  actions << m_ui->m_actionQuit;
  actions << m_ui->m_actionFullscreen;
  actions << m_ui->m_actionAboutGuard;
  actions << m_ui->m_actionSwitchFeedsList;
  actions << m_ui->m_actionSwitchMainWindow;
  actions << m_ui->m_actionSwitchMainMenu;
  actions << m_ui->m_actionSwitchToolBars;
  actions << m_ui->m_actionSwitchListHeaders;
  actions << m_ui->m_actionSwitchStatusBar;
  actions << m_ui->m_actionSwitchMessageListOrientation;

  // Add web browser actions.
  actions << m_ui->m_actionAddBrowser;
  actions << m_ui->m_actionCloseCurrentTab;
  actions << m_ui->m_actionCloseAllTabs;

  // Add feeds/messages actions.
  actions << m_ui->m_actionOpenSelectedSourceArticlesExternally;
  actions << m_ui->m_actionOpenSelectedSourceArticlesInternally;
  actions << m_ui->m_actionOpenSelectedMessagesInternally;
  actions << m_ui->m_actionMarkAllItemsRead;
  actions << m_ui->m_actionMarkSelectedItemsAsRead;
  actions << m_ui->m_actionMarkSelectedItemsAsUnread;
  actions << m_ui->m_actionClearSelectedItems;
  actions << m_ui->m_actionClearAllItems;
  actions << m_ui->m_actionShowOnlyUnreadItems;
  actions << m_ui->m_actionMarkSelectedMessagesAsRead;
  actions << m_ui->m_actionMarkSelectedMessagesAsUnread;
  actions << m_ui->m_actionSwitchImportanceOfSelectedMessages;
  actions << m_ui->m_actionDeleteSelectedMessages;
  actions << m_ui->m_actionUpdateAllItems;
  actions << m_ui->m_actionUpdateSelectedItems;
  actions << m_ui->m_actionStopRunningItemsUpdate;
  actions << m_ui->m_actionEditSelectedItem;
  actions << m_ui->m_actionDeleteSelectedItem;
  actions << m_ui->m_actionServiceAdd;
  actions << m_ui->m_actionServiceEdit;
  actions << m_ui->m_actionServiceDelete;
  actions << m_ui->m_actionAddFeedIntoSelectedAccount;
  actions << m_ui->m_actionAddCategoryIntoSelectedAccount;
  actions << m_ui->m_actionViewSelectedItemsNewspaperMode;
  actions << m_ui->m_actionSelectNextItem;
  actions << m_ui->m_actionSelectPreviousItem;
  actions << m_ui->m_actionSelectNextMessage;
  actions << m_ui->m_actionSelectPreviousMessage;
  actions << m_ui->m_actionSelectNextUnreadMessage;
  actions << m_ui->m_actionExpandCollapseItem;

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
    m_trayMenu->addAction(m_ui->m_actionSwitchMainWindow);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_ui->m_actionUpdateAllItems);
    m_trayMenu->addAction(m_ui->m_actionMarkAllItemsRead);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_ui->m_actionSettings);
    m_trayMenu->addAction(m_ui->m_actionQuit);

    qDebug("Creating tray icon menu.");
  }
}

void FormMain::switchFullscreenMode() {
  if (!isFullScreen()) {
    showFullScreen();
  }
  else {
    showNormal();
  }
}

void FormMain::updateAddItemMenu() {
  // NOTE: Clear here deletes items from memory but only those OWNED by the menu.
  m_ui->m_menuAddItem->clear();

  foreach (ServiceRoot *activated_root, tabWidget()->feedMessageViewer()->feedsView()->sourceModel()->serviceRoots()) {
    QMenu *root_menu = new QMenu(activated_root->title(), m_ui->m_menuAddItem);
    root_menu->setIcon(activated_root->icon());
    root_menu->setToolTip(activated_root->description());

    QList<QAction*> specific_root_actions = activated_root->addItemMenu();

    if (activated_root->supportsCategoryAdding()) {
      QAction *action_new_category = new QAction(qApp->icons()->fromTheme(QSL("folder-category")),
                                                 tr("Add new category"),
                                                 m_ui->m_menuAddItem);
      root_menu->addAction(action_new_category);
      connect(action_new_category, SIGNAL(triggered()), activated_root, SLOT(addNewCategory()));
    }

    if (activated_root->supportsFeedAdding()) {
      QAction *action_new_feed = new QAction(qApp->icons()->fromTheme(QSL("folder-feed")),
                                             tr("Add new feed"),
                                             m_ui->m_menuAddItem);
      root_menu->addAction(action_new_feed);
      connect(action_new_feed, SIGNAL(triggered()), activated_root, SLOT(addNewFeed()));
    }

    if (!specific_root_actions.isEmpty()) {
      if (!root_menu->isEmpty()) {
        root_menu->addSeparator();
      }

      root_menu->addActions(specific_root_actions);
    }

    m_ui->m_menuAddItem->addMenu(root_menu);
  }

  if (!m_ui->m_menuAddItem->isEmpty()) {
    m_ui->m_menuAddItem->addSeparator();
    m_ui->m_menuAddItem->addAction(m_ui->m_actionAddCategoryIntoSelectedAccount);
    m_ui->m_menuAddItem->addAction(m_ui->m_actionAddFeedIntoSelectedAccount);
  }
  else {
    m_ui->m_menuAddItem->addAction(m_ui->m_actionNoActions);
  }
}

void FormMain::updateRecycleBinMenu() {
  m_ui->m_menuRecycleBin->clear();

  foreach (const ServiceRoot *activated_root, tabWidget()->feedMessageViewer()->feedsView()->sourceModel()->serviceRoots()) {
    QMenu *root_menu = new QMenu(activated_root->title(), m_ui->m_menuRecycleBin);
    root_menu->setIcon(activated_root->icon());
    root_menu->setToolTip(activated_root->description());

    RecycleBin *bin = activated_root->recycleBin();

    if (bin == NULL) {
      QAction *no_action = new QAction(qApp->icons()->fromTheme(QSL("dialog-error")),
                                       tr("No recycle bin"),
                                       m_ui->m_menuRecycleBin);
      no_action->setEnabled(false);
      root_menu->addAction(no_action);
    }
    else {
      QAction *restore_action = new QAction(qApp->icons()->fromTheme(QSL("recycle-bin-restore-all")),
                                            tr("Restore recycle bin"),
                                            m_ui->m_menuRecycleBin);
      QAction *empty_action = new QAction(qApp->icons()->fromTheme(QSL("recycle-bin-empty")),
                                          tr("Empty recycle bin"),
                                          m_ui->m_menuRecycleBin);

      connect(restore_action, SIGNAL(triggered()), bin, SLOT(restore()));
      connect(empty_action, SIGNAL(triggered()), bin, SLOT(empty()));

      root_menu->addAction(restore_action);
      root_menu->addAction(empty_action);
    }

    m_ui->m_menuRecycleBin->addMenu(root_menu);
  }

  if (!m_ui->m_menuRecycleBin->isEmpty()) {
    m_ui->m_menuRecycleBin->addSeparator();
  }

  m_ui->m_menuRecycleBin->addAction(m_ui->m_actionRestoreAllRecycleBins);
  m_ui->m_menuRecycleBin->addAction(m_ui->m_actionEmptyAllRecycleBins);
}

void FormMain::updateAccountsMenu() {
  m_ui->m_menuAccounts->clear();

  foreach (ServiceRoot *activated_root, tabWidget()->feedMessageViewer()->feedsView()->sourceModel()->serviceRoots()) {
    QMenu *root_menu = new QMenu(activated_root->title(), m_ui->m_menuAccounts);
    root_menu->setIcon(activated_root->icon());
    root_menu->setToolTip(activated_root->description());

    QList<QAction*> root_actions = activated_root->serviceMenu();

    if (root_actions.isEmpty()) {
      QAction *no_action = new QAction(qApp->icons()->fromTheme(QSL("dialog-error")),
                                       tr("No possible actions"),
                                       m_ui->m_menuAccounts);
      no_action->setEnabled(false);
      root_menu->addAction(no_action);
    }
    else {
      root_menu->addActions(root_actions);
    }

    m_ui->m_menuAccounts->addMenu(root_menu);
  }

  if (m_ui->m_menuAccounts->actions().size() > 0) {
    m_ui->m_menuAccounts->addSeparator();
  }

  m_ui->m_menuAccounts->addAction(m_ui->m_actionServiceAdd);
  m_ui->m_menuAccounts->addAction(m_ui->m_actionServiceEdit);
  m_ui->m_menuAccounts->addAction(m_ui->m_actionServiceDelete);
}

void FormMain::switchVisibility(bool force_hide) {
  if (force_hide || isVisible()) {
    if (SystemTrayIcon::isSystemTrayActivated()) {
      hide();
    }
    else {
      // Window gets minimized in single-window mode.
      showMinimized();
    }
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
  Application::alert(this);
}

void FormMain::setupIcons() {
  IconFactory *icon_theme_factory = qApp->icons();

  // Setup icons of this main window.
  m_ui->m_actionDownloadManager->setIcon(icon_theme_factory->fromTheme(QSL("download-manager")));
  m_ui->m_actionSettings->setIcon(icon_theme_factory->fromTheme(QSL("application-settings")));
  m_ui->m_actionQuit->setIcon(icon_theme_factory->fromTheme(QSL("application-exit")));
  m_ui->m_actionRestart->setIcon(icon_theme_factory->fromTheme(QSL("go-refresh")));
  m_ui->m_actionAboutGuard->setIcon(icon_theme_factory->fromTheme(QSL("application-about")));
  m_ui->m_actionCheckForUpdates->setIcon(icon_theme_factory->fromTheme(QSL("check-for-updates")));
  m_ui->m_actionCleanupDatabase->setIcon(icon_theme_factory->fromTheme(QSL("cleanup-database")));
  m_ui->m_actionReportBugGitHub->setIcon(icon_theme_factory->fromTheme(QSL("application-report-bug")));
  m_ui->m_actionReportBugBitBucket->setIcon(icon_theme_factory->fromTheme(QSL("application-report-bug")));
  m_ui->m_actionBackupDatabaseSettings->setIcon(icon_theme_factory->fromTheme(QSL("document-export")));
  m_ui->m_actionRestoreDatabaseSettings->setIcon(icon_theme_factory->fromTheme(QSL("document-import")));
  m_ui->m_actionDonate->setIcon(icon_theme_factory->fromTheme(QSL("application-donate")));
  m_ui->m_actionDisplayWiki->setIcon(icon_theme_factory->fromTheme(QSL("application-wiki")));

  // View.
  m_ui->m_actionSwitchMainWindow->setIcon(icon_theme_factory->fromTheme(QSL("view-switch-window")));
  m_ui->m_actionFullscreen->setIcon(icon_theme_factory->fromTheme(QSL("view-fullscreen")));
  m_ui->m_actionSwitchFeedsList->setIcon(icon_theme_factory->fromTheme(QSL("view-switch-list")));
  m_ui->m_actionSwitchMainMenu->setIcon(icon_theme_factory->fromTheme(QSL("view-switch-menu")));
  m_ui->m_actionSwitchToolBars->setIcon(icon_theme_factory->fromTheme(QSL("view-switch-list")));
  m_ui->m_actionSwitchListHeaders->setIcon(icon_theme_factory->fromTheme(QSL("view-switch-list")));
  m_ui->m_actionSwitchStatusBar->setIcon(icon_theme_factory->fromTheme(QSL("dialog-information")));
  m_ui->m_actionSwitchMessageListOrientation->setIcon(icon_theme_factory->fromTheme(QSL("view-switch-layout-direction")));
  m_ui->m_menuShowHide->setIcon(icon_theme_factory->fromTheme(QSL("view-switch")));

  // Web browser.
  m_ui->m_actionAddBrowser->setIcon(icon_theme_factory->fromTheme(QSL("list-add")));
  m_ui->m_actionCloseCurrentTab->setIcon(icon_theme_factory->fromTheme(QSL("list-remove")));
  m_ui->m_actionCloseAllTabs->setIcon(icon_theme_factory->fromTheme(QSL("list-remove")));
  m_ui->m_menuCurrentTab->setIcon(icon_theme_factory->fromTheme(QSL("list-current")));
  m_ui->m_menuWebSettings->setIcon(icon_theme_factory->fromTheme(QSL("application-settings")));
  m_ui->m_actionWebAutoloadImages->setIcon(icon_theme_factory->fromTheme(QSL("image-generic")));
  m_ui->m_actionWebEnableExternalPlugins->setIcon(icon_theme_factory->fromTheme(QSL("web-flash")));
  m_ui->m_actionWebEnableJavascript->setIcon(icon_theme_factory->fromTheme(QSL("web-javascript")));

  // Feeds/messages.
  m_ui->m_menuAddItem->setIcon(icon_theme_factory->fromTheme(QSL("item-new")));
  m_ui->m_actionStopRunningItemsUpdate->setIcon(icon_theme_factory->fromTheme(QSL("go-stop")));
  m_ui->m_actionUpdateAllItems->setIcon(icon_theme_factory->fromTheme(QSL("item-update-all")));
  m_ui->m_actionUpdateSelectedItems->setIcon(icon_theme_factory->fromTheme(QSL("item-update-selected")));
  m_ui->m_actionClearSelectedItems->setIcon(icon_theme_factory->fromTheme(QSL("mail-remove")));
  m_ui->m_actionClearAllItems->setIcon(icon_theme_factory->fromTheme(QSL("mail-remove")));
  m_ui->m_actionDeleteSelectedItem->setIcon(icon_theme_factory->fromTheme(QSL("item-remove")));
  m_ui->m_actionDeleteSelectedMessages->setIcon(icon_theme_factory->fromTheme(QSL("mail-remove")));
  m_ui->m_actionEditSelectedItem->setIcon(icon_theme_factory->fromTheme(QSL("item-edit")));
  m_ui->m_actionMarkAllItemsRead->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-read")));
  m_ui->m_actionMarkSelectedItemsAsRead->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-read")));
  m_ui->m_actionMarkSelectedItemsAsUnread->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-unread")));
  m_ui->m_actionMarkSelectedMessagesAsRead->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-read")));
  m_ui->m_actionMarkSelectedMessagesAsUnread->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-unread")));
  m_ui->m_actionSwitchImportanceOfSelectedMessages->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-favorite")));
  m_ui->m_actionOpenSelectedSourceArticlesInternally->setIcon(icon_theme_factory->fromTheme(QSL("item-open-internal")));
  m_ui->m_actionOpenSelectedSourceArticlesExternally->setIcon(icon_theme_factory->fromTheme(QSL("item-open-external")));
  m_ui->m_actionOpenSelectedMessagesInternally->setIcon(icon_theme_factory->fromTheme(QSL("item-open-internal")));
  m_ui->m_actionSendMessageViaEmail->setIcon(icon_theme_factory->fromTheme(QSL("item-send-email")));
  m_ui->m_actionViewSelectedItemsNewspaperMode->setIcon(icon_theme_factory->fromTheme(QSL("item-newspaper")));
  m_ui->m_actionSelectNextItem->setIcon(icon_theme_factory->fromTheme(QSL("go-down")));
  m_ui->m_actionSelectPreviousItem->setIcon(icon_theme_factory->fromTheme(QSL("go-up")));
  m_ui->m_actionSelectNextMessage->setIcon(icon_theme_factory->fromTheme(QSL("go-down")));
  m_ui->m_actionSelectPreviousMessage->setIcon(icon_theme_factory->fromTheme(QSL("go-up")));
  m_ui->m_actionSelectNextUnreadMessage->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-unread")));
  m_ui->m_actionShowOnlyUnreadItems->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-unread")));
  m_ui->m_actionExpandCollapseItem->setIcon(icon_theme_factory->fromTheme(QSL("expand-collapse")));
  m_ui->m_actionRestoreSelectedMessages->setIcon(icon_theme_factory->fromTheme(QSL("recycle-bin-restore-one")));
  m_ui->m_actionRestoreAllRecycleBins->setIcon(icon_theme_factory->fromTheme(QSL("recycle-bin-restore-all")));
  m_ui->m_actionEmptyAllRecycleBins->setIcon(icon_theme_factory->fromTheme(QSL("recycle-bin-empty")));
  m_ui->m_actionServiceAdd->setIcon(icon_theme_factory->fromTheme(QSL("item-new")));
  m_ui->m_actionServiceEdit->setIcon(icon_theme_factory->fromTheme(QSL("item-edit")));
  m_ui->m_actionServiceDelete->setIcon(icon_theme_factory->fromTheme(QSL("item-remove")));
  m_ui->m_actionAddFeedIntoSelectedAccount->setIcon(icon_theme_factory->fromTheme(QSL("folder-feed")));
  m_ui->m_actionAddCategoryIntoSelectedAccount->setIcon(icon_theme_factory->fromTheme(QSL("folder-category")));

  // Setup icons for underlying components: opened web browsers...
  foreach (WebBrowser *browser, WebBrowser::runningWebBrowsers()) {
    browser->setupIcons();
  }

  // Setup icons on TabWidget too.
  m_ui->m_tabWidget->setupIcons();

  // Most of icons are loaded, clear the cache.
  qApp->icons()->clearCache();
}

void FormMain::loadSize() {
  const QRect screen = qApp->desktop()->screenGeometry();
  const Settings *settings = qApp->settings();

  // Reload main window size & position.
  resize(settings->value(GROUP(GUI), GUI::MainWindowInitialSize, size()).toSize());
  move(settings->value(GROUP(GUI), GUI::MainWindowInitialPosition, screen.center() - rect().center()).toPoint());

  // If user exited the application while in fullsreen mode,
  // then re-enable it now.
  if (settings->value(GROUP(GUI), SETTING(GUI::MainWindowStartsFullscreen)).toBool()) {
    m_ui->m_actionFullscreen->setChecked(true);
  }

  if (settings->value(GROUP(GUI), SETTING(GUI::MainWindowStartsMaximized)).toBool()) {
    setWindowState(windowState() | Qt::WindowMaximized);
  }

  // Hide the main menu if user wants it.
  m_ui->m_actionSwitchMainMenu->setChecked(settings->value(GROUP(GUI), SETTING(GUI::MainMenuVisible)).toBool());

  // Adjust dimensions of "feeds & messages" widget.
  m_ui->m_tabWidget->feedMessageViewer()->loadSize();
  m_ui->m_actionSwitchToolBars->setChecked(settings->value(GROUP(GUI), SETTING(GUI::ToolbarsVisible)).toBool());
  m_ui->m_actionSwitchListHeaders->setChecked(settings->value(GROUP(GUI), SETTING(GUI::ListHeadersVisible)).toBool());
  m_ui->m_actionSwitchStatusBar->setChecked(settings->value(GROUP(GUI), SETTING(GUI::StatusBarVisible)).toBool());

  // Make sure that only unread feeds are shown if user has that feature set on.
  m_ui->m_actionShowOnlyUnreadItems->setChecked(settings->value(GROUP(Feeds), SETTING(Feeds::ShowOnlyUnreadFeeds)).toBool());
}

void FormMain::saveSize() {
  Settings *settings = qApp->settings();
  const bool is_fullscreen = isFullScreen();
  const bool is_maximized = isMaximized();

  if (is_fullscreen) {
    m_ui->m_actionFullscreen->setChecked(false);
  }

  if (is_maximized) {
    setWindowState(windowState() & ~Qt::WindowMaximized);
  }

  settings->setValue(GROUP(GUI), GUI::MainMenuVisible, m_ui->m_actionSwitchMainMenu->isChecked());
  settings->setValue(GROUP(GUI), GUI::MainWindowInitialPosition, pos());
  settings->setValue(GROUP(GUI), GUI::MainWindowInitialSize, size());
  settings->setValue(GROUP(GUI), GUI::MainWindowStartsMaximized, is_maximized);
  settings->setValue(GROUP(GUI), GUI::MainWindowStartsFullscreen, is_fullscreen);
  settings->setValue(GROUP(GUI), GUI::StatusBarVisible, m_ui->m_actionSwitchStatusBar->isChecked());

  m_ui->m_tabWidget->feedMessageViewer()->saveSize();
}

void FormMain::createConnections() {
  // Status bar connections.
  connect(m_statusBar->fullscreenSwitcher(), SIGNAL(toggled(bool)), m_ui->m_actionFullscreen, SLOT(setChecked(bool)));
  connect(m_ui->m_actionFullscreen, SIGNAL(toggled(bool)), m_statusBar->fullscreenSwitcher(), SLOT(setChecked(bool)));

  connect(m_ui->m_menuAddItem, SIGNAL(aboutToShow()), this, SLOT(updateAddItemMenu()));
  connect(m_ui->m_menuRecycleBin, SIGNAL(aboutToShow()), this, SLOT(updateRecycleBinMenu()));
  connect(m_ui->m_menuAccounts, SIGNAL(aboutToShow()), this, SLOT(updateAccountsMenu()));

  connect(m_ui->m_actionServiceDelete, SIGNAL(triggered()), m_ui->m_actionDeleteSelectedItem, SIGNAL(triggered()));
  connect(m_ui->m_actionServiceEdit, SIGNAL(triggered()), m_ui->m_actionEditSelectedItem, SIGNAL(triggered()));

  // Menu "File" connections.
  connect(m_ui->m_actionBackupDatabaseSettings, SIGNAL(triggered()), this, SLOT(backupDatabaseSettings()));
  connect(m_ui->m_actionRestoreDatabaseSettings, SIGNAL(triggered()), this, SLOT(restoreDatabaseSettings()));
  connect(m_ui->m_actionRestart, SIGNAL(triggered()), qApp, SLOT(restart()));
  connect(m_ui->m_actionQuit, SIGNAL(triggered()), qApp, SLOT(quit()));
  connect(m_ui->m_actionServiceAdd, SIGNAL(triggered()), this, SLOT(showAddAccountDialog()));

  // Menu "View" connections.
  connect(m_ui->m_actionFullscreen, SIGNAL(toggled(bool)), this, SLOT(switchFullscreenMode()));
  connect(m_ui->m_actionSwitchMainMenu, SIGNAL(toggled(bool)), m_ui->m_menuBar, SLOT(setVisible(bool)));
  connect(m_ui->m_actionSwitchMainWindow, SIGNAL(triggered()), this, SLOT(switchVisibility()));
  connect(m_ui->m_actionSwitchStatusBar, SIGNAL(toggled(bool)), statusBar(), SLOT(setVisible(bool)));

  // Menu "Tools" connections.
  connect(m_ui->m_actionSettings, SIGNAL(triggered()), this, SLOT(showSettings()));
  connect(m_ui->m_actionDownloadManager, SIGNAL(triggered()), m_ui->m_tabWidget, SLOT(showDownloadManager()));

  // Menu "Help" connections.
  connect(m_ui->m_actionAboutGuard, SIGNAL(triggered()), this, SLOT(showAbout()));
  connect(m_ui->m_actionCheckForUpdates, SIGNAL(triggered()), this, SLOT(showUpdates()));
  connect(m_ui->m_actionReportBugGitHub, SIGNAL(triggered()), this, SLOT(reportABugOnGitHub()));
  connect(m_ui->m_actionReportBugBitBucket, SIGNAL(triggered()), this, SLOT(reportABugOnBitBucket()));
  connect(m_ui->m_actionDonate, SIGNAL(triggered()), this, SLOT(donate()));
  connect(m_ui->m_actionDisplayWiki, SIGNAL(triggered()), this, SLOT(showWiki()));

  // Menu "Web browser" connections.
  connect(m_ui->m_tabWidget, SIGNAL(currentChanged(int)), this, SLOT(loadWebBrowserMenu(int)));
  connect(m_ui->m_actionCloseCurrentTab, SIGNAL(triggered()), m_ui->m_tabWidget, SLOT(closeCurrentTab()));
  connect(m_ui->m_actionAddBrowser, SIGNAL(triggered()), m_ui->m_tabWidget, SLOT(addEmptyBrowser()));
  connect(m_ui->m_actionCloseAllTabs, SIGNAL(triggered()), m_ui->m_tabWidget, SLOT(closeAllTabsExceptCurrent()));
  connect(m_ui->m_actionWebAutoloadImages, SIGNAL(toggled(bool)), WebFactory::instance(), SLOT(switchImages(bool)));
  connect(m_ui->m_actionWebEnableExternalPlugins, SIGNAL(toggled(bool)), WebFactory::instance(), SLOT(switchPlugins(bool)));
  connect(m_ui->m_actionWebEnableJavascript, SIGNAL(toggled(bool)), WebFactory::instance(), SLOT(switchJavascript(bool)));

  connect(WebFactory::instance(), SIGNAL(imagesLoadingSwitched(bool)), m_ui->m_actionWebAutoloadImages, SLOT(setChecked(bool)));
  connect(WebFactory::instance(), SIGNAL(javascriptSwitched(bool)), m_ui->m_actionWebEnableJavascript, SLOT(setChecked(bool)));
  connect(WebFactory::instance(), SIGNAL(pluginsSwitched(bool)), m_ui->m_actionWebEnableExternalPlugins, SLOT(setChecked(bool)));
}

void FormMain::loadWebBrowserMenu(int index) {
  const WebBrowser *active_browser = m_ui->m_tabWidget->widget(index)->webBrowser();

  m_ui->m_menuCurrentTab->clear();

  if (active_browser != NULL) {
    m_ui->m_menuCurrentTab->setEnabled(true);
    m_ui->m_menuCurrentTab->addActions(active_browser->globalMenu());

    if (m_ui->m_menuCurrentTab->actions().size() == 0) {
      m_ui->m_menuCurrentTab->insertAction(NULL, m_ui->m_actionNoActions);
    }
  }
  else {
    m_ui->m_menuCurrentTab->setEnabled(false);
  }

  m_ui->m_actionCloseCurrentTab->setEnabled(m_ui->m_tabWidget->tabBar()->tabType(index) == TabBar::Closable);
}

void FormMain::backupDatabaseSettings() {
  QScopedPointer<FormBackupDatabaseSettings> form(new FormBackupDatabaseSettings(this));
  form->exec();
}

void FormMain::restoreDatabaseSettings() {
  QScopedPointer<FormRestoreDatabaseSettings> form(new FormRestoreDatabaseSettings(this));
  form->exec();
}

void FormMain::changeEvent(QEvent *event) {
  switch (event->type()) {
    case QEvent::WindowStateChange: {
      if (windowState() & Qt::WindowMinimized &&
          SystemTrayIcon::isSystemTrayActivated() &&
          qApp->settings()->value(GROUP(GUI), SETTING(GUI::HideMainWindowWhenMinimized)).toBool()) {
        event->ignore();
        QTimer::singleShot(CHANGE_EVENT_DELAY, this, SLOT(switchVisibility()));
      }

      break;
    }

    default:
      break;
  }

  QMainWindow::changeEvent(event);
}

void FormMain::showAbout() {
  QScopedPointer<FormAbout> form_pointer(new FormAbout(this));
  form_pointer->exec();
}

void FormMain::showUpdates() {
  QScopedPointer<FormUpdate> form_update(new FormUpdate(this));
  form_update->exec();
}

void FormMain::showWiki() {
  if (!WebFactory::instance()->openUrlInExternalBrowser(APP_URL_WIKI)) {
    qApp->showGuiMessage(tr("Cannot open external browser"),
                         tr("Cannot open external browser. Navigate to application website manually."),
                         QSystemTrayIcon::Warning, this, true);
  }
}

void FormMain::showAddAccountDialog() {
  QScopedPointer<FormAddAccount> form_update(new FormAddAccount(qApp->feedServices(),
                                                                tabWidget()->feedMessageViewer()->feedsView()->sourceModel(),
                                                                this));
  form_update->exec();
}

void FormMain::reportABugOnGitHub() {
  if (!WebFactory::instance()->openUrlInExternalBrowser(APP_URL_ISSUES_NEW_GITHUB)) {
    qApp->showGuiMessage(tr("Cannot open external browser"),
                         tr("Cannot open external browser. Navigate to application website manually."),
                         QSystemTrayIcon::Warning, this, true);
  }
}

void FormMain::reportABugOnBitBucket() {
  if (!WebFactory::instance()->openUrlInExternalBrowser(APP_URL_ISSUES_NEW_BITBUCKET)) {
    qApp->showGuiMessage(tr("Cannot open external browser"),
                         tr("Cannot open external browser. Navigate to application website manually."),
                         QSystemTrayIcon::Warning, this, true);
  }
}

void FormMain::donate() {
  if (!WebFactory::instance()->openUrlInExternalBrowser(APP_DONATE_URL)) {
    qApp->showGuiMessage(tr("Cannot open external browser"),
                         tr("Cannot open external browser. Navigate to application website manually."),
                         QSystemTrayIcon::Warning, this, true);
  }
}

void FormMain::showSettings() {
  QScopedPointer<FormSettings> form_pointer(new FormSettings(this));
  form_pointer->exec();
}
