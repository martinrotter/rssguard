// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef TABWIDGET_H
#define TABWIDGET_H

#include "gui/tabbar.h"
#include "gui/tabcontent.h"

#include <QTabWidget>
#include <QUrl>


class QMenu;
class PlainToolButton;
class Message;
class FeedMessageViewer;

class TabWidget : public QTabWidget {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit TabWidget(QWidget *parent = 0);
    virtual ~TabWidget();

    // Manimulators for tabs.
    int addTab(TabContent *widget, const QString &,
               const TabBar::TabType &type = TabBar::NonClosable);
    int addTab(TabContent *widget, const QIcon &icon,
               const QString &label, const TabBar::TabType &type = TabBar::NonClosable);
    int insertTab(int index, QWidget *widget, const QString &label,
                  const TabBar::TabType &type = TabBar::Closable);
    int insertTab(int index, QWidget *widget, const QIcon &icon,
                  const QString &label, const TabBar::TabType &type = TabBar::NonClosable);
    void removeTab(int index, bool clear_from_memory);

    // Returns tab bar.
    inline TabBar *tabBar() {
      return static_cast<TabBar*>(QTabWidget::tabBar());
    }

    // Returns the central widget of this tab.
    inline TabContent *widget(int index) const {
      return static_cast<TabContent*>(QTabWidget::widget(index));
    }

    // Initializes TabWidget with tabs, this includes initialization
    // of main "Feeds" widget.
    void initializeTabs();

    // Sets up icons for this TabWidget.
    void setupIcons();

    // Accessor to feed/message viewer.
    inline FeedMessageViewer *feedMessageViewer() const {
      return m_feedMessageViewer;
    }

  protected:
    // Creates necesary connections.
    void createConnections();

    // Sets up properties of custom corner button.
    void setupCornerButton();
    void setupMainMenuButton();

    // Handlers of insertin/removing of tabs.
    void tabInserted(int index);
    void tabRemoved(int index);

  public slots:
    // Fixes tabs indexes.
    void fixContentsAfterMove(int from, int to);

    // Called when number of tab pages changes.
    void checkTabBarVisibility();

    // Changes icon/text of the tab.
    void changeTitle(int index, const QString &new_title);
    void changeIcon(int index, const QIcon &new_icon);

    // Closes tab with given index and deletes contained widget.
    bool closeTab(int index);
    bool closeCurrentTab();

    // Opens main menu.
    void openMainMenu();

    void showDownloadManager();

    // Closes all "closable" tabs except the active tab.
    void closeAllTabsExceptCurrent();

    // Open single or multiple (newspaper mode) messages in new tab.
    int addBrowserWithMessages(const QList<Message> &messages);

    // Adds new WebBrowser tab to global TabWidget.
    int addEmptyBrowser();

    // Adds new WebBrowser with link. This is used when user
    // selects to "Open link in new tab.".
    int addLinkedBrowser(const QUrl &initial_url = QUrl());
    int addLinkedBrowser(const QString &initial_url);

    // General method for adding WebBrowsers.
    int addBrowser(bool move_after_current,
                   bool make_active,
                   const QUrl &initial_url = QUrl());

  private:
    PlainToolButton *m_btnAddTab;
    PlainToolButton *m_btnMainMenu;
    QMenu *m_menuMain;
    FeedMessageViewer *m_feedMessageViewer;
};

#endif // TABWIDGET_H
