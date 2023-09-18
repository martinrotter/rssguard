// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/basetreeview.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QKeyEvent>

BaseTreeView::BaseTreeView(QWidget* parent) : QTreeView(parent) {
  m_allowedKeyboardKeys = {Qt::Key::Key_Back,
                           Qt::Key::Key_Select,
                           Qt::Key::Key_Copy,
                           Qt::Key::Key_Shift,
                           Qt::Key::Key_Control,
                           Qt::Key::Key_Up,
                           Qt::Key::Key_Down,
                           Qt::Key::Key_Left,
                           Qt::Key::Key_Right,
                           Qt::Key::Key_Home,
                           Qt::Key::Key_End,
                           Qt::Key::Key_PageUp,
                           Qt::Key::Key_PageDown};
}

void BaseTreeView::keyPressEvent(QKeyEvent* event) {
  if (qApp->settings()->value(GROUP(Feeds), SETTING(Feeds::OnlyBasicShortcutsInLists)).toBool() &&
      !m_allowedKeyboardKeys.contains(event->key()) && !event->matches(QKeySequence::StandardKey::SelectAll)) {
    event->ignore();
  }
  else {
    QTreeView::keyPressEvent(event);
  }
}
