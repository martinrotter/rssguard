// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/basetoolbar.h"

#include "definitions/definitions.h"
#include "gui/dialogs/formmain.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>

BaseToolBar::BaseToolBar(const QString& title, QWidget* parent) : QToolBar(title, parent) {
  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();

  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);
}

BaseToolBar::~BaseToolBar() {
  qDebug("Destroying BaseToolBar instance.");
}

void BaseBar::loadSavedActions() {
  loadSpecificActions(getSpecificActions(savedActions()));
}

QAction* BaseBar::findMatchingAction(const QString& action, const QList<QAction*>& actions) const {
  foreach (QAction* act, actions) {
    if (act->objectName() == action) {
      return act;
    }
  }

  return nullptr;
}
