// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/formmain.h"

#include "definitions/definitions.h"
#include "definitions/globals.h"
#include "gui/dialogs/formabout.h"
#include "gui/dialogs/formaddaccount.h"
#include "gui/dialogs/formbackupdatabasesettings.h"
#include "gui/dialogs/formdatabasecleanup.h"
#include "gui/dialogs/formrestoredatabasesettings.h"
#include "gui/dialogs/formsettings.h"
#include "gui/dialogs/formupdate.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/guiutilities.h"
#include "gui/messagepreviewer.h"
#include "gui/messagesview.h"
#include "gui/reusable/searchlineedit.h"
#include "gui/systemtrayicon.h"
#include "gui/tabbar.h"
#include "gui/toolbars/feedstoolbar.h"
#include "gui/toolbars/messagestoolbar.h"
#include "gui/toolbars/statusbar.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/mutex.h"
#include "miscellaneous/settings.h"
#include "network-web/adblock/adblockicon.h"
#include "network-web/adblock/adblockmanager.h"
#include "network-web/webfactory.h"
#include "services/abstract/recyclebin.h"
#include "services/abstract/serviceroot.h"

#include <QClipboard>
#include <QCloseEvent>
#include <QFileDialog>
#include <QRect>
#include <QScopedPointer>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>

#if QT_VERSION >= 0x050E00 // Qt >= 5.14.0
#include <QScreen>
#else
#include <QDesktopWidget>
#endif

#include <QWindow>

FormMain::FormMain(QWidget* parent, Qt::WindowFlags f)
  : QMainWindow(parent, f), m_ui(new Ui::FormMain), m_trayMenu(nullptr), m_statusBar(nullptr) {
  qDebugNN << LOGSEC_GUI
           << "Creating main application form in thread:" << QUOTE_W_SPACE_DOT(QThread::currentThreadId());
  // setAttribute(Qt::WA_WindowPropagation, true);
  m_ui->setupUi(this);
  qApp->setMainForm(this);

  setWindowIcon(qApp->desktopAwareIcon());
  setWindowTitle(QSL(APP_LONG_NAME));

  QMenu* main_menu = new QMenu(tr("Main menu"), this);

  main_menu->addMenu(m_ui->m_menuFile);
  main_menu->addMenu(m_ui->m_menuView);
  main_menu->addMenu(m_ui->m_menuAccounts);
  main_menu->addMenu(m_ui->m_menuFeeds);
  main_menu->addMenu(m_ui->m_menuMessages);
  main_menu->addMenu(m_ui->m_menuWebBrowserTabs);
  main_menu->addMenu(m_ui->m_menuTools);
  main_menu->addMenu(m_ui->m_menuHelp);

  QToolButton* btn_main_menu = new QToolButton(this);

  btn_main_menu->setToolTip(tr("Open main menu"));
  btn_main_menu->setMenu(main_menu);
  btn_main_menu->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
  btn_main_menu->setIcon(qApp->icons()->fromTheme(QSL("go-home")));

  m_actionToolbarMainMenu = new QWidgetAction(this);
  m_actionToolbarMainMenu->setDefaultWidget(btn_main_menu);
  m_actionToolbarMainMenu->setIcon(qApp->icons()->fromTheme(QSL("go-home")));
  m_actionToolbarMainMenu->setText(tr("Open &main menu"));
  m_actionToolbarMainMenu->setObjectName("m_actionToolbarMainMenu");

  connect(m_actionToolbarMainMenu, &QWidgetAction::triggered, this, [this]() {
    qobject_cast<QToolButton*>(m_actionToolbarMainMenu->defaultWidget())->menu()->exec();
  });

  m_ui->m_menuWebBrowserTabs->addAction(qApp->web()->adBlock()->adBlockIcon());

#if defined(NO_LITE)
  m_ui->m_menuWebBrowserTabs->addAction(qApp->web()->engineSettingsAction());
#endif

  // Add these actions to the list of actions of the main window.
  // This allows to use actions via shortcuts
  // even if main menu is not visible.
  addActions(qApp->userActions());
  setStatusBar(m_statusBar = new StatusBar(this));

  // Prepare main window and tabs.
  prepareMenus();

  // Prepare tabs.
  tabWidget()->feedMessageViewer()->feedsToolBar()->loadSavedActions();
  tabWidget()->feedMessageViewer()->messagesToolBar()->loadSavedActions();

  // Establish connections.
  createConnections();
  updateMessageButtonsAvailability();
  updateFeedButtonsAvailability();
  updateTabsButtonsAvailability(tabWidget()->currentIndex());

  // Setup some appearance of the window.
  setupIcons();
  loadSize();
  m_statusBar->loadSavedActions();
}

FormMain::~FormMain() {
  qDebugNN << LOGSEC_GUI << "Destroying FormMain instance.";
}

QMenu* FormMain::trayMenu() const {
  return m_trayMenu;
}

TabWidget* FormMain::tabWidget() const {
  return m_ui->m_tabWidget;
}

StatusBar* FormMain::statusBar() const {
  return m_statusBar;
}

