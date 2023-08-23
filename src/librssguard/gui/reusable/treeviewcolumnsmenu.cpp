// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/treeviewcolumnsmenu.h"

#include "definitions/definitions.h"

#include <QHeaderView>

TreeViewColumnsMenu::TreeViewColumnsMenu(QHeaderView* parent) : NonClosableMenu(parent) {
  connect(this, &TreeViewColumnsMenu::aboutToShow, this, &TreeViewColumnsMenu::prepareMenu);
}

void TreeViewColumnsMenu::prepareMenu() {
  QHeaderView* header_view = header();

  for (int i = 0; i < header_view->count(); i++) {
    QAction* act = addAction(header_view->model()->headerData(i, Qt::Horizontal, Qt::EditRole).toString());

    act->setData(i);
    act->setCheckable(true);
    act->setChecked(!header_view->isSectionHidden(i));

    connect(act, &QAction::triggered, this, &TreeViewColumnsMenu::actionTriggered);
  }
}

void TreeViewColumnsMenu::actionTriggered(bool toggle) {
  auto* send_act = qobject_cast<QAction*>(sender());

  header()->setSectionHidden(send_act->data().toInt(), !send_act->isChecked());

  if (send_act->isChecked() && header()->sectionSize(send_act->data().toInt()) < 10) {
    header()->resizeSection(send_act->data().toInt(), MESSAGES_VIEW_DEFAULT_COL);
  }

  Q_UNUSED(toggle)
}

QHeaderView* TreeViewColumnsMenu::header() {
  return qobject_cast<QHeaderView*>(parent());
}
