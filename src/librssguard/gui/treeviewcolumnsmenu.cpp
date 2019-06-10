// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/treeviewcolumnsmenu.h"

#include <QHeaderView>

TreeViewColumnsMenu::TreeViewColumnsMenu(QHeaderView* parent) : QMenu(parent) {
  connect(this, &TreeViewColumnsMenu::aboutToShow, this, &TreeViewColumnsMenu::prepareMenu);
}

TreeViewColumnsMenu::~TreeViewColumnsMenu() {}

void TreeViewColumnsMenu::prepareMenu() {
  QHeaderView* header_view = header();

  for (int i = 0; i < header_view->count(); i++) {
    QAction* act = addAction(header_view->model()->headerData(i, Qt::Horizontal, Qt::EditRole).toString());

    act->setData(i);
    act->setCheckable(true);
    act->setChecked(!header_view->isSectionHidden(i));
    connect(act, &QAction::toggled, this, &TreeViewColumnsMenu::actionTriggered);
  }
}

void TreeViewColumnsMenu::actionTriggered(bool toggle) {
  Q_UNUSED(toggle)
  QAction * send_act = qobject_cast<QAction*>(sender());
  header()->setSectionHidden(send_act->data().toInt(), !send_act->isChecked());
}

QHeaderView* TreeViewColumnsMenu::header() {
  return qobject_cast<QHeaderView*>(parent());
}
