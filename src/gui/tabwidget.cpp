#include <QUrl>
#include <QApplication>

#include "core/defs.h"
#include "core/settings.h"
#include "core/textfactory.h"
#include "gui/tabwidget.h"
#include "gui/tabbar.h"
#include "gui/iconthemefactory.h"
#include "gui/webbrowser.h"
#include "gui/feedmessageviewer.h"
#include "gui/cornerbutton.h"


TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent) {
  setTabBar(new TabBar(this));
  setupCornerButton();
  createConnections();
}

TabWidget::~TabWidget() {
  qDebug("Destroying TabWidget instance.");
}

void TabWidget::setupCornerButton() {
  m_cornerButton = new CornerButton(this);
  setCornerWidget(m_cornerButton);
}

void TabWidget::checkTabBarVisibility() {
  tabBar()->setVisible(count() > 1 || !Settings::getInstance()->value(APP_CFG_GUI,
                                                                     "hide_tabbar_one_tab",
                                                                     true).toBool());
}

void TabWidget::tabInserted(int index) {
  QTabWidget::tabInserted(index);
  checkTabBarVisibility();
}

void TabWidget::tabRemoved(int index) {
  QTabWidget::tabRemoved(index);
  checkTabBarVisibility();
}

void TabWidget::createConnections() {
  connect(m_cornerButton, SIGNAL(clicked()), this, SLOT(addEmptyBrowser()));
  connect(tabBar(), SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
  connect(tabBar(), SIGNAL(emptySpaceDoubleClicked()), this, SLOT(addEmptyBrowser()));
  connect(tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(fixContentsAfterMove(int,int)));
  connect(tabBar(), SIGNAL(currentChanged(int)), this, SLOT(fixContentAfterIndexChange(int)));
}

TabBar *TabWidget::tabBar() {
  return static_cast<TabBar*>(QTabWidget::tabBar());
}

void TabWidget::initializeTabs() { 
  // Create widget for "Feeds" page and add it.
  FeedMessageViewer *browser = new FeedMessageViewer(this);
  int index_of_browser = addTab(static_cast<TabContent*>(browser),
                                QIcon(),
                                tr("Feeds"),
                                TabBar::FeedReader);
  setTabToolTip(index_of_browser, tr("Browse your feeds and messages"));
}

TabContent *TabWidget::widget(int index) const {
  return static_cast<TabContent*>(QTabWidget::widget(index));
}

void TabWidget::setupIcons() {
  // Iterate through all tabs and update icons
  // accordingly.
  for (int index = 0; index < count(); index++) {
    // Index 0 usually contains widget which displays feeds & messages.
    if (tabBar()->tabType(index) == TabBar::FeedReader) {
      setTabIcon(index, IconThemeFactory::getInstance()->fromTheme("application-rss+xml"));
    }
    // Other indexes probably contain WebBrowsers.
    else {
      WebBrowser *active_browser = widget(index)->webBrowser();
      if (active_browser != NULL && active_browser->icon().isNull()) {
        // We found WebBrowser instance of this tab page, which
        // has no suitable icon, load a new one from the icon theme.
        setTabIcon(index, IconThemeFactory::getInstance()->fromTheme("text-html"));
      }
    }
  }

  // Setup corner button icon.
  m_cornerButton->setIcon(IconThemeFactory::getInstance()->fromTheme("list-add"));
}

bool TabWidget::closeTab(int index) {
  if (tabBar()->tabType(index) == TabBar::Closable) {
    removeTab(index);
    return true;
  }
  else {
    return false;
  }
}

bool TabWidget::closeCurrentTab() {
  return closeTab(currentIndex());
}

void TabWidget::closeAllTabsExceptCurrent() {
  // Close tabs after active tab.
  int index_of_active = currentIndex();
  int total_count = count();
  int iterative_index = 0;

  while (total_count-- > 0) {
    if (iterative_index < index_of_active) {
      // Deleting tab on the left from the active one.
      if (closeTab(iterative_index)) {
        // We successfully deleted that LEFT tab.
        index_of_active--;
      }
      else {
        // We reached "non-closable" tab, go forward.
        iterative_index++;
      }
    }
    else if (iterative_index > index_of_active) {
      // Deleting tab on the right from the active one.
      if (!closeTab(iterative_index)) {
        // We reached "non-closable" tab, go forward.
        iterative_index++;
      }
    }
    else {
      // We iterate through active tab now, no deleting;
      iterative_index++;
    }
  }
}

void TabWidget::removeTab(int index) {
  widget(index)->deleteLater();
  QTabWidget::removeTab(index);
}

int TabWidget::addTab(TabContent *widget, const QIcon &icon,
                      const QString &label, const TabBar::TabType &type) {
  int index = QTabWidget::addTab(widget, icon, label);
  tabBar()->setTabType(index, type);

  return index;
}

int TabWidget::addTab(TabContent *widget, const QString &label, const TabBar::TabType &type) {
  int index = QTabWidget::addTab(widget, label);
  tabBar()->setTabType(index, type);

  return index;
}

int TabWidget::insertTab(int index, QWidget *widget, const QIcon &icon,
                         const QString &label, const TabBar::TabType &type) {
  int tab_index = QTabWidget::insertTab(index, widget, icon, label);
  tabBar()->setTabType(tab_index, type);

  return tab_index;
}

int TabWidget::insertTab(int index, QWidget *widget, const QString &label,
                         const TabBar::TabType &type) {
  int tab_index = QTabWidget::insertTab(index, widget, label);
  tabBar()->setTabType(tab_index, type);

  return tab_index;
}

int TabWidget::addEmptyBrowser() {
  return addBrowser(false, true);
}

int TabWidget::addLinkedBrowser(const QUrl &initial_url) {
  return addBrowser(Settings::getInstance()->value(APP_CFG_BROWSER,
                                                   "queue_tabs",
                                                   true).toBool(),
                    false,
                    initial_url);
}

int TabWidget::addBrowser(bool move_after_current,
                          bool make_active,
                          const QUrl &initial_url) {
  // Create new WebBrowser.
  WebBrowser *browser = new WebBrowser(this);
  browser->setupIcons();

  int final_index;

  if (move_after_current) {
    // Insert web browser after current tab.
    final_index = insertTab(currentIndex() + 1,
                            browser,
                            IconThemeFactory::getInstance()->fromTheme("text-html"),
                            tr("Web browser"),
                            TabBar::Closable);
  }
  else {
    // Add new browser as the last tab.
    final_index = addTab(browser,
                         IconThemeFactory::getInstance()->fromTheme("text-html"),
                         tr("Web browser"),
                         TabBar::Closable);
  }

  // Make connections.
  connect(browser, SIGNAL(titleChanged(int,QString)),
          this, SLOT(changeTitle(int,QString)));
  connect(browser, SIGNAL(iconChanged(int,QIcon)),
          this, SLOT(changeIcon(int,QIcon)));

  // Setup the tab index.
  browser->setIndex(final_index);

  // Load initial web page if desired.
  if (initial_url.isValid()) {
    browser->navigateToUrl(initial_url);
  }

  // Make new web browser active if desired.
  if (make_active) {
    setCurrentIndex(final_index);
    browser->setFocus(Qt::OtherFocusReason);
  }

  return final_index;
}

void TabWidget::changeIcon(int index, const QIcon &new_icon) {
  setTabIcon(index, new_icon);
}

void TabWidget::changeTitle(int index, const QString &new_title) {
  setTabText(index, TextFactory::shorten(new_title));
  setTabToolTip(index, new_title);
}

void TabWidget::fixContentAfterIndexChange(int from) {
  fixContentsIndexes(from, count() - 1);
}

void TabWidget::fixContentsAfterMove(int from, int to) {
  fixContentsIndexes(qMin(from, to), qMax(from, to));
}

void TabWidget::fixContentsIndexes(int starting_index, int ending_index) {
  for ( ; starting_index <= ending_index; starting_index++) {
    TabContent *content = static_cast<TabContent*>(widget(starting_index));
    content->setIndex(starting_index);
  }
}