void FormMain::showDbCleanupAssistant() {
  if (qApp->feedUpdateLock()->tryLock()) {
    FormDatabaseCleanup form(this);

    form.exec();

    // Reload needed stuff.
    qApp->feedUpdateLock()->unlock();
    tabWidget()->feedMessageViewer()->messagesView()->reloadSelections();
    qApp->feedReader()->feedsModel()->informAboutDatabaseCleanup();
    qApp->feedReader()->feedsModel()->reloadCountsOfWholeModel();
  }
  else {
    qApp->showGuiMessage(Notification::Event::GeneralEvent,
                         {tr("Cannot cleanup database"),
                          tr("Cannot cleanup database, because another critical action is running."),
                          QSystemTrayIcon::MessageIcon::Warning});
  }
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

#if !defined(Q_OS_MACOS)
  actions << m_ui->m_actionFullscreen;
  actions << m_ui->m_actionSwitchMainMenu;
#endif

  actions << m_ui->m_actionAboutGuard;
  actions << m_ui->m_actionSwitchFeedsList;
  actions << m_ui->m_actionSwitchMainWindow;

  actions << m_ui->m_actionSwitchToolBars;
  actions << m_ui->m_actionSwitchListHeaders;
  actions << m_ui->m_actionSwitchMessageViewerToolbars;
  actions << m_ui->m_actionSwitchStatusBar;
  actions << m_ui->m_actionSwitchMessageListOrientation;
  actions << m_ui->m_actionTabsNext;
  actions << m_ui->m_actionTabsPrevious;
  actions << m_ui->m_actionSendMessageViaEmail;
  actions << m_ui->m_actionOpenSelectedSourceArticlesExternally;
  actions << m_ui->m_actionOpenSelectedMessagesInternally;
  actions << m_ui->m_actionPlaySelectedArticlesInMediaPlayer;
  actions << m_ui->m_actionOpenSelectedMessagesInternallyNoTab;
  actions << m_ui->m_actionAlternateColorsInLists;
  actions << m_ui->m_actionMessagePreviewEnabled;
  actions << m_ui->m_actionMarkAllItemsRead;
  actions << m_ui->m_actionMarkSelectedItemsAsRead;
  actions << m_ui->m_actionMarkSelectedItemsAsUnread;
  actions << m_ui->m_actionClearSelectedItems;
  actions << m_ui->m_actionClearAllItems;
  actions << m_ui->m_actionShowOnlyUnreadItems;
  actions << m_ui->m_actionSortFeedsAlphabetically;
  actions << m_ui->m_actionShowTreeBranches;
  actions << m_ui->m_actionAutoExpandItemsWhenSelected;
  actions << m_ui->m_actionMarkSelectedMessagesAsRead;
  actions << m_ui->m_actionMarkSelectedMessagesAsUnread;
  actions << m_ui->m_actionSwitchImportanceOfSelectedMessages;
  actions << m_ui->m_actionDeleteSelectedMessages;
  actions << m_ui->m_actionUpdateAllItems;
  actions << m_ui->m_actionUpdateSelectedItems;
  actions << m_ui->m_actionUpdateSelectedItemsWithCustomTimers;
  actions << m_ui->m_actionStopRunningItemsUpdate;
  actions << m_ui->m_actionEditSelectedItem;
  actions << m_ui->m_actionEditChildFeeds;
  actions << m_ui->m_actionEditChildFeedsRecursive;
  actions << m_ui->m_actionCopyUrlSelectedFeed;
  actions << m_ui->m_actionCopyUrlSelectedArticles;
  actions << m_ui->m_actionFocusSearchFeeds;
  actions << m_ui->m_actionFocusSearchArticles;
  actions << m_ui->m_actionDeleteSelectedItem;
  actions << m_ui->m_actionServiceAdd;
  actions << m_ui->m_actionServiceEdit;
  actions << m_ui->m_actionServiceDelete;
  actions << m_ui->m_actionCleanupDatabase;
  actions << m_ui->m_actionAddFeedIntoSelectedItem;
  actions << m_ui->m_actionFeedMoveUp;
  actions << m_ui->m_actionFeedMoveDown;
  actions << m_ui->m_actionFeedMoveTop;
  actions << m_ui->m_actionFeedMoveBottom;
  actions << m_ui->m_actionAddCategoryIntoSelectedItem;
  actions << m_ui->m_actionSelectNextItem;
  actions << m_ui->m_actionSelectPreviousItem;
  actions << m_ui->m_actionSelectNextMessage;
  actions << m_ui->m_actionSelectPreviousMessage;
  actions << m_ui->m_actionSelectNextUnreadMessage;
  actions << m_ui->m_actionExpandCollapseItem;
  actions << m_ui->m_actionExpandCollapseItemRecursively;
  actions << m_ui->m_actionMessageFilters;
  actions << m_ui->m_actionEmptyAllRecycleBins;
  actions << m_ui->m_actionRestoreAllRecycleBins;
  actions << m_ui->m_actionTabNewWebBrowser;
  actions << m_ui->m_actionTabsCloseCurrent;
  actions << m_ui->m_actionTabsCloseAll;
  actions << m_ui->m_actionTabsCloseAllExceptCurrent;
  actions << m_ui->m_actionBrowserScrollUp;
  actions << m_ui->m_actionBrowserScrollDown;
  actions << m_actionToolbarMainMenu;

  return actions;
}

void FormMain::prepareMenus() {
  // Setup menu for tray icon.
  if (SystemTrayIcon::isSystemTrayAreaAvailable()) {
#if defined(Q_OS_WIN)
    m_trayMenu = new TrayIconMenu(QSL(APP_NAME), this);
#else
    m_trayMenu = new QMenu(QSL(APP_NAME), this);
#endif

    // Add needed items to the menu.
    m_trayMenu->addAction(m_ui->m_actionUpdateAllItems);
    m_trayMenu->addAction(m_ui->m_actionMarkAllItemsRead);
    m_trayMenu->addSeparator();

    m_trayMenu->addAction(m_ui->m_actionSwitchMainWindow);
    m_trayMenu->addAction(m_ui->m_actionSettings);
    m_trayMenu->addAction(m_ui->m_actionQuit);

    qDebugNN << LOGSEC_GUI << "Creating tray icon menu.";
  }

#if defined(Q_OS_MACOS)
  m_ui->m_actionSwitchMainMenu->setVisible(false);
  m_ui->m_actionFullscreen->setVisible(false);
#endif

  if (QSysInfo::currentCpuArchitecture().contains(QSL("arm"), Qt::CaseSensitivity::CaseInsensitive)) {
    qWarningNN << LOGSEC_GUI << "Disabling native menu bar.";
    m_ui->m_menuBar->setNativeMenuBar(false);

#if defined(Q_OS_MACOS)
    // This works around a macOS-only Qt crash.
    // QTBUG: https://bugreports.qt.io/browse/QTBUG-102107
    // TODO: Remove this workaround once the upstream bug gets addressed.
    m_ui->m_menuBar->setCornerWidget(nullptr);
#endif
  }
}

void FormMain::switchFullscreenMode() {
  if (!isFullScreen()) {
    qApp->settings()->setValue(GROUP(GUI), GUI::IsMainWindowMaximizedBeforeFullscreen, isMaximized());
    showFullScreen();
  }
  else {
    if (qApp->settings()->value(GROUP(GUI), SETTING(GUI::IsMainWindowMaximizedBeforeFullscreen)).toBool()) {
      setWindowState((windowState() & ~Qt::WindowFullScreen) | Qt::WindowMaximized);
    }
    else {
      showNormal();
    }
  }
}

