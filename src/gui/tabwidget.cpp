#include <QUrl>
#include <QApplication>

#include "core/defs.h"
#include "gui/tabwidget.h"
#include "gui/tabbar.h"
#include "gui/themefactory.h"
#include "gui/webbrowser.h"


TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent) {
  setTabBar(new TabBar(this));
  createConnections();
}

TabWidget::~TabWidget() {
  qDebug("Destroying TabWidget instance.");
}

void TabWidget::createConnections() {
  connect(tabBar(), &QTabBar::tabCloseRequested, this, &TabWidget::closeTab);
  connect(tabBar(), &TabBar::emptySpaceDoubleClicked,
          this, &TabWidget::addEmptyBrowser);
}

TabBar *TabWidget::tabBar() {
  return static_cast<TabBar*>(QTabWidget::tabBar());
}

void TabWidget::initializeTabs() {
  // Create widget for "Feeds" page and add it.
  WebBrowser *browser = new WebBrowser(this);
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
      setTabIcon(index, ThemeFactory::getInstance()->fromTheme("application-rss+xml"));
    }
    // Other indexes probably contain WebBrowsers.
    else {
      WebBrowser *active_browser = widget(index)->webBrowser();
      if (active_browser != nullptr && active_browser->icon().isNull()) {
        // We found WebBrowser instance of this tab page, which
        // has no suitable icon, load a new one from the icon theme.
        setTabIcon(index, ThemeFactory::getInstance()->fromTheme("text-html"));
      }
    }
  }
}

void TabWidget::closeTab(int index) {
  removeTab(index);
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

void TabWidget::addEmptyBrowser() {
  // TODO: Add reading of move_after_current and make_active
  // flags from settings.
  addBrowser(false, true);
}

void TabWidget::addLinkedBrowser(const QUrl &initial_url) {
  // TODO: Add reading of move_after_current and make_active
  // flags from settings.
  addBrowser(true, false, initial_url);
}

void TabWidget::addBrowser(bool move_after_current,
                           bool make_active,
                           const QUrl &initial_url) {
  // Create new WebBrowser.
  WebBrowser *browser = new WebBrowser(this);
  int final_index;

  if (move_after_current) {
    // Insert web browser after current tab.
    final_index = insertTab(currentIndex() + 1,
                            browser,
                            ThemeFactory::getInstance()->fromTheme("text-html"),
                            tr("Web browser"),
                            TabBar::Closable);
  }
  else {
    // Add new browser as the last tab.
    final_index = addTab(browser,
                         ThemeFactory::getInstance()->fromTheme("text-html"),
                         tr("Web browser"),
                         TabBar::Closable);
    browser->setFocus(Qt::OtherFocusReason);
  }

  // Load initial web page if desired.
  if (initial_url.isValid()) {
    browser->navigateToUrl(initial_url);
  }

  // Make new web browser active if desired.
  if (make_active) {
    setCurrentIndex(final_index);
  }
}
