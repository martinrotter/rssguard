#include "gui/tabwidget.h"
#include "gui/tabbar.h"
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
