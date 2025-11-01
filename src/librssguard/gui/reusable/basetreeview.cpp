// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/basetreeview.h"

#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"

#include <QKeyEvent>
#include <QScrollBar>

BaseTreeView::BaseTreeView(QWidget* parent) : QTreeView(parent), m_lastWheelTime(0), m_scrollSpeedFactor(1.0) {
  setAllColumnsShowFocus(true);
  setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

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

bool BaseTreeView::isIndexHidden(const QModelIndex& idx) const {
  return QTreeView::isIndexHidden(idx);
}

void BaseTreeView::wheelEvent(QWheelEvent* event) {
  qint64 now = m_scrollingTimer.elapsed();

  if (m_lastWheelTime == 0) {
    m_scrollingTimer.start();
    m_lastWheelTime = 1;
  }
  else {
    qint64 dt = now - m_lastWheelTime;
    m_lastWheelTime = now;

    // If scrolling quickly, boost speed; otherwise decay.
    if (dt < 120) {                                           // <120 ms between wheel events → user scrolling fast
      m_scrollSpeedFactor = qMin(m_scrollSpeedFactor * 1.4, 6.0); // accelerate up to 6×
    }
    else {
      m_scrollSpeedFactor = qMax(m_scrollSpeedFactor * 0.5, 1.0); // slowly return to normal
    }
  }

  int delta = static_cast<int>(event->angleDelta().y() * m_scrollSpeedFactor);
  verticalScrollBar()->setValue(verticalScrollBar()->value() - delta);

  qDebugNN << LOGSEC_GUI << "Tree view scrolling factor is" << NONQUOTE_W_SPACE_DOT(m_scrollSpeedFactor);
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
