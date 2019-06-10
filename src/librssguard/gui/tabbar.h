// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TABBAR_H
#define TABBAR_H

#include "miscellaneous/iconfactory.h"

#include <QTabBar>
#include <QVariant>

class TabBar : public QTabBar {
  Q_OBJECT

  public:
    enum TabType {
      FeedReader = 1,
      DownloadManager = 2,
      NonClosable = 4,
      Closable = 8
    };

    // Constructors.
    explicit TabBar(QWidget* parent = 0);
    virtual ~TabBar();

    // Getter/setter for tab type.
    void setTabType(int index, const TabBar::TabType& type);

    inline TabBar::TabType tabType(int index) const {
      return static_cast<TabBar::TabType>(tabData(index).value<int>());
    }

  private slots:

    // Called when user selects to close tab via close button.
    void closeTabViaButton();

  private:

    // Reimplementations.
    void mouseDoubleClickEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void wheelEvent(QWheelEvent* event);

  signals:

    // Emmited if empty space on tab bar is double clicked.
    void emptySpaceDoubleClicked();
};

#endif // TABBAR_H
