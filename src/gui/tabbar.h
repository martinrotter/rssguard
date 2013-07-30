#ifndef TABBAR_H
#define TABBAR_H

#include <QTabBar>


class TabBar : public QTabBar {
    Q_OBJECT

  public:
    enum TabType {
      FeedReader    = 1000,
      NonClosable   = 1001,
      Closable      = 1002
    };

    explicit TabBar(QWidget *parent = 0);
    virtual ~TabBar();

    // Getter/setter for tab type.
    void setTabType(int index, const TabBar::TabType &type);
    TabBar::TabType tabType(int index);
    
  protected:
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);

  signals:
    // Emmited if empty space on tab bar is double clicked.
    void emptySpaceDoubleClicked();
    
};

#endif // TABBAR_H
