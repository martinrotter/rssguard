// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TABBAR_H
#define TABBAR_H

#include "miscellaneous/iconfactory.h"

#include <QTabBar>
#include <QVariant>

class TabBar : public QTabBar {
    Q_OBJECT

  public:
    enum class TabType {
      FeedReader = 1,
      DownloadManager = 2,
      NonClosable = 4,
      Closable = 8
    };

    // Constructors.
    explicit TabBar(QWidget* parent = nullptr);
    virtual ~TabBar();

    // Getter/setter for tab type.
    void setTabType(int index, TabType type);
    TabBar::TabType tabType(int index) const;

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

inline TabBar::TabType TabBar::tabType(int index) const {
  return static_cast<TabBar::TabType>(tabData(index).toInt());
}

TabBar::TabType operator|(TabBar::TabType a, TabBar::TabType b);
TabBar::TabType operator&(TabBar::TabType a, TabBar::TabType b);
TabBar::TabType& operator|=(TabBar::TabType& a, TabBar::TabType b);
TabBar::TabType& operator&=(TabBar::TabType& a, TabBar::TabType b);

#endif // TABBAR_H
