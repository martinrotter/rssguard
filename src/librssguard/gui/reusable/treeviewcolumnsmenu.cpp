// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/treeviewcolumnsmenu.h"

#include <QAction>
#include <QActionGroup>
#include <QHeaderView>

TreeViewColumnsMenu::TreeViewColumnsMenu(QHeaderView* parent) : NonClosableMenu(parent) {
  connect(this, &TreeViewColumnsMenu::aboutToShow, this, &TreeViewColumnsMenu::prepareMenu);
}

bool TreeViewColumnsMenu::shouldActionClose(QAction* action) const {
  if (action != nullptr && action->menu() != nullptr) {
    return true;
  }

  return NonClosableMenu::shouldActionClose(action);
}

void TreeViewColumnsMenu::prepareMenu() {
  QHeaderView* header_view = header();
  const QList<QMenu*> child_menus = findChildren<QMenu*>(QString(), Qt::FindChildOption::FindDirectChildrenOnly);

  clear();
  qDeleteAll(child_menus);

  QAction* act_stretch_last_section = addAction(tr("Stretch last section"));

  act_stretch_last_section->setCheckable(true);
  act_stretch_last_section->setChecked(header_view->stretchLastSection());

  connect(act_stretch_last_section, &QAction::triggered, this, &TreeViewColumnsMenu::setStretchLastSection);

  addSeparator();

  for (int i = 0; i < header_view->count(); i++) {
    addColumnMenu(i);
  }
}

void TreeViewColumnsMenu::showHideColumn(bool toggle) {
  Q_UNUSED(toggle)

  auto* send_act = qobject_cast<QAction*>(sender());

  if (send_act == nullptr) {
    return;
  }

  int section = send_act->data().toInt();

  header()->setSectionHidden(section, !send_act->isChecked());

  if (send_act->isChecked() && header()->sectionSize(send_act->data().toInt()) < header()->defaultSectionSize()) {
    header()->resizeSection(section, header()->defaultSectionSize());
  }
}

void TreeViewColumnsMenu::setStretchLastSection(bool stretch) {
  header()->setStretchLastSection(stretch);
}

void TreeViewColumnsMenu::setColumnResizeMode() {
  auto* send_act = qobject_cast<QAction*>(sender());

  if (send_act == nullptr) {
    return;
  }

  const int section = send_act->property("section").toInt();
  const auto mode = QHeaderView::ResizeMode(send_act->property("resizeMode").toInt());

  header()->setSectionResizeMode(section, mode);
}

void TreeViewColumnsMenu::addColumnMenu(int section) {
  QHeaderView* header_view = header();
  const QString title =
    header_view->model()->headerData(section, Qt::Orientation::Horizontal, Qt::ItemDataRole::EditRole).toString();
  auto* menu_column = new NonClosableMenu(title, this);
  QAction* act_visible = menu_column->addAction(tr("Visible"));

  act_visible->setData(section);
  act_visible->setCheckable(true);
  act_visible->setChecked(!header_view->isSectionHidden(section));

  connect(act_visible, &QAction::triggered, this, &TreeViewColumnsMenu::showHideColumn);

  menu_column->addSection(tr("Resize mode"));

  auto* group = new QActionGroup(menu_column);
  group->setExclusive(true);

  addResizeModeAction(menu_column, section, QHeaderView::ResizeMode::Interactive, tr("Interactive"), group);
  addResizeModeAction(menu_column, section, QHeaderView::ResizeMode::Stretch, tr("Stretch"), group);
  addResizeModeAction(menu_column, section, QHeaderView::ResizeMode::ResizeToContents, tr("Resize to contents"), group);

  addMenu(menu_column);
}

void TreeViewColumnsMenu::addResizeModeAction(QMenu* menu,
                                              int section,
                                              QHeaderView::ResizeMode mode,
                                              const QString& title,
                                              QActionGroup* group) {
  QAction* act = menu->addAction(title);

  act->setCheckable(true);
  act->setChecked(header()->sectionResizeMode(section) == mode);
  act->setProperty("section", section);
  act->setProperty("resizeMode", int(mode));
  group->addAction(act);

  connect(act, &QAction::triggered, this, &TreeViewColumnsMenu::setColumnResizeMode);
}

QHeaderView* TreeViewColumnsMenu::header() {
  return qobject_cast<QHeaderView*>(parent());
}
