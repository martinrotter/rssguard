#include "gui/tabbar.h"

#include "core/defs.h"
#include "core/settings.h"

#include <QMouseEvent>
#include <QToolButton>
#include <QStyle>


TabBar::TabBar(QWidget *parent) : QTabBar(parent) {
  setDocumentMode(true);
  setUsesScrollButtons(true);
  setContextMenuPolicy(Qt::CustomContextMenu);
}

TabBar::~TabBar() {
  qDebug("Destroying TabBar instance.");
}

void TabBar::setTabType(int index, const TabBar::TabType &type) {
  switch (type) {
    case TabBar::Closable: {
      QToolButton *close_button = new QToolButton(this);

      close_button->setIcon(IconThemeFactory::instance()->fromTheme("application-exit"));
      close_button->setToolTip(tr("Close this tab."));
      close_button->setText(tr("Close tab"));
      close_button->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
      close_button->setFixedSize(iconSize());

      // Close underlying tab when button is clicked.
      connect(close_button, SIGNAL(clicked()),
              this, SLOT(closeTabViaButton()));

      setTabButton(index, QTabBar::RightSide, close_button);
      break;
    }

    case TabBar::NonClosable:
    case TabBar::FeedReader:
    default:
      setTabButton(index, QTabBar::RightSide, 0);
      break;
  }

  setTabData(index, QVariant(type));
}

void TabBar::closeTabViaButton() {
  QToolButton *close_button = qobject_cast<QToolButton*>(sender());
  QTabBar::ButtonPosition button_position = static_cast<ButtonPosition>(style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition,
                                                                                           0,
                                                                                           this));

  if (close_button != NULL) {
    // Find index of tab for this close button.
    for (int i = 0; i < count(); i++) {
      if (tabButton(i, button_position) == close_button) {
        emit tabCloseRequested(i);

        return;
      }
    }
  }
}

void TabBar::wheelEvent(QWheelEvent *event) {
  int current_index = currentIndex();
  int number_of_tabs = count();

  // Make sure rotating works.
  if (number_of_tabs > 1) {
    if (event->delta() > 0) {
      // Scroll to the LEFT tab.
      setCurrentIndex(current_index == 0 ?
                        number_of_tabs - 1 :
                        current_index - 1);
    }
    else if (event->delta() < 0) {
      // Scroll to the RIGHT tab.
      setCurrentIndex(current_index == number_of_tabs - 1 ?
                        0 :
                        current_index + 1);
    }
  }
}

void TabBar::mousePressEvent(QMouseEvent *event) {
  QTabBar::mousePressEvent(event);

  int tab_index = tabAt(event->pos());

  // Check if user clicked on some tab or on empty space.
  if (tab_index >= 0) {
    // Check if user clicked tab with middle button.
    // NOTE: This needs to be done here because
    // destination does not know the original event.
    if (event->button() & Qt::MiddleButton && Settings::instance()->value(APP_CFG_GUI,
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
    if (event->button() & Qt::LeftButton && Settings::instance()->value(APP_CFG_GUI,
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
  else if (Settings::instance()->value(APP_CFG_GUI,
                                       "tab_new_double_button",
                                       true).toBool()) {
    emit emptySpaceDoubleClicked();
  }
}
