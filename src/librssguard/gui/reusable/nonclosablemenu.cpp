// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/nonclosablemenu.h"

#include <QKeyEvent>

NonClosableMenu::NonClosableMenu(QWidget* parent) : QMenu(parent) {}

NonClosableMenu::NonClosableMenu(const QString& title, QWidget* parent) : QMenu(title, parent) {}

bool NonClosableMenu::shouldActionClose(QAction* action) const {
  return false;
}

void NonClosableMenu::keyPressEvent(QKeyEvent* event) {
  auto* act = activeAction();

  if (act == nullptr || shouldActionClose(act) || event->key() != Qt::Key::Key_Space) {
    QMenu::keyPressEvent(event);
  }
}

void NonClosableMenu::mousePressEvent(QMouseEvent* event) {
  auto* act = activeAction();

  if (!shouldActionClose(act) && act != nullptr) {
    act->trigger();
  }
  else if (act == nullptr || shouldActionClose(act)) {
    QMenu::mousePressEvent(event);
  }
}

void NonClosableMenu::mouseReleaseEvent(QMouseEvent* event) {
  auto* act = activeAction();

  if (act == nullptr || shouldActionClose(act)) {
    QMenu::mouseReleaseEvent(event);
  }
}
