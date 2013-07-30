#include <QMouseEvent>

#include "core/defs.h"
#include "core/settings.h"
#include "gui/tabbar.h"


TabBar::TabBar(QWidget *parent) : QTabBar(parent) {
  setDocumentMode(true);
  setUsesScrollButtons(true);
  setContextMenuPolicy(Qt::CustomContextMenu);
}

TabBar::~TabBar() {
}

void TabBar::setTabType(int index, const TabBar::TabType &type) {
  setTabData(index, QVariant(type));
}

TabBar::TabType TabBar::tabType(int index) {
  return static_cast<TabBar::TabType>(tabData(index).value<int>());
}

void TabBar::mousePressEvent(QMouseEvent *event) {
  QTabBar::mousePressEvent(event);

  int tab_index = tabAt(event->pos());

  // Check if user clicked on some tab or on empty space.
  if (tab_index >= 0) {
    // Check if user clicked tab with middle button.
    // NOTE: This needs to be done here because
    // destination does not know the original event.
    if (event->button() & Qt::MiddleButton && Settings::getInstance()->value(APP_CFG_GUI,
                                                                             "tab_close_mid_button",
                                                                             true).toBool()) {
      if (tabType(tab_index) == TabBar::Closable) {
        // This tab is closable, so we can close it.
        emit tabCloseRequested(tab_index);
      }
    }
  }
}

void TabBar::mouseDoubleClickEvent(QMouseEvent *event) {
  QTabBar::mouseDoubleClickEvent(event);

  int tab_index = tabAt(event->pos());

  // Check if user clicked on some tab or on empty space.
  if (tab_index >= 0) {
    // Check if user clicked tab with middle button.
    // NOTE: This needs to be done here because
    // destination does not know the original event.
    if (event->button() & Qt::LeftButton && Settings::getInstance()->value(APP_CFG_GUI,
                                                                           "tab_close_double_button",
                                                                           true).toBool()) {
      if (tabType(tab_index) == TabBar::Closable) {
        // This tab is closable, so we can close it.
        emit tabCloseRequested(tab_index);
      }
    }
  }
  // Check if new tab should be opened with initial web browser.
  // NOTE: This check could be unnecesary here and should be done in
  // destination object but we keep it here for consistency.
  else if (Settings::getInstance()->value(APP_CFG_GUI,
                                          "tab_new_double_button",
                                          true).toBool()) {
    emit emptySpaceDoubleClicked();
  }
}
