// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/tabbar.h"

#include "definitions/definitions.h"
#include "gui/plaintoolbutton.h"
#include "miscellaneous/settings.h"

#include <QMouseEvent>
#include <QStyle>

TabBar::TabBar(QWidget* parent) : QTabBar(parent) {
  setDocumentMode(false);
  setUsesScrollButtons(true);
  setContextMenuPolicy(Qt::CustomContextMenu);
}

TabBar::~TabBar() {
  qDebug("Destroying TabBar instance.");
}

void TabBar::setTabType(int index, const TabBar::TabType& type) {
  const QTabBar::ButtonPosition button_position = static_cast<ButtonPosition>(style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition,
                                                                                                 0,
                                                                                                 this));

  switch (type) {
    case TabBar::DownloadManager:
    case TabBar::Closable: {
      PlainToolButton* close_button = new PlainToolButton(this);

      close_button->setIcon(qApp->icons()->fromTheme(QSL("application-exit")));
      close_button->setToolTip(tr("Close this tab."));
      close_button->setText(tr("Close tab"));
      close_button->setFixedSize(iconSize());

      // Close underlying tab when button is clicked.
      connect(close_button, &PlainToolButton::clicked, this, &TabBar::closeTabViaButton);
      setTabButton(index, button_position, close_button);
      break;
    }

    case TabBar::NonClosable:
    case TabBar::FeedReader:
    default:
      setTabButton(index, button_position, 0);
      break;
  }

  setTabData(index, QVariant(type));
}

void TabBar::closeTabViaButton() {
  const QAbstractButton* close_button = qobject_cast<QAbstractButton*>(sender());
  const QTabBar::ButtonPosition button_position = static_cast<ButtonPosition>(style()->styleHint(QStyle::SH_TabBar_CloseButtonPosition,
                                                                                                 0,
                                                                                                 this));

  if (close_button != nullptr) {
    // Find index of tab for this close button.
    for (int i = 0; i < count(); i++) {
      if (tabButton(i, button_position) == close_button) {
        emit tabCloseRequested(i);

        return;
      }
    }
  }
}

void TabBar::wheelEvent(QWheelEvent* event) {
  const int current_index = currentIndex();
  const int number_of_tabs = count();

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

void TabBar::mousePressEvent(QMouseEvent* event) {
  QTabBar::mousePressEvent(event);
  const int tab_index = tabAt(event->pos());

  // Check if user clicked on some tab or on empty space.
  if (tab_index >= 0) {
    // Check if user clicked tab with middle button.
    // NOTE: This needs to be done here because
    // destination does not know the original event.
    if (event->button() & Qt::MiddleButton && qApp->settings()->value(GROUP(GUI), SETTING(GUI::TabCloseMiddleClick)).toBool()) {
      if (tabType(tab_index) == TabBar::Closable || tabType(tab_index) == TabBar::DownloadManager) {
        // This tab is closable, so we can close it.
        emit tabCloseRequested(tab_index);
      }
    }
  }
}

void TabBar::mouseDoubleClickEvent(QMouseEvent* event) {
  QTabBar::mouseDoubleClickEvent(event);
  const int tab_index = tabAt(event->pos());

  // Check if user clicked on some tab or on empty space.
  if (tab_index >= 0) {
    // Check if user clicked tab with middle button.
    // NOTE: This needs to be done here because
    // destination does not know the original event.
    if (event->button() & Qt::LeftButton && qApp->settings()->value(GROUP(GUI), SETTING(GUI::TabCloseDoubleClick)).toBool()) {
      if ((tabType(tab_index) & (TabBar::Closable | TabBar::DownloadManager)) > 0) {
        // This tab is closable, so we can close it.
        emit tabCloseRequested(tab_index);
      }
    }
  }
  else {
    emit emptySpaceDoubleClicked();
  }
}
