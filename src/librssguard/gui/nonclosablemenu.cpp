// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/nonclosablemenu.h"

#include <QKeyEvent>

NonClosableMenu::NonClosableMenu(QWidget* parent) : QMenu(parent) {}

NonClosableMenu::NonClosableMenu(const QString& title, QWidget* parent) : QMenu(title, parent) {}

void NonClosableMenu::keyPressEvent(QKeyEvent* event) {
  if (event->key() != Qt::Key::Key_Space) {
    QMenu::keyPressEvent(event);
  }
}

void NonClosableMenu::mousePressEvent(QMouseEvent* event) {
  Q_UNUSED(event)
}

void NonClosableMenu::mouseReleaseEvent(QMouseEvent* event) {
  Q_UNUSED(event)
}
