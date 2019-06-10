// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/feedstoolbar.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QWidgetAction>

FeedsToolBar::FeedsToolBar(const QString& title, QWidget* parent) : BaseToolBar(title, parent) {
  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();

  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);
}

FeedsToolBar::~FeedsToolBar() {}

QList<QAction*> FeedsToolBar::availableActions() const {
  return qApp->userActions();
}

QList<QAction*> FeedsToolBar::changeableActions() const {
  return actions();
}

void FeedsToolBar::saveChangeableActions(const QStringList& actions) {
  qApp->settings()->setValue(GROUP(GUI), GUI::FeedsToolbarActions, actions.join(QSL(",")));
  loadSpecificActions(getSpecificActions(actions));
}

QList<QAction*> FeedsToolBar::getSpecificActions(const QStringList& actions) {
  QList<QAction*> available_actions = availableActions();
  QList<QAction*> spec_actions;

  // Iterate action names and add respectable actions into the toolbar.
  foreach (const QString& action_name, actions) {
    QAction* matching_action = findMatchingAction(action_name, available_actions);

    if (matching_action != nullptr) {
      // Add existing standard action.
      spec_actions.append(matching_action);
    }
    else if (action_name == SEPARATOR_ACTION_NAME) {
      // Add new separator.
      QAction* act = new QAction(this);

      act->setSeparator(true);
      spec_actions.append(act);
    }
    else if (action_name == SPACER_ACTION_NAME) {
      // Add new spacer.
      QWidget* spacer = new QWidget(this);

      spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
      QWidgetAction* action = new QWidgetAction(this);

      action->setDefaultWidget(spacer);
      action->setIcon(qApp->icons()->fromTheme(QSL("system-search")));
      action->setProperty("type", SPACER_ACTION_NAME);
      action->setProperty("name", tr("Toolbar spacer"));
      spec_actions.append(action);
    }
  }

  return spec_actions;
}

void FeedsToolBar::loadSpecificActions(const QList<QAction*>& actions) {
  clear();

  foreach (QAction* act, actions) {
    addAction(act);
  }
}

QStringList FeedsToolBar::defaultActions() const {
  return QString(GUI::FeedsToolbarActionsDef).split(',',
                                                    QString::SkipEmptyParts);
}

QStringList FeedsToolBar::savedActions() const {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::FeedsToolbarActions)).toString().split(',',
                                                                                                 QString::SkipEmptyParts);
}
