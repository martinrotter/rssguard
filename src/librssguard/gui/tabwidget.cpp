// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/tabwidget.h"

#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/feedsview.h"
#include "gui/messagepreviewer.h"
#include "gui/messagesview.h"
#include "gui/reusable/labelsmenu.h"
#include "gui/reusable/plaintoolbutton.h"
#include "gui/tabbar.h"
#include "gui/webbrowser.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "network-web/webfactory.h"

#if defined(ENABLE_MEDIAPLAYER)
#include "gui/mediaplayer/mediaplayer.h"
#endif

#include <QMenu>
#include <QTimer>
#include <QToolButton>

TabWidget::TabWidget(QWidget* parent) : QTabWidget(parent), m_menuMain(nullptr) {
  setTabBar(new TabBar(this));
  setupMainMenuButton();
  initializeTabs();
  createConnections();
  updateAppearance();
}

TabWidget::~TabWidget() {
  qDebugNN << LOGSEC_GUI << "Destroying TabWidget instance.";
}

void TabWidget::setupMainMenuButton() {
  m_btnMainMenu = new PlainToolButton(this);
  m_btnMainMenu->setAutoRaise(true);
  m_btnMainMenu->setPadding(3);
  m_btnMainMenu->setToolTip(tr("Displays main menu."));
  m_btnMainMenu->setIcon(qApp->icons()->fromTheme(QSL("go-home")));
  m_btnMainMenu->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);

  setCornerWidget(m_btnMainMenu, Qt::Corner::TopLeftCorner);

  connect(m_btnMainMenu, &PlainToolButton::clicked, this, &TabWidget::openMainMenu);
}

void TabWidget::openMainMenu() {
  if (m_menuMain == nullptr) {
    m_menuMain = new QMenu(tr("Main menu"), this);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuFile);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuView);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuAccounts);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuFeeds);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuMessages);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuTabs);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuTools);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuHelp);
  }

  QPoint button_position = m_btnMainMenu->pos();
  const QSize target_size = m_btnMainMenu->size() / 2.0;

  button_position.setX(button_position.x() + target_size.width());
  button_position.setY(button_position.y() + target_size.height());
  m_menuMain->exec(mapToGlobal(button_position));
}

void TabWidget::checkCornerButtonVisibility() {
  const bool should_be_visible = count() > 1 || !tabBarAutoHide();

  /*
  if (should_be_visible) {
    setCornerWidget(m_btnMainMenu, Qt::Corner::TopLeftCorner);
  }
  else {
    setCornerWidget(nullptr, Qt::Corner::TopLeftCorner);
    setCornerWidget(nullptr, Qt::Corner::TopRightCorner);
  }
  */

  m_btnMainMenu->setVisible(should_be_visible);
  // tabBar()->setVisible(should_be_visible);
}

void TabWidget::updateAppearance() {
  setTabBarAutoHide(qApp->settings()->value(GROUP(GUI), SETTING(GUI::HideTabBarIfOnlyOneTab)).toBool());
}

void TabWidget::tabInserted(int index) {
  QTabWidget::tabInserted(index);

  checkCornerButtonVisibility();
  const int count_of_tabs = count();

  if (index < count_of_tabs - 1 && count_of_tabs > 1) {
    // New tab was inserted and the tab is not the last one.
    fixContentsAfterMove(index, count_of_tabs - 1);
  }
}

void TabWidget::tabRemoved(int index) {
  QTabWidget::tabRemoved(index);

  checkCornerButtonVisibility();
  const int count_of_tabs = count();

  if (index < count_of_tabs && count_of_tabs > 1) {
    // Some tab was removed and the tab was not the last one.
    fixContentsAfterMove(index, count_of_tabs - 1);
  }
}

void TabWidget::createConnections() {
  connect(tabBar(), &TabBar::tabCloseRequested, this, &TabWidget::closeTab);
  connect(tabBar(), &TabBar::tabMoved, this, &TabWidget::fixContentsAfterMove);

  connect(feedMessageViewer()->messagesView(),
          &MessagesView::openSingleMessageInNewTab,
          this,
          &TabWidget::addSingleMessageView);

#if defined(ENABLE_MEDIAPLAYER)
  connect(feedMessageViewer()->messagesView(), &MessagesView::playLinkInMediaPlayer, this, [this](const QString& url) {
    addMediaPlayer(url, true);
  });
#endif
}

void TabWidget::initializeTabs() {
  // Create widget for "Feeds" page and add it.
  m_feedMessageViewer = new FeedMessageViewer(this);
  const int index_of_browser = addTab(m_feedMessageViewer, QIcon(), tr("Feeds"), TabBar::TabType::FeedReader);

  setTabToolTip(index_of_browser, tr("Browse your feeds and articles"));
}

void TabWidget::setupIcons() {
  // Iterate through all tabs and update icons
  // accordingly.
  for (int index = 0; index < count(); index++) {
    // Index 0 usually contains widget which displays feeds & messages.
    if (tabBar()->tabType(index) == TabBar::TabType::FeedReader) {
      setTabIcon(index, qApp->icons()->fromTheme(QSL("application-rss+xml")));
    }
  }
}

void TabWidget::scrollUpCurrentBrowser() {
  currentWidget()->webBrowser()->scrollUp();
}

void TabWidget::scrollDownCurrentBrowser() {
  currentWidget()->webBrowser()->scrollDown();
}

bool TabWidget::closeTab(int index) {
  if (tabBar()->tabType(index) == TabBar::TabType::Closable) {
    removeTab(index, true);
    return true;
  }
  else {
    return false;
  }
}