void FormMain::updateAddItemMenu() {
  // NOTE: Clear here deletes items from memory but only those OWNED by the menu.
  m_ui->m_menuAddItem->clear();

  auto srts = qApp->feedReader()->feedsModel()->serviceRoots();

  for (ServiceRoot* activated_root : std::as_const(srts)) {
    QMenu* root_menu = new QMenu(activated_root->title(), m_ui->m_menuAddItem);

    root_menu->setIcon(activated_root->icon());
    root_menu->setToolTip(activated_root->description());
    QList<QAction*> specific_root_actions = activated_root->addItemMenu();

    if (activated_root->supportsCategoryAdding()) {
      QAction* action_new_category =
        new QAction(qApp->icons()->fromTheme(QSL("folder")), tr("Add new category"), m_ui->m_menuAddItem);

      root_menu->addAction(action_new_category);
      connect(action_new_category, &QAction::triggered, activated_root, [activated_root]() {
        activated_root->addNewCategory(activated_root);
      });
    }

    if (activated_root->supportsFeedAdding()) {
      QAction* action_new_feed =
        new QAction(qApp->icons()->fromTheme(QSL("application-rss+xml")), tr("Add new feed"), m_ui->m_menuAddItem);

      root_menu->addAction(action_new_feed);
      connect(action_new_feed, &QAction::triggered, activated_root, [activated_root]() {
        activated_root->addNewFeed(activated_root, QGuiApplication::clipboard()->text(QClipboard::Mode::Clipboard));
      });
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
    m_ui->m_menuAddItem->addAction(m_ui->m_actionAddCategoryIntoSelectedItem);
    m_ui->m_menuAddItem->addAction(m_ui->m_actionAddFeedIntoSelectedItem);
  }
  else {
    m_ui->m_menuAddItem->addAction(m_ui->m_actionNoAccounts);
  }
}

void FormMain::updateRecycleBinMenu() {
  m_ui->m_menuRecycleBin->clear();

  auto srts = qApp->feedReader()->feedsModel()->serviceRoots();

  for (const ServiceRoot* activated_root : std::as_const(srts)) {
    QMenu* root_menu = new QMenu(activated_root->title(), m_ui->m_menuRecycleBin);

    root_menu->setIcon(activated_root->icon());
    root_menu->setToolTip(activated_root->description());
    RecycleBin* bin = activated_root->recycleBin();
    QList<QAction*> context_menu;

    if (bin == nullptr) {
      QAction* no_action =
        new QAction(qApp->icons()->fromTheme(QSL("dialog-error")), tr("No recycle bin"), m_ui->m_menuRecycleBin);

      no_action->setEnabled(false);
      root_menu->addAction(no_action);
    }
    else if ((context_menu = bin->contextMenuFeedsList()).isEmpty()) {
      QAction* no_action =
        new QAction(qApp->icons()->fromTheme(QSL("dialog-error")), tr("No actions possible"), m_ui->m_menuRecycleBin);

      no_action->setEnabled(false);
      root_menu->addAction(no_action);
    }
    else {
      root_menu->addActions(context_menu);
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

  auto srts = qApp->feedReader()->feedsModel()->serviceRoots();

  for (ServiceRoot* activated_root : srts) {
    QMenu* root_menu = new QMenu(activated_root->title(), m_ui->m_menuAccounts);

    root_menu->setIcon(activated_root->icon());
    root_menu->setToolTip(activated_root->description());
    QList<QAction*> root_actions = activated_root->serviceMenu();

    if (root_actions.isEmpty()) {
      QAction* no_action =
        new QAction(qApp->icons()->fromTheme(QSL("dialog-error")), tr("No possible actions"), m_ui->m_menuAccounts);

      no_action->setEnabled(false);
      root_menu->addAction(no_action);
    }
    else {
      root_menu->addActions(root_actions);
    }

    m_ui->m_menuAccounts->addMenu(root_menu);
  }

  if (!m_ui->m_menuAccounts->actions().isEmpty()) {
    m_ui->m_menuAccounts->addSeparator();
  }

  m_ui->m_menuAccounts->addAction(m_ui->m_actionServiceAdd);
  m_ui->m_menuAccounts->addAction(m_ui->m_actionServiceEdit);
  m_ui->m_menuAccounts->addAction(m_ui->m_actionServiceDelete);
}

void FormMain::updateTabsButtonsAvailability(int index) {
  m_ui->m_actionTabsCloseCurrent->setEnabled(tabWidget()->tabBar()->tabType(index) == TabBar::TabType::Closable ||
                                             tabWidget()->tabBar()->tabType(index) == TabBar::TabType::DownloadManager);
}

void FormMain::onFeedUpdatesFinished(const FeedDownloadResults& results) {
  Q_UNUSED(results)

  statusBar()->clearProgressFeeds();
  tabWidget()->feedMessageViewer()->messagesView()->reloadSelections();
}

void FormMain::onFeedUpdatesStarted() {
  m_ui->m_actionStopRunningItemsUpdate->setEnabled(true);
  statusBar()->showProgressFeeds(-1, tr("Fetching common data"));
}

void FormMain::onFeedUpdatesProgress(const Feed* feed, int current, int total) {
  statusBar()->showProgressFeeds(int((current * 100.0) / total), feed->sanitizedTitle());
}

void FormMain::updateMessageButtonsAvailability() {
  const bool one_message_selected =
    tabWidget()->feedMessageViewer()->messagesView()->selectionModel()->selectedRows().size() == 1;
  const bool atleast_one_message_selected =
    !tabWidget()->feedMessageViewer()->messagesView()->selectionModel()->selectedRows().isEmpty();
  const bool bin_loaded =
    tabWidget()->feedMessageViewer()->messagesView()->sourceModel()->loadedItem() != nullptr &&
    tabWidget()->feedMessageViewer()->messagesView()->sourceModel()->loadedItem()->kind() == RootItem::Kind::Bin;

  m_ui->m_actionDeleteSelectedMessages->setEnabled(atleast_one_message_selected);
  m_ui->m_actionRestoreSelectedMessages->setEnabled(atleast_one_message_selected && bin_loaded);
  m_ui->m_actionMarkSelectedMessagesAsRead->setEnabled(atleast_one_message_selected);
  m_ui->m_actionMarkSelectedMessagesAsUnread->setEnabled(atleast_one_message_selected);
  m_ui->m_actionOpenSelectedMessagesInternallyNoTab->setEnabled(one_message_selected);

#if !defined(ENABLE_MEDIAPLAYER)
  m_ui->m_actionPlaySelectedArticlesInMediaPlayer->setText(tr("Play in media player") + QSL(" ") +
                                                           tr("(not supported)"));
  m_ui->m_actionPlaySelectedArticlesInMediaPlayer->setEnabled(false);
#else
  m_ui->m_actionPlaySelectedArticlesInMediaPlayer->setEnabled(one_message_selected);
#endif

  m_ui->m_actionOpenSelectedMessagesInternally->setEnabled(one_message_selected);
  m_ui->m_actionOpenSelectedSourceArticlesExternally->setEnabled(atleast_one_message_selected);
  m_ui->m_actionCopyUrlSelectedArticles->setEnabled(atleast_one_message_selected);
  m_ui->m_actionSendMessageViaEmail->setEnabled(one_message_selected);
  m_ui->m_actionSwitchImportanceOfSelectedMessages->setEnabled(atleast_one_message_selected);
}

void FormMain::updateFeedButtonsAvailability() {
  const bool is_update_running = qApp->feedReader()->isFeedUpdateRunning();
  const bool critical_action_running = qApp->feedUpdateLock()->isLocked();
  const RootItem* selected_item = tabWidget()->feedMessageViewer()->feedsView()->selectedItem();
  const bool anything_selected = selected_item != nullptr;
  const bool feed_selected = anything_selected && selected_item->kind() == RootItem::Kind::Feed;
  const bool category_selected = anything_selected && selected_item->kind() == RootItem::Kind::Category;
  const bool service_selected = anything_selected && selected_item->kind() == RootItem::Kind::ServiceRoot;
  const bool manual_feed_sort = !m_ui->m_actionSortFeedsAlphabetically->isChecked();

  m_ui->m_actionRearrangeFeeds->setEnabled(manual_feed_sort && (service_selected || category_selected));
  m_ui->m_actionRearrangeCategories->setEnabled(manual_feed_sort && (service_selected || category_selected));
  m_ui->m_actionStopRunningItemsUpdate->setEnabled(is_update_running);
  m_ui->m_actionBackupDatabaseSettings->setEnabled(!critical_action_running);
  m_ui->m_actionCleanupDatabase->setEnabled(!critical_action_running);
  m_ui->m_actionClearSelectedItems->setEnabled(anything_selected);
  m_ui->m_actionDeleteSelectedItem->setEnabled(!critical_action_running && anything_selected);
  m_ui->m_actionEditSelectedItem->setEnabled(!critical_action_running && anything_selected);
  m_ui->m_actionEditChildFeeds->setEnabled(!critical_action_running && (service_selected || category_selected));
  m_ui->m_actionEditChildFeedsRecursive->setEnabled(!critical_action_running &&
                                                    (service_selected || category_selected));
  m_ui->m_actionCopyUrlSelectedFeed->setEnabled(service_selected || feed_selected || category_selected);
  m_ui->m_actionMarkSelectedItemsAsRead->setEnabled(anything_selected);
  m_ui->m_actionMarkSelectedItemsAsUnread->setEnabled(anything_selected);
  m_ui->m_actionUpdateAllItems->setEnabled(!critical_action_running);
  m_ui->m_actionUpdateSelectedItemsWithCustomTimers->setEnabled(!critical_action_running);
  m_ui->m_actionUpdateSelectedItems->setEnabled(!critical_action_running &&
                                                (feed_selected || category_selected || service_selected));
  m_ui->m_actionExpandCollapseItem->setEnabled(category_selected || service_selected);
  m_ui->m_actionExpandCollapseItemRecursively->setEnabled(category_selected || service_selected);
  m_ui->m_actionServiceDelete->setEnabled(service_selected);
  m_ui->m_actionServiceEdit->setEnabled(service_selected);
  m_ui->m_actionAddFeedIntoSelectedItem->setEnabled(anything_selected);
  m_ui->m_actionAddCategoryIntoSelectedItem->setEnabled(anything_selected);
  m_ui->m_menuAddItem->setEnabled(!critical_action_running);
  m_ui->m_menuAccounts->setEnabled(!critical_action_running);
  m_ui->m_menuRecycleBin->setEnabled(!critical_action_running);

  m_ui->m_actionFeedMoveUp->setEnabled(manual_feed_sort && (feed_selected || category_selected || service_selected));
  m_ui->m_actionFeedMoveDown->setEnabled(manual_feed_sort && (feed_selected || category_selected || service_selected));
  m_ui->m_actionFeedMoveTop->setEnabled(manual_feed_sort && (feed_selected || category_selected || service_selected));
  m_ui->m_actionFeedMoveBottom->setEnabled(manual_feed_sort &&
                                           (feed_selected || category_selected || service_selected));
}

void FormMain::switchVisibility(bool force_hide) {
  if (force_hide || (isVisible() && !isMinimized())) {
    if (SystemTrayIcon::isSystemTrayDesired() && SystemTrayIcon::isSystemTrayAreaAvailable()) {

      if (QApplication::activeModalWidget() != nullptr) {
        qApp->showGuiMessage(Notification::Event::GeneralEvent,
                             {tr("Close dialogs"),
                              tr("Close opened modal dialogs first."),
                              QSystemTrayIcon::MessageIcon::Warning});
      }
      else {
        hide();
      }
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
  setWindowState((windowState() & ~Qt::WindowState::WindowMinimized) | Qt::WindowState::WindowActive);

  // Display the window and make sure it is raised on top.
  show();
  raise();
  activateWindow();

  // Raise alert event. Check the documentation for more info on this.
  QApplication::alert(this);
}

void FormMain::setupIcons() {
  IconFactory* icon_theme_factory = qApp->icons();

  // Setup icons of this main window.
  m_ui->m_actionDownloadManager->setIcon(icon_theme_factory->fromTheme(QSL("emblem-downloads"), QSL("download")));
  m_ui->m_actionSettings->setIcon(icon_theme_factory->fromTheme(QSL("emblem-system"), QSL("applications-system")));
  m_ui->m_actionQuit->setIcon(icon_theme_factory->fromTheme(QSL("application-exit")));
  m_ui->m_actionRestart->setIcon(icon_theme_factory->fromTheme(QSL("view-refresh")));
  m_ui->m_actionAboutGuard->setIcon(icon_theme_factory->fromTheme(QSL("help-about")));
  m_ui->m_actionCheckForUpdates->setIcon(icon_theme_factory->fromTheme(QSL("system-upgrade")));
  m_ui->m_actionCleanupDatabase->setIcon(icon_theme_factory->fromTheme(QSL("edit-clear")));
  m_ui->m_actionReportBug->setIcon(icon_theme_factory->fromTheme(QSL("call-start")));
  m_ui->m_actionBackupDatabaseSettings->setIcon(icon_theme_factory->fromTheme(QSL("document-export")));
  m_ui->m_actionRestoreDatabaseSettings->setIcon(icon_theme_factory->fromTheme(QSL("document-import")));
  m_ui->m_actionDonate->setIcon(icon_theme_factory->fromTheme(QSL("applications-office")));
  m_ui->m_actionApplicationLog->setIcon(icon_theme_factory->fromTheme(QSL("dialog-information")));
  m_ui->m_actionDisplayDocs->setIcon(icon_theme_factory->fromTheme(QSL("applications-science")));

  // View.
  m_ui->m_actionSwitchMainWindow->setIcon(icon_theme_factory->fromTheme(QSL("window-close")));
  m_ui->m_actionFullscreen->setIcon(icon_theme_factory->fromTheme(QSL("view-fullscreen")));
  m_ui->m_actionSwitchFeedsList->setIcon(icon_theme_factory->fromTheme(QSL("view-restore")));
  m_ui->m_actionSwitchMainMenu->setIcon(icon_theme_factory->fromTheme(QSL("view-restore")));
  m_ui->m_actionSwitchToolBars->setIcon(icon_theme_factory->fromTheme(QSL("view-restore")));
  m_ui->m_actionSwitchListHeaders->setIcon(icon_theme_factory->fromTheme(QSL("view-restore")));
  m_ui->m_actionSwitchMessageViewerToolbars->setIcon(icon_theme_factory->fromTheme(QSL("view-restore")));
  m_ui->m_actionSwitchStatusBar->setIcon(icon_theme_factory->fromTheme(QSL("dialog-information")));
  m_ui->m_actionSwitchMessageListOrientation->setIcon(icon_theme_factory->fromTheme(QSL("view-restore")));
  m_ui->m_menuShowHide->setIcon(icon_theme_factory->fromTheme(QSL("view-restore")));

  // Feeds/messages.
  m_ui->m_menuAddItem->setIcon(icon_theme_factory->fromTheme(QSL("list-add")));
  m_ui->m_actionStopRunningItemsUpdate->setIcon(icon_theme_factory->fromTheme(QSL("process-stop")));
  m_ui->m_actionUpdateAllItems->setIcon(icon_theme_factory->fromTheme(QSL("download"), QSL("browser-download")));
  m_ui->m_actionUpdateSelectedItems->setIcon(icon_theme_factory->fromTheme(QSL("download"), QSL("browser-download")));
  m_ui->m_actionUpdateSelectedItemsWithCustomTimers->setIcon(icon_theme_factory->fromTheme(QSL("download"),
                                                                                           QSL("browser-download")));
  m_ui->m_actionClearSelectedItems->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-junk")));
  m_ui->m_actionClearAllItems->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-junk")));
  m_ui->m_actionDeleteSelectedItem->setIcon(icon_theme_factory->fromTheme(QSL("list-remove")));
  m_ui->m_actionDeleteSelectedMessages->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-junk")));
  m_ui->m_actionEditSelectedItem->setIcon(icon_theme_factory->fromTheme(QSL("document-edit")));
  m_ui->m_actionEditChildFeeds->setIcon(icon_theme_factory->fromTheme(QSL("document-edit")));
  m_ui->m_actionEditChildFeedsRecursive->setIcon(icon_theme_factory->fromTheme(QSL("document-edit")));
  m_ui->m_actionCopyUrlSelectedFeed->setIcon(icon_theme_factory->fromTheme(QSL("edit-copy")));
  m_ui->m_actionCopyUrlSelectedArticles->setIcon(icon_theme_factory->fromTheme(QSL("edit-copy")));
  m_ui->m_actionMarkAllItemsRead->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-read")));
  m_ui->m_actionMarkSelectedItemsAsRead->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-read")));
  m_ui->m_actionMarkSelectedItemsAsUnread->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-unread")));
  m_ui->m_actionMarkSelectedMessagesAsRead->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-read")));
  m_ui->m_actionMarkSelectedMessagesAsUnread->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-unread")));
  m_ui->m_actionSwitchImportanceOfSelectedMessages->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-important")));
  m_ui->m_actionOpenSelectedSourceArticlesExternally->setIcon(icon_theme_factory->fromTheme(QSL("document-open")));
  m_ui->m_actionOpenSelectedMessagesInternally->setIcon(icon_theme_factory->fromTheme(QSL("document-open")));
  m_ui->m_actionPlaySelectedArticlesInMediaPlayer->setIcon(icon_theme_factory->fromTheme(QSL("player_play"),
                                                                                         QSL("media-playback-start")));
  m_ui->m_actionOpenSelectedMessagesInternallyNoTab->setIcon(icon_theme_factory->fromTheme(QSL("document-open")));
  m_ui->m_actionSendMessageViaEmail->setIcon(icon_theme_factory->fromTheme(QSL("mail-send")));
  m_ui->m_actionSelectNextItem->setIcon(icon_theme_factory->fromTheme(QSL("arrow-down")));
  m_ui->m_actionSelectPreviousItem->setIcon(icon_theme_factory->fromTheme(QSL("arrow-up")));
  m_ui->m_actionSelectNextMessage->setIcon(icon_theme_factory->fromTheme(QSL("arrow-down")));
  m_ui->m_actionSelectPreviousMessage->setIcon(icon_theme_factory->fromTheme(QSL("arrow-up")));
  m_ui->m_actionSelectNextUnreadMessage->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-unread")));
  m_ui->m_actionSortFeedsAlphabetically->setIcon(icon_theme_factory->fromTheme(QSL("format-text-bold")));
  m_ui->m_actionShowOnlyUnreadItems->setIcon(icon_theme_factory->fromTheme(QSL("mail-mark-unread")));
  m_ui->m_actionExpandCollapseItem->setIcon(icon_theme_factory->fromTheme(QSL("format-indent-more")));
  m_ui->m_actionExpandCollapseItemRecursively->setIcon(icon_theme_factory->fromTheme(QSL("format-indent-more")));
  m_ui->m_actionRestoreSelectedMessages->setIcon(icon_theme_factory->fromTheme(QSL("view-refresh")));
  m_ui->m_actionRestoreAllRecycleBins->setIcon(icon_theme_factory->fromTheme(QSL("view-refresh")));
  m_ui->m_actionEmptyAllRecycleBins->setIcon(icon_theme_factory->fromTheme(QSL("edit-clear")));
  m_ui->m_actionServiceAdd->setIcon(icon_theme_factory->fromTheme(QSL("list-add")));
  m_ui->m_actionServiceEdit->setIcon(icon_theme_factory->fromTheme(QSL("document-edit")));
  m_ui->m_actionServiceDelete->setIcon(icon_theme_factory->fromTheme(QSL("list-remove")));
  m_ui->m_actionAddFeedIntoSelectedItem->setIcon(icon_theme_factory->fromTheme(QSL("application-rss+xml")));
  m_ui->m_actionAddCategoryIntoSelectedItem->setIcon(icon_theme_factory->fromTheme(QSL("folder")));
  m_ui->m_actionMessageFilters->setIcon(icon_theme_factory->fromTheme(QSL("view-list-details")));

  m_ui->m_actionFeedMoveUp->setIcon(icon_theme_factory->fromTheme(QSL("arrow-up"), QSL("go-up")));
  m_ui->m_actionFeedMoveDown->setIcon(icon_theme_factory->fromTheme(QSL("arrow-down"), QSL("go-down")));
  m_ui->m_actionFeedMoveTop->setIcon(icon_theme_factory->fromTheme(QSL("arrow-up-double"), QSL("go-up")));
  m_ui->m_actionFeedMoveBottom->setIcon(icon_theme_factory->fromTheme(QSL("arrow-down-double"), QSL("go-down")));

  // Tabs & web browser.
  m_ui->m_actionTabNewWebBrowser->setIcon(icon_theme_factory->fromTheme(QSL("tab-new")));
  m_ui->m_actionTabsCloseAll->setIcon(icon_theme_factory->fromTheme(QSL("window-close")));
  m_ui->m_actionTabsCloseCurrent->setIcon(icon_theme_factory->fromTheme(QSL("window-close")));
  m_ui->m_actionTabsCloseAllExceptCurrent->setIcon(icon_theme_factory->fromTheme(QSL("window-close")));
  m_ui->m_actionTabsNext->setIcon(icon_theme_factory->fromTheme(QSL("go-next")));
  m_ui->m_actionTabsPrevious->setIcon(icon_theme_factory->fromTheme(QSL("go-previous")));
  m_ui->m_actionBrowserScrollUp->setIcon(icon_theme_factory->fromTheme(QSL("arrow-up")));
  m_ui->m_actionBrowserScrollDown->setIcon(icon_theme_factory->fromTheme(QSL("arrow-down")));
  m_ui->m_actionCleanupWebCache->setIcon(icon_theme_factory->fromTheme(QSL("edit-clear")));

  // Setup icons on TabWidget too.
  m_ui->m_tabWidget->setupIcons();
}

void FormMain::loadSize() {
#if QT_VERSION >= 0x050E00 // Qt >= 5.14.0
  QScreen* scr = screen();

  if (scr == nullptr) {
    qWarningNN << LOGSEC_MESSAGEMODEL << "Cannot load dialog size, because no screens are detected.";
    return;
  }

  const QRect screen = scr->geometry();
#else
  const QRect screen = qApp->desktop()->screenGeometry();
#endif

  const Settings* settings = qApp->settings();

  // Reload main window size & position.
  resize(settings->value(GROUP(GUI), GUI::MainWindowInitialSize, size()).toSize());
  move(settings->value(GROUP(GUI), GUI::MainWindowInitialPosition, screen.center() - rect().center()).toPoint());

  GuiUtilities::fixTooBigDialog(*this, false);

  if (settings->value(GROUP(GUI), SETTING(GUI::MainWindowStartsMaximized)).toBool()) {
    setWindowState(windowState() | Qt::WindowState::WindowMaximized);

    // We process events so that window is really maximized fast.
    QCoreApplication::processEvents();
  }

  m_ui->m_actionMessagePreviewEnabled
    ->setChecked(settings->value(GROUP(Messages), SETTING(Messages::EnableMessagePreview)).toBool());

  // If user exited the application while in fullsreen mode,
  // then re-enable it now.
  if (settings->value(GROUP(GUI), SETTING(GUI::MainWindowStartsFullscreen)).toBool()) {
    m_ui->m_actionFullscreen->setChecked(true);
  }

  // Hide the main menu if user wants it.
  m_ui->m_actionSwitchMainMenu->setChecked(settings->value(GROUP(GUI), SETTING(GUI::MainMenuVisible)).toBool());

  // Adjust dimensions of "feeds & messages" widget.
  m_ui->m_tabWidget->feedMessageViewer()->loadSize();
  m_ui->m_actionSwitchToolBars->setChecked(settings->value(GROUP(GUI), SETTING(GUI::ToolbarsVisible)).toBool());
  m_ui->m_actionSwitchListHeaders->setChecked(settings->value(GROUP(GUI), SETTING(GUI::ListHeadersVisible)).toBool());
  m_ui->m_actionSwitchMessageViewerToolbars
    ->setChecked(settings->value(GROUP(GUI), SETTING(GUI::MessageViewerToolbarsVisible)).toBool());
  m_ui->m_actionSwitchStatusBar->setChecked(settings->value(GROUP(GUI), SETTING(GUI::StatusBarVisible)).toBool());

  // Other startup GUI-related settings.
  m_ui->m_actionSortFeedsAlphabetically
    ->setChecked(settings->value(GROUP(Feeds), SETTING(Feeds::SortAlphabetically)).toBool());
  m_ui->m_actionShowOnlyUnreadItems
    ->setChecked(settings->value(GROUP(Feeds), SETTING(Feeds::ShowOnlyUnreadFeeds)).toBool());
  m_ui->m_actionShowTreeBranches->setChecked(settings->value(GROUP(Feeds), SETTING(Feeds::ShowTreeBranches)).toBool());
  m_ui->m_actionAutoExpandItemsWhenSelected
    ->setChecked(settings->value(GROUP(Feeds), SETTING(Feeds::AutoExpandOnSelection)).toBool());
  m_ui->m_actionAlternateColorsInLists
    ->setChecked(settings->value(GROUP(GUI), SETTING(GUI::AlternateRowColorsInLists)).toBool());
}

void FormMain::saveSize() {
  Settings* settings = qApp->settings();
  bool is_fullscreen = isFullScreen();
  bool is_maximized = isMaximized();
  QPoint window_pos = frameGeometry().topLeft();
  QSize window_size = normalGeometry().size();

  if (!window_size.isValid()) {
    window_size = sizeHint();
  }

  settings->setValue(GROUP(GUI), GUI::MainMenuVisible, m_ui->m_actionSwitchMainMenu->isChecked());
  settings->setValue(GROUP(GUI), GUI::StatusBarVisible, m_ui->m_actionSwitchStatusBar->isChecked());

  settings->setValue(GROUP(GUI), GUI::MainWindowInitialPosition, window_pos);
  settings->setValue(GROUP(GUI), GUI::MainWindowInitialSize, window_size);
  settings->setValue(GROUP(GUI), GUI::MainWindowStartsMaximized, is_maximized);
  settings->setValue(GROUP(GUI), GUI::MainWindowStartsFullscreen, is_fullscreen);

  m_ui->m_tabWidget->feedMessageViewer()->saveSize();
}

void FormMain::createConnections() {
  // Status bar connections.
  connect(m_ui->m_menuAddItem, &QMenu::aboutToShow, this, &FormMain::updateAddItemMenu);
  connect(m_ui->m_menuRecycleBin, &QMenu::aboutToShow, this, &FormMain::updateRecycleBinMenu);
  connect(m_ui->m_menuAccounts, &QMenu::aboutToShow, this, &FormMain::updateAccountsMenu);
  connect(m_ui->m_actionServiceDelete, &QAction::triggered, m_ui->m_actionDeleteSelectedItem, &QAction::triggered);
  connect(m_ui->m_actionServiceEdit, &QAction::triggered, m_ui->m_actionEditSelectedItem, &QAction::triggered);

  // Menu "File" connections.
  connect(m_ui->m_actionBackupDatabaseSettings, &QAction::triggered, this, &FormMain::backupDatabaseSettings);
  connect(m_ui->m_actionRestoreDatabaseSettings, &QAction::triggered, this, &FormMain::restoreDatabaseSettings);
  connect(m_ui->m_actionQuit, &QAction::triggered, qApp, &Application::quit);
  connect(m_ui->m_actionServiceAdd, &QAction::triggered, this, &FormMain::showAddAccountDialog);
  connect(m_ui->m_actionRestart, &QAction::triggered, qApp, &Application::restart);

  // Menu "View" connections.
  connect(m_ui->m_actionSwitchFeedsList,
          &QAction::toggled,
          tabWidget()->feedMessageViewer(),
          &FeedMessageViewer::switchFeedComponentVisibility);
  connect(m_ui->m_actionFullscreen, &QAction::toggled, this, &FormMain::switchFullscreenMode);
  connect(m_ui->m_actionSwitchMainMenu, &QAction::toggled, m_ui->m_menuBar, &QMenuBar::setVisible);
  connect(m_ui->m_actionSwitchMainWindow, &QAction::triggered, this, &FormMain::switchVisibility);
  connect(m_ui->m_actionSwitchStatusBar, &QAction::toggled, statusBar(), &StatusBar::setVisible);
  connect(m_ui->m_actionMessagePreviewEnabled, &QAction::toggled, this, [](bool enabled) {
    qApp->settings()->setValue(GROUP(Messages), Messages::EnableMessagePreview, enabled);
  });

  // Menu "Tools" connections.
  connect(m_ui->m_actionSettings, &QAction::triggered, this, [this]() {
    FormSettings(*this).exec();
  });
  connect(m_ui->m_actionDownloadManager, &QAction::triggered, m_ui->m_tabWidget, &TabWidget::showDownloadManager);
  connect(m_ui->m_actionCleanupDatabase, &QAction::triggered, this, &FormMain::showDbCleanupAssistant);

#if defined(NO_LITE)
  connect(m_ui->m_actionCleanupWebCache, &QAction::triggered, qApp->web(), &WebFactory::cleanupCache);
#else
  m_ui->m_menuTools->removeAction(m_ui->m_actionCleanupWebCache);
#endif

  // Menu "Help" connections.
  connect(m_ui->m_actionAboutGuard, &QAction::triggered, this, [this]() {
    FormAbout(false, this).exec();
  });
  connect(m_ui->m_actionCheckForUpdates, &QAction::triggered, this, [this]() {
    FormUpdate(this).exec();
  });
  connect(m_ui->m_actionReportBug, &QAction::triggered, this, &FormMain::reportABug);
  connect(m_ui->m_actionDonate, &QAction::triggered, this, &FormMain::donate);
  connect(m_ui->m_actionApplicationLog, &QAction::triggered, qApp, &Application::displayLog);
  connect(m_ui->m_actionDisplayDocs, &QAction::triggered, this, &FormMain::showDocs);

  // Tab widget connections.
  connect(m_ui->m_actionTabsNext, &QAction::triggered, m_ui->m_tabWidget, &TabWidget::gotoNextTab);
  connect(m_ui->m_actionTabsPrevious, &QAction::triggered, m_ui->m_tabWidget, &TabWidget::gotoPreviousTab);
  connect(m_ui->m_actionTabsCloseAllExceptCurrent,
          &QAction::triggered,
          m_ui->m_tabWidget,
          &TabWidget::closeAllTabsExceptCurrent);
  connect(m_ui->m_actionTabsCloseAll, &QAction::triggered, m_ui->m_tabWidget, &TabWidget::closeAllTabs);
  connect(m_ui->m_actionTabsCloseCurrent, &QAction::triggered, m_ui->m_tabWidget, &TabWidget::closeCurrentTab);
  connect(m_ui->m_actionTabNewWebBrowser, &QAction::triggered, m_ui->m_tabWidget, &TabWidget::addEmptyBrowser);
  connect(tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::itemSelected,
          this,
          &FormMain::updateFeedButtonsAvailability);
  connect(qApp->feedUpdateLock(), &Mutex::locked, this, &FormMain::updateFeedButtonsAvailability);
  connect(qApp->feedUpdateLock(), &Mutex::unlocked, this, &FormMain::updateFeedButtonsAvailability);
  connect(tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::currentMessageRemoved,
          this,
          &FormMain::updateMessageButtonsAvailability);
  connect(tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::currentMessageChanged,
          this,
          &FormMain::updateMessageButtonsAvailability);
  connect(tabWidget(), &TabWidget::currentChanged, this, &FormMain::updateTabsButtonsAvailability);
  connect(qApp->feedReader(), &FeedReader::feedUpdatesStarted, this, &FormMain::onFeedUpdatesStarted);
  connect(qApp->feedReader(), &FeedReader::feedUpdatesProgress, this, &FormMain::onFeedUpdatesProgress);
  connect(qApp->feedReader(), &FeedReader::feedUpdatesFinished, this, &FormMain::onFeedUpdatesFinished);

  connect(m_ui->m_actionBrowserScrollUp, &QAction::triggered, tabWidget(), &TabWidget::scrollUpCurrentBrowser);
  connect(m_ui->m_actionBrowserScrollDown, &QAction::triggered, tabWidget(), &TabWidget::scrollDownCurrentBrowser);

  // Toolbar forwardings.
  connect(m_ui->m_actionFocusSearchFeeds,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsToolBar()->searchBox(),
          [this]() {
            tabWidget()->feedMessageViewer()->feedsToolBar()->searchBox()->setFocus();
          });
  connect(m_ui->m_actionFocusSearchArticles,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesToolBar()->searchBox(),
          [this]() {
            tabWidget()->feedMessageViewer()->messagesToolBar()->searchBox()->setFocus();
          });
  connect(m_ui->m_actionAddFeedIntoSelectedItem,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::addFeedIntoSelectedAccount);
  connect(m_ui->m_actionAddCategoryIntoSelectedItem,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::addCategoryIntoSelectedAccount);
  connect(m_ui->m_actionSwitchImportanceOfSelectedMessages,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::switchSelectedMessagesImportance);
  connect(m_ui->m_actionDeleteSelectedMessages,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::deleteSelectedMessages);
  connect(m_ui->m_actionMarkSelectedMessagesAsRead,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::markSelectedMessagesRead);
  connect(m_ui->m_actionMarkSelectedMessagesAsUnread,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::markSelectedMessagesUnread);
  connect(m_ui->m_actionOpenSelectedSourceArticlesExternally,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::openSelectedSourceMessagesExternally);

#if defined(ENABLE_MEDIAPLAYER)
  connect(m_ui->m_actionPlaySelectedArticlesInMediaPlayer,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::playSelectedArticleInMediaPlayer);
#endif

  connect(m_ui->m_actionOpenSelectedMessagesInternally,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::openSelectedMessagesInternally);

  connect(m_ui->m_actionOpenSelectedMessagesInternallyNoTab,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::openSelectedMessageUrl);

  connect(m_ui->m_actionSendMessageViaEmail,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::sendSelectedMessageViaEmail);
  connect(m_ui->m_actionMarkAllItemsRead,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::markAllItemsRead);
  connect(m_ui->m_actionMarkSelectedItemsAsRead,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::markSelectedItemRead);
  connect(m_ui->m_actionExpandCollapseItem,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          [this]() {
            tabWidget()->feedMessageViewer()->feedsView()->expandCollapseCurrentItem(false);
          });
  connect(m_ui->m_actionExpandCollapseItemRecursively,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          [this]() {
            tabWidget()->feedMessageViewer()->feedsView()->expandCollapseCurrentItem(true);
          });
  connect(m_ui->m_actionMarkSelectedItemsAsUnread,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::markSelectedItemUnread);
  connect(m_ui->m_actionClearSelectedItems,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::clearSelectedItems);
  connect(m_ui->m_actionClearAllItems,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::clearAllItems);
  connect(m_ui->m_actionUpdateSelectedItems,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::updateSelectedItems);
  connect(m_ui->m_actionRearrangeCategories,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::rearrangeCategoriesOfSelectedItem);
  connect(m_ui->m_actionRearrangeFeeds,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::rearrangeFeedsOfSelectedItem);

  connect(m_ui->m_actionUpdateAllItems, &QAction::triggered, qApp->feedReader(), &FeedReader::updateAllFeeds);
  connect(m_ui->m_actionUpdateSelectedItemsWithCustomTimers,
          &QAction::triggered,
          qApp->feedReader(),
          &FeedReader::updateManuallyIntervaledFeeds);
  connect(m_ui->m_actionStopRunningItemsUpdate,
          &QAction::triggered,
          qApp->feedReader(),
          &FeedReader::stopRunningFeedUpdate);
  connect(m_ui->m_actionCopyUrlSelectedFeed,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::copyUrlOfSelectedFeeds);
  connect(m_ui->m_actionCopyUrlSelectedArticles,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::copyUrlOfSelectedArticles);
  connect(m_ui->m_actionEditSelectedItem,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::editSelectedItems);
  connect(m_ui->m_actionEditChildFeeds,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::editChildFeeds);
  connect(m_ui->m_actionEditChildFeedsRecursive,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::editRecursiveFeeds);
  connect(m_ui->m_actionDeleteSelectedItem,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::deleteSelectedItem);
  connect(m_ui->m_actionSelectNextItem,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::selectNextItem);
  connect(m_ui->m_actionSwitchToolBars,
          &QAction::toggled,
          tabWidget()->feedMessageViewer(),
          &FeedMessageViewer::setToolBarsEnabled);
  connect(m_ui->m_actionSwitchListHeaders,
          &QAction::toggled,
          tabWidget()->feedMessageViewer(),
          &FeedMessageViewer::setListHeadersEnabled);
  connect(m_ui->m_actionSwitchMessageViewerToolbars,
          &QAction::toggled,
          tabWidget()->feedMessageViewer()->messagesBrowser(),
          &MessagePreviewer::setToolbarsVisible);
  connect(m_ui->m_actionSelectPreviousItem,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::selectPreviousItem);
  connect(m_ui->m_actionSelectNextMessage,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::selectNextItem);
  connect(m_ui->m_actionSelectNextUnreadMessage,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::selectNextUnreadItem);
  connect(m_ui->m_actionSelectPreviousMessage,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::selectPreviousItem);
  connect(m_ui->m_actionSwitchMessageListOrientation,
          &QAction::triggered,
          tabWidget()->feedMessageViewer(),
          &FeedMessageViewer::switchMessageSplitterOrientation);
  connect(m_ui->m_actionShowOnlyUnreadItems,
          &QAction::toggled,
          tabWidget()->feedMessageViewer(),
          &FeedMessageViewer::toggleShowOnlyUnreadFeeds);
  connect(m_ui->m_actionSortFeedsAlphabetically,
          &QAction::toggled,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::toggleFeedSortingMode);
  connect(m_ui->m_actionShowTreeBranches,
          &QAction::toggled,
          tabWidget()->feedMessageViewer(),
          &FeedMessageViewer::toggleShowFeedTreeBranches);
  connect(m_ui->m_actionAutoExpandItemsWhenSelected,
          &QAction::toggled,
          tabWidget()->feedMessageViewer(),
          &FeedMessageViewer::toggleItemsAutoExpandingOnSelection);
  connect(m_ui->m_actionAlternateColorsInLists,
          &QAction::toggled,
          tabWidget()->feedMessageViewer(),
          &FeedMessageViewer::alternateRowColorsInLists);
  connect(m_ui->m_actionRestoreSelectedMessages,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->messagesView(),
          &MessagesView::restoreSelectedMessages);
  connect(m_ui->m_actionRestoreAllRecycleBins,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView()->sourceModel(),
          &FeedsModel::restoreAllBins);
  connect(m_ui->m_actionEmptyAllRecycleBins,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView()->sourceModel(),
          &FeedsModel::emptyAllBins);
  connect(m_ui->m_actionMessageFilters, &QAction::triggered, this, [this]() {
    qApp->feedReader()->showMessageFiltersManager();
    tabWidget()->feedMessageViewer()->messagesView()->reloadSelections();
  });
  connect(m_ui->m_actionFeedMoveUp,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::moveSelectedItemUp);
  connect(m_ui->m_actionFeedMoveDown,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::moveSelectedItemDown);
  connect(m_ui->m_actionFeedMoveTop,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::moveSelectedItemTop);
  connect(m_ui->m_actionFeedMoveBottom,
          &QAction::triggered,
          tabWidget()->feedMessageViewer()->feedsView(),
          &FeedsView::moveSelectedItemBottom);
}

void FormMain::backupDatabaseSettings() {
  QScopedPointer<FormBackupDatabaseSettings> form(new FormBackupDatabaseSettings(this));

  form->exec();
}

void FormMain::restoreDatabaseSettings() {
  FormRestoreDatabaseSettings form(*this);

  form.exec();

  if (form.shouldRestart()) {
    qApp->restart();
  }
}

void FormMain::changeEvent(QEvent* event) {
  switch (event->type()) {
    case QEvent::Type::WindowStateChange: {
      if (Globals::hasFlag(windowState(), Qt::WindowState::WindowMinimized) && SystemTrayIcon::isSystemTrayDesired() &&
          SystemTrayIcon::isSystemTrayAreaAvailable() &&
          qApp->settings()->value(GROUP(GUI), SETTING(GUI::HideMainWindowWhenMinimized)).toBool()) {
        event->ignore();

        QTimer::singleShot(CHANGE_EVENT_DELAY, this, [this]() {
          switchVisibility(true);
        });
      }

      break;
    }

    default:
      break;
  }

  QMainWindow::changeEvent(event);
}

void FormMain::closeEvent(QCloseEvent* event) {
  if (qApp->quitOnLastWindowClosed()) {
    QMainWindow::closeEvent(event);
  }
  else /*if (!event->spontaneous())*/ {
    event->ignore();
    hide();
  }

  qDebugNN << LOGSEC_GUI << "Main window close event";
}

void FormMain::showEvent(QShowEvent* event) {
  QMainWindow::showEvent(event);

  tabWidget()->feedMessageViewer()->normalizeToolbarHeights();
}

void FormMain::hideEvent(QHideEvent* event) {
  QMainWindow::hideEvent(event);

  qDebugNN << LOGSEC_GUI << "Main window hide event";
}

void FormMain::showDocs() {
  qApp->web()->openUrlInExternalBrowser(QSL(APP_URL_DOCUMENTATION));
}

void FormMain::showAddAccountDialog() {
  QScopedPointer<FormAddAccount> form_update(new FormAddAccount(qApp->feedReader()->feedServices(),
                                                                qApp->feedReader()->feedsModel(),
                                                                this));

  form_update->exec();
}

void FormMain::reportABug() {
  qApp->web()->openUrlInExternalBrowser(QSL(APP_URL_ISSUES_NEW));
}

void FormMain::donate() {
  qApp->web()->openUrlInExternalBrowser(QSL(APP_DONATE_URL));
}

void FormMain::resizeEvent(QResizeEvent* event) {
  QMainWindow::resizeEvent(event);

  emit windowResized(event->size());
}
