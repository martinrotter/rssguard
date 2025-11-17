// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/toolbars/basetoolbar.h"

#include "definitions/definitions.h"
#include "qtlinq/qtlinq.h"
#include "miscellaneous/settings.h"

#include <QFont>
#include <QPainter>
#include <QWidgetAction>

BaseToolBar::BaseToolBar(const QString& title, QWidget* parent) : QToolBar(title, parent) {
  // Update right margin of filter textbox.
  QMargins margins = contentsMargins();

  margins.setRight(margins.right() + FILTER_RIGHT_MARGIN);
  setContentsMargins(margins);
}

BaseToolBar::~BaseToolBar() {
  qDebugNN << LOGSEC_GUI << "Destroying BaseToolBar instance.";
}

QList<QAction*> BaseBar::extraActions() const {
  return {};
}

void BaseBar::loadSavedActions() {
  loadSpecificActions(convertActions(savedActions()), true);
}

QAction* BaseBar::findMatchingAction(const QString& action, const QList<QAction*>& actions) const {
  for (QAction* act : actions) {
    auto act_obj_name = act->objectName();

    // NOTE: action.startsWith(act_obj_name)
    if (!act_obj_name.isEmpty() && QString::compare(action, act_obj_name, Qt::CaseSensitivity::CaseInsensitive) == 0) {
      return act;
    }
  }

  return nullptr;
}

void BaseToolBar::addActionToMenu(QMenu* menu,
                                  const QIcon& icon,
                                  const QString& title,
                                  const QString& tooltip_suffix,
                                  const QVariant& value,
                                  const QString& object_name) {
  QAction* action = menu->addAction(icon, title);

  action->setToolTip(title + tooltip_suffix);
  action->setCheckable(true);
  action->setData(value);
  action->setObjectName(object_name);
}

void BaseToolBar::activateAction(const QString& action_name, QWidgetAction* widget_action) {
  const int start = action_name.indexOf('[');
  const int end = action_name.indexOf(']');

  if (start != -1 && end != -1 && (start + 1 != end) && end == action_name.length() - 1) {
    const QStringList menu_action_names =
      action_name.chopped(1).right(end - start - 1).split(QL1C(';'), SPLIT_BEHAVIOR::SkipEmptyParts);

    if (menu_action_names.isEmpty()) {
      // NOTE: No sub-items are activated, exit.
      return;
    }

    auto tool_btn = qobject_cast<QToolButton*>(widget_action->defaultWidget());

    for (QAction* action : tool_btn->menu()->actions()) {
      if (menu_action_names.contains(action->objectName()) && !action->isChecked()) {
        action->trigger();
      }
    }
  }
}

void BaseToolBar::saveToolButtonSelection(const QString& button_name,
                                          const QString& setting_name,
                                          const QList<QAction*>& actions) const {
  QStringList action_names = savedActions();
  QStringList opts = qlinq::from(actions)
                       .select([](const QAction* act) {
                         return act->objectName();
                       })
                       .toList();

  for (QString& action_name : action_names) {
    if (action_name.startsWith(button_name)) {
      action_name = button_name + QSL("[%1]").arg(opts.join(QL1C(';')));
    }
  }

  qApp->settings()->setValue(GROUP(GUI), setting_name, action_names.join(QSL(",")));
}

void BaseToolBar::drawNumberOfCriterias(QToolButton* btn, int count) {
  QPixmap px(128, 128);
  px.fill(Qt::GlobalColor::transparent);

  QPainter p(&px);

  auto fon = p.font();

  fon.setPixelSize(40);
  p.setFont(fon);

  p.drawPixmap(0, 0, 80, 80, btn->defaultAction()->icon().pixmap(128, 128));
  p.drawText(65, 65, 50, 50, Qt::AlignmentFlag::AlignCenter, QString::number(count));

  btn->setIcon(px);
}
