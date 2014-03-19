// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/tabwidget.h"

#include "core/defs.h"
#include "core/settings.h"
#include "core/textfactory.h"
#include "gui/tabbar.h"
#include "gui/iconthemefactory.h"
#include "gui/webbrowser.h"
#include "gui/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/plaintoolbutton.h"

#include <QApplication>
#include <QMenu>
#include <QToolButton>


TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent), m_menuMain(NULL) {
  setTabBar(new TabBar(this));
  setupCornerButton();
  setupMainMenuButton();
  createConnections();
}

TabWidget::~TabWidget() {
  qDebug("Destroying TabWidget instance.");
}

void TabWidget::setupCornerButton() {
  m_btnAddTab = new PlainToolButton(this);
  m_btnAddTab->setAutoRaise(true);
  m_btnAddTab->setPadding(3);
  m_btnAddTab->setToolTip(tr("Open new web browser tab."));
  m_btnAddTab->setIcon(IconThemeFactory::instance()->fromTheme("list-add"));

  connect(m_btnAddTab, SIGNAL(clicked()), this, SLOT(addEmptyBrowser()));
}

void TabWidget::setupMainMenuButton() {
  m_btnMainMenu = new PlainToolButton(this);
  m_btnMainMenu->setAutoRaise(true);
  m_btnMainMenu->setPadding(3);
  m_btnMainMenu->setToolTip(tr("Displays main menu."));
  m_btnMainMenu->setIcon(IconThemeFactory::instance()->fromTheme("application-menu"));
  m_btnMainMenu->setPopupMode(QToolButton::InstantPopup);

  connect(m_btnMainMenu, SIGNAL(clicked()), this, SLOT(openMainMenu()));
}

void TabWidget::openMainMenu() {
  if (m_menuMain == NULL) {
    m_menuMain = new QMenu(tr("Main menu"), this);
    m_menuMain->addMenu(FormMain::instance()->m_ui->m_menuFile);
    m_menuMain->addMenu(FormMain::instance()->m_ui->m_menuView);
    m_menuMain->addMenu(FormMain::instance()->m_ui->m_menuFeeds);
    m_menuMain->addMenu(FormMain::instance()->m_ui->m_menuMessages);
    m_menuMain->addMenu(FormMain::instance()->m_ui->m_menuWebBrowser);
    m_menuMain->addMenu(FormMain::instance()->m_ui->m_menuTools);
    m_menuMain->addMenu(FormMain::instance()->m_ui->m_menuHelp);
  }

  QPoint button_position = m_btnMainMenu->pos();
  QSize target_size = m_btnMainMenu->size() / 2.0;

  button_position.setX(button_position.x() + target_size.width());
  button_position.setY(button_position.y() + target_size.height());

  m_menuMain->exec(mapToGlobal(button_position));
}

void TabWidget::checkTabBarVisibility() {
  bool should_be_visible = count() > 1 || !Settings::instance()->value(APP_CFG_GUI,
                                                                     "hide_tabbar_one_tab",
                                                                     true).toBool();

  if (should_be_visible) {
    setCornerWidget(m_btnMainMenu, Qt::TopLeftCorner);
    setCornerWidget(m_btnAddTab, Qt::TopRightCorner);

    m_btnMainMenu->setVisible(true);
    m_btnAddTab->setVisible(true);
  }
  else {
    setCornerWidget(0, Qt::TopLeftCorner);
    setCornerWidget(0, Qt::TopRightCorner);

    m_btnMainMenu->setVisible(false);
    m_btnAddTab->setVisible(false);
  }

  tabBar()->setVisible(should_be_visible);
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
  connect(tabBar(), SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
  connect(tabBar(), SIGNAL(emptySpaceDoubleClicked()), this, SLOT(addEmptyBrowser()));
  connect(tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(fixContentsAfterMove(int,int)));
  connect(tabBar(), SIGNAL(currentChanged(int)), this, SLOT(fixContentAfterIndexChange(int)));
}

void TabWidget::initializeTabs() {
  // Create widget for "Feeds" page and add it.
  m_feedMessageViewer = new FeedMessageViewer(this);
  int index_of_browser = addTab(static_cast<TabContent*>(m_feedMessageViewer),
                                QIcon(),
                                tr("Feeds"),
                                TabBar::FeedReader);
  setTabToolTip(index_of_browser, tr("Browse your feeds and messages"));
}

void TabWidget::setupIcons() {
  // Iterate through all tabs and update icons
  // accordingly.
  for (int index = 0; index < count(); index++) {
    // Index 0 usually contains widget which displays feeds & messages.
    if (tabBar()->tabType(index) == TabBar::FeedReader) {
      setTabIcon(index, IconThemeFactory::instance()->fromTheme("folder-feed"));
    }
    // Other indexes probably contain WebBrowsers.
    else {
      WebBrowser *active_browser = widget(index)->webBrowser();
      if (active_browser != NULL && active_browser->icon().isNull()) {
        // We found WebBrowser instance of this tab page, which
        // has no suitable icon, load a new one from the icon theme.
        setTabIcon(index, IconThemeFactory::instance()->fromTheme("text-html"));
      }
    }
  }

  // Setup corner button icon.
  m_btnAddTab->setIcon(IconThemeFactory::instance()->fromTheme("list-add"));
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

int TabWidget::addBrowserWithMessages(const QList<Message> &messages) {
  int new_index = addBrowser(false, true);
  WebBrowser *browser = static_cast<WebBrowser*>(widget(new_index));

  browser->setNavigationBarVisible(false);
  browser->navigateToMessages(messages);

  return new_index;
}

int TabWidget::addEmptyBrowser() {
  return addBrowser(false, true);
}

int TabWidget::addLinkedBrowser(const QString &initial_url) {
  return addLinkedBrowser(QUrl(initial_url));
}

int TabWidget::addLinkedBrowser(const QUrl &initial_url) {
  return addBrowser(Settings::instance()->value(APP_CFG_BROWSER,
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
                            IconThemeFactory::instance()->fromTheme("text-html"),
                            tr("Web browser"),
                            TabBar::Closable);
  }
  else {
    // Add new browser as the last tab.
    final_index = addTab(browser,
                         IconThemeFactory::instance()->fromTheme("text-html"),
                         //: Web browser default tab title.
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
