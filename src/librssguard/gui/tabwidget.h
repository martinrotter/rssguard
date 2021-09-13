// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TABWIDGET_H
#define TABWIDGET_H

#include <QTabWidget>

#include "core/message.h"
#include "gui/tabbar.h"
#include "gui/tabcontent.h"

#include <QUrl>

class QMenu;
class PlainToolButton;
class RootItem;
class FeedMessageViewer;

class TabWidget : public QTabWidget {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit TabWidget(QWidget* parent = nullptr);
    virtual ~TabWidget();

    // Manimulators for tabs.
    int addTab(TabContent* widget, const QString&,
               TabBar::TabType type = TabBar::TabType::NonClosable);
    int addTab(TabContent* widget, const QIcon& icon,
               const QString& label, TabBar::TabType type = TabBar::TabType::NonClosable);
    int insertTab(int index, QWidget* widget, const QString& label,
                  TabBar::TabType type = TabBar::TabType::Closable);
    int insertTab(int index, QWidget* widget, const QIcon& icon,
                  const QString& label, TabBar::TabType type = TabBar::TabType::NonClosable);
    void removeTab(int index, bool clear_from_memory);

    // Returns tab bar.
    TabBar* tabBar() const;

    // Returns the central widget of this tab.
    TabContent* widget(int index) const;

    TabContent* currentWidget() const;

    // Initializes TabWidget with tabs, this includes initialization
    // of main "Feeds" widget.
    void initializeTabs();

    // Sets up icons for this TabWidget.
    void setupIcons();

    // Accessor to feed/message viewer.
    FeedMessageViewer* feedMessageViewer() const;

  public slots:

    // Called when number of tab pages changes.
    void checkTabBarVisibility();

    // Tab closing.
    bool closeTab(int index);
    void closeAllTabsExceptCurrent();
    void closeAllTabs();

    // Displays download manager.
    void showDownloadManager();

    int addNewspaperView(RootItem* root, const QList<Message>& messages);

    // Adds new WebBrowser tab to global TabWidget.
    int addEmptyBrowser();

    // Adds new WebBrowser with link. This is used when user
    // selects to "Open link in new tab.".
    int addLinkedBrowser(const QUrl& initial_url = QUrl());
    int addLinkedBrowser(const QString& initial_url);

    // General method for adding WebBrowsers.
    int addBrowser(bool move_after_current, bool make_active, const QUrl& initial_url = QUrl());

    void gotoNextTab();
    void gotoPreviousTab();

  private slots:

    // Fixes tabs indexes.
    void fixContentsAfterMove(int from, int to);

    // Changes icon/text of the tab.
    void changeTitle(int index, const QString& new_title);
    void changeIcon(int index, const QIcon& new_icon);

    // Opens main menu.
    void openMainMenu();

  private:
    void indentTabText(int index);
    void createConnections();
    void setupMainMenuButton();

    void tabInserted(int index);
    void tabRemoved(int index);

    PlainToolButton* m_btnMainMenu;
    QMenu* m_menuMain;
    FeedMessageViewer* m_feedMessageViewer;
};

inline TabBar* TabWidget::tabBar() const {
  return static_cast<TabBar*>(QTabWidget::tabBar());
}

inline TabContent* TabWidget::widget(int index) const {
  return static_cast<TabContent*>(QTabWidget::widget(index));
}

inline TabContent* TabWidget::currentWidget() const {
  return static_cast<TabContent*>(QTabWidget::currentWidget());
}

inline FeedMessageViewer* TabWidget::feedMessageViewer() const {
  return m_feedMessageViewer;
}

#endif // TABWIDGET_H
