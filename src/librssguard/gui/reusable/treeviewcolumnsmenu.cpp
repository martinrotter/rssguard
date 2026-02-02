// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/treeviewcolumnsmenu.h"

#include <QHeaderView>

TreeViewColumnsMenu::TreeViewColumnsMenu(QHeaderView* parent) : NonClosableMenu(parent) {
  connect(this, &TreeViewColumnsMenu::aboutToShow, this, &TreeViewColumnsMenu::prepareMenu);
}

void TreeViewColumnsMenu::prepareMenu() {
  QHeaderView* header_view = header();

  for (int i = 0; i < header_view->count(); i++) {
    QAction* act = addAction(header_view->model()
                               ->headerData(i, Qt::Orientation::Horizontal, Qt::ItemDataRole::EditRole)
                               .toString());

    act->setData(i);
    act->setCheckable(true);
    act->setChecked(!header_view->isSectionHidden(i));

    connect(act, &QAction::triggered, this, &TreeViewColumnsMenu::showHideColumn);
  }
}

void TreeViewColumnsMenu::showHideColumn(bool toggle) {
  Q_UNUSED(toggle)

  auto* send_act = qobject_cast<QAction*>(sender());
  int section = send_act->data().toInt();

  header()->setSectionHidden(section, !send_act->isChecked());

  if (send_act->isChecked() && header()->sectionSize(send_act->data().toInt()) < header()->defaultSectionSize()) {
    header()->resizeSection(section, header()->defaultSectionSize());
  }
}

QHeaderView* TreeViewColumnsMenu::header() {
  return qobject_cast<QHeaderView*>(parent());
}
