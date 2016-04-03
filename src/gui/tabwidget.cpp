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

#include "gui/tabwidget.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"
#include "miscellaneous/iconfactory.h"
#include "gui/tabbar.h"
#include "gui/feedmessageviewer.h"
#include "gui/plaintoolbutton.h"
#include "gui/dialogs/formmain.h"
#include "gui/newspaperpreviewer.h"

#include <QMenu>
#include <QToolButton>


TabWidget::TabWidget(QWidget *parent) : QTabWidget(parent), m_menuMain(NULL) {
  setTabBar(new TabBar(this));
  setupMainMenuButton();
  createConnections();
}

TabWidget::~TabWidget() {
  qDebug("Destroying TabWidget instance.");
}

void TabWidget::setupMainMenuButton() {
  m_btnMainMenu = new PlainToolButton(this);
  m_btnMainMenu->setAutoRaise(true);
  m_btnMainMenu->setPadding(3);
  m_btnMainMenu->setToolTip(tr("Displays main menu."));
  m_btnMainMenu->setIcon(qApp->icons()->fromTheme(QSL("application-menu")));
  m_btnMainMenu->setPopupMode(QToolButton::InstantPopup);

  connect(m_btnMainMenu, SIGNAL(clicked()), this, SLOT(openMainMenu()));
}

void TabWidget::openMainMenu() {
  if (m_menuMain == NULL) {
    m_menuMain = new QMenu(tr("Main menu"), this);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuFile);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuView);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuFeeds);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuMessages);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuTools);
    m_menuMain->addMenu(qApp->mainForm()->m_ui->m_menuHelp);
  }

  QPoint button_position = m_btnMainMenu->pos();
  const QSize target_size = m_btnMainMenu->size() / 2.0;

  button_position.setX(button_position.x() + target_size.width());
  button_position.setY(button_position.y() + target_size.height());

  m_menuMain->exec(mapToGlobal(button_position));
}

void TabWidget::showDownloadManager() {
  for (int i = 0; i < count(); i++) {
    if (widget(i)->metaObject()->className() == QSL("DownloadManager")) {
      setCurrentIndex(i);
      return;
    }
  }

  // Download manager is not opened. Create tab with it.
  qApp->downloadManager()->setParent(this);
  addTab(qApp->downloadManager(), qApp->icons()->fromTheme(QSL("download-manager")), tr("Downloads"), TabBar::DownloadManager);
  setCurrentIndex(count() - 1);
}

void TabWidget::checkTabBarVisibility() {
  const bool should_be_visible = count() > 1 || !qApp->settings()->value(GROUP(GUI), SETTING(GUI::HideTabBarIfOnlyOneTab)).toBool();

  if (should_be_visible) {
    setCornerWidget(m_btnMainMenu, Qt::TopLeftCorner);
    m_btnMainMenu->setVisible(true);
  }
  else {
    setCornerWidget(0, Qt::TopLeftCorner);
    setCornerWidget(0, Qt::TopRightCorner);
    m_btnMainMenu->setVisible(false);
  }

  tabBar()->setVisible(should_be_visible);
}

void TabWidget::tabInserted(int index) {
  QTabWidget::tabInserted(index);
  checkTabBarVisibility();

  const int count_of_tabs = count();

  if (index < count_of_tabs - 1 && count_of_tabs > 1) {
    // New tab was inserted and the tab is not the last one.
    fixContentsAfterMove(index, count_of_tabs - 1);
  }
}

void TabWidget::tabRemoved(int index) {
  QTabWidget::tabRemoved(index);
  checkTabBarVisibility();

  const int count_of_tabs = count();

  if (index < count_of_tabs && count_of_tabs > 1) {
    // Some tab was removed and the tab was not the last one.
    fixContentsAfterMove(index, count_of_tabs - 1);
  }
}

void TabWidget::createConnections() {
  connect(tabBar(), SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));
  connect(tabBar(), SIGNAL(emptySpaceDoubleClicked()), this, SLOT(addEmptyBrowser()));
  connect(tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(fixContentsAfterMove(int,int)));
}

void TabWidget::initializeTabs() {
  // Create widget for "Feeds" page and add it.
  m_feedMessageViewer = new FeedMessageViewer(this);
  const int index_of_browser = addTab(m_feedMessageViewer,
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
      setTabIcon(index, qApp->icons()->fromTheme(QSL("folder-feed")));
    }
  }
}

bool TabWidget::closeTab(int index) {
  if (tabBar()->tabType(index) == TabBar::Closable) {
    removeTab(index, true);
    return true;
  }
  else if (tabBar()->tabType(index) == TabBar::DownloadManager) {
    removeTab(index, false);
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

void TabWidget::removeTab(int index, bool clear_from_memory) {
  if (clear_from_memory) {
    widget(index)->deleteLater();
  }

  QTabWidget::removeTab(index);
}

int TabWidget::addTab(TabContent *widget, const QIcon &icon, const QString &label, const TabBar::TabType &type) {
  const int index = QTabWidget::addTab(widget, icon, label);
  tabBar()->setTabType(index, type);

  return index;
}

int TabWidget::addTab(TabContent *widget, const QString &label, const TabBar::TabType &type) {
  const int index = QTabWidget::addTab(widget, label);
  tabBar()->setTabType(index, type);

  return index;
}

int TabWidget::insertTab(int index, QWidget *widget, const QIcon &icon, const QString &label, const TabBar::TabType &type) {
  const int tab_index = QTabWidget::insertTab(index, widget, icon, label);
  tabBar()->setTabType(tab_index, type);

  return tab_index;
}

int TabWidget::insertTab(int index, QWidget *widget, const QString &label, const TabBar::TabType &type) {
  const int tab_index = QTabWidget::insertTab(index, widget, label);
  tabBar()->setTabType(tab_index, type);

  return tab_index;
}

int TabWidget::addBrowserWithMessages(RootItem *root, const QList<Message> &messages) {
  NewspaperPreviewer *prev = new NewspaperPreviewer(root, messages, this);
  return addTab(prev, qApp->icons()->fromTheme(QSL("item-newspaper")), tr("Newspaper view"), TabBar::Closable);
}

void TabWidget::changeIcon(int index, const QIcon &new_icon) {
  setTabIcon(index, new_icon);
}

void TabWidget::changeTitle(int index, const QString &new_title) {
  setTabText(index, TextFactory::shorten(new_title));
  setTabToolTip(index, new_title);
}

void TabWidget::fixContentsAfterMove(int from, int to) {
  from = qMin(from, to);
  to = qMax(from, to);

  for ( ; from <= to; from++) {
    TabContent *content = static_cast<TabContent*>(widget(from));
    content->setIndex(from);
  }
}