void TabWidget::closeTabWithSender() {
  auto idx = indexOf(qobject_cast<QWidget*>(sender()));

  if (idx >= 0) {
    closeTab(idx);
  }
}

void TabWidget::closeAllTabsExceptCurrent() {
  // Close tabs after active tab.
  int index_of_active = currentIndex();

  for (int i = count() - 1; i >= 0; i--) {
    if (i != index_of_active) {
      if (i < index_of_active) {
        index_of_active--;
      }

      closeTab(i);
    }
  }
}

void TabWidget::closeAllTabs() {
  for (int i = count() - 1; i >= 0; i--) {
    closeTab(i);
  }
}

void TabWidget::closeCurrentTab() {
  closeTab(currentIndex());
}

int TabWidget::addSingleMessageView(RootItem* root, const Message& message) {
  auto* browser = new MessagePreviewer(this);
  auto* msg_mdl = qApp->mainForm()->tabWidget()->feedMessageViewer()->messagesView()->sourceModel();

  connect(browser, &MessagePreviewer::markMessageRead, msg_mdl, &MessagesModel::setMessageReadById);
  connect(browser, &MessagePreviewer::markMessageImportant, msg_mdl, &MessagesModel::setMessageImportantById);
  connect(browser->menuLabels(), &LabelsMenu::setModelArticleLabelIds, msg_mdl, &MessagesModel::setMessageLabelsById);

  int index = addTab(browser, root->fullIcon(), message.m_title, TabBar::TabType::Closable);

  QTimer::singleShot(500, browser, [browser, root, message]() {
    browser->loadMessage(message, root);
  });

  return index;
}

#if defined(ENABLE_MEDIAPLAYER)
int TabWidget::addMediaPlayer(const QString& url, bool make_active) {
  // #if defined(ENABLE_MEDIAPLAYER_LIBMPV)
  // QQuickWindow::setGraphicsApi(QSGRendererInterface::GraphicsApi::OpenGL);
  // #endif

  auto* player = new MediaPlayer(this);

  connect(player, &MediaPlayer::urlDownloadRequested, qApp->web(), &WebFactory::openUrlInExternalBrowser);
  connect(player, &MediaPlayer::closed, this, &TabWidget::closeTabWithSender);

  int index = addTab(player,
                     qApp->icons()->fromTheme(QSL("player_play"), QSL("media-playback-start")),
                     tr("Media player"),
                     TabBar::TabType::Closable);

  if (make_active) {
    setCurrentIndex(index);
    player->setFocus(Qt::FocusReason::OtherFocusReason);
  }

  QTimer::singleShot(3000, player, [player, url]() {
    player->playUrl(url);
  });

  return index;
}
#endif

void TabWidget::gotoNextTab() {
  if (currentIndex() == count() - 1) {
    setCurrentIndex(0);
  }
  else {
    setCurrentIndex(currentIndex() + 1);
  }
}

void TabWidget::gotoPreviousTab() {
  if (currentIndex() == 0) {
    setCurrentIndex(count() - 1);
  }
  else {
    setCurrentIndex(currentIndex() - 1);
  }
}

void TabWidget::indentTabText(int index) {
#if defined(Q_OS_MACOS)
  if (tabBar()->tabType(index) != TabBar::TabType::FeedReader && !tabIcon(index).isNull()) {
    // We have closable tab with some icon, fix the title.
    const QString text = tabText(index);

    if (!text.startsWith(QSL("  "))) {
      setTabText(index, QSL("  ") + text);
    }
  }
#else
  Q_UNUSED(index)
#endif
}

void TabWidget::removeTab(int index, bool clear_from_memory) {
  if (clear_from_memory) {
    widget(index)->deleteLater();
  }

  QTabWidget::removeTab(index);
}

int TabWidget::addTab(TabContent* widget, const QIcon& icon, const QString& label, TabBar::TabType type) {
  const int index = QTabWidget::addTab(widget, icon, TextFactory::shorten(label));

  tabBar()->setTabType(index, type);
  indentTabText(index);
  return index;
}

int TabWidget::addTab(TabContent* widget, const QString& label, TabBar::TabType type) {
  const int index = QTabWidget::addTab(widget, TextFactory::shorten(label));

  tabBar()->setTabType(index, type);
  indentTabText(index);
  return index;
}

int TabWidget::insertTab(int index, QWidget* widget, const QIcon& icon, const QString& label, TabBar::TabType type) {
  const int tab_index = QTabWidget::insertTab(index, widget, icon, label);

  tabBar()->setTabType(tab_index, type);
  indentTabText(index);
  return tab_index;
}

int TabWidget::insertTab(int index, QWidget* widget, const QString& label, TabBar::TabType type) {
  const int tab_index = QTabWidget::insertTab(index, widget, label);

  tabBar()->setTabType(tab_index, type);
  indentTabText(index);
  return tab_index;
}

void TabWidget::changeIcon(int index, const QIcon& new_icon) {
  setTabIcon(index, new_icon);
  indentTabText(index);
}

void TabWidget::changeTitle(int index, const QString& new_title) {
  setTabText(index, TextFactory::shorten(new_title));
  setTabToolTip(index, TextFactory::shorten(new_title));
  indentTabText(index);
}

void TabWidget::fixContentsAfterMove(int from, int to) {
  from = qMin(from, to);
  to = qMax(from, to);

  for (; from <= to; from++) {
    auto* content = widget(from);

    content->setIndex(from);
  }
}
