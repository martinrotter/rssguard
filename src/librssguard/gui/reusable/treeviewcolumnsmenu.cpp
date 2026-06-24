// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/treeviewcolumnsmenu.h"

#include <QAction>
#include <QActionGroup>
#include <QHeaderView>
#include <QStyle>

TreeViewColumnsMenu::TreeViewColumnsMenu(QHeaderView* parent, int highlighted_section)
  : NonClosableMenu(parent), m_highlightedSection(highlighted_section) {
  connect(this, &TreeViewColumnsMenu::aboutToShow, this, &TreeViewColumnsMenu::prepareMenu);
}

void TreeViewColumnsMenu::setMenuExtensionBuilder(const MenuExtensionBuilder& builder) {
  m_menuExtensionBuilder = builder;
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

  QAction* act_cascading_section_resizes = addAction(tr("Cascading section resizes"));

  act_cascading_section_resizes->setCheckable(true);
  act_cascading_section_resizes->setChecked(header_view->cascadingSectionResizes());

  connect(act_cascading_section_resizes,
          &QAction::triggered,
          this,
          &TreeViewColumnsMenu::setCascadingSectionResizes);

  QAction* act_autosize_visible_columns = addAction(tr("Autosize visible columns"));

  connect(act_autosize_visible_columns, &QAction::triggered, this, &TreeViewColumnsMenu::autosizeVisibleColumns);

  addSeparator();

  if (m_menuExtensionBuilder != nullptr && m_menuExtensionBuilder(this)) {
    addSeparator();
  }

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

void TreeViewColumnsMenu::setCascadingSectionResizes(bool cascade) {
  header()->setCascadingSectionResizes(cascade);
}

void TreeViewColumnsMenu::autosizeColumn() {
  auto* send_act = qobject_cast<QAction*>(sender());

  if (send_act == nullptr) {
    return;
  }

  const int section = send_act->data().toInt();

  header()->resizeSection(section, header()->sectionSizeHint(section));
}

void TreeViewColumnsMenu::autosizeVisibleColumns() {
  for (int i = 0; i < header()->count(); i++) {
    if (!header()->isSectionHidden(i) &&
        header()->sectionResizeMode(i) == QHeaderView::ResizeMode::Interactive) {
      header()->resizeSection(i, header()->sectionSizeHint(i));
    }
  }
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

  QAction* act_autosize = menu_column->addAction(tr("Autosize column"));

  act_autosize->setData(section);
  act_autosize->setEnabled(header_view->sectionResizeMode(section) == QHeaderView::ResizeMode::Interactive);

  connect(act_autosize, &QAction::triggered, this, &TreeViewColumnsMenu::autosizeColumn);

  menu_column->addSection(tr("Resize mode"));

  auto* group = new QActionGroup(menu_column);
  group->setExclusive(true);

  addResizeModeAction(menu_column, section, QHeaderView::ResizeMode::Interactive, tr("Interactive"), group);
  addResizeModeAction(menu_column, section, QHeaderView::ResizeMode::Stretch, tr("Stretch"), group);
  addResizeModeAction(menu_column, section, QHeaderView::ResizeMode::ResizeToContents, tr("Resize to contents"), group);

  QAction* act_column = addMenu(menu_column);

  if (section == m_highlightedSection) {
    act_column->setIcon(style()->standardIcon(QStyle::StandardPixmap::SP_ArrowRight));
  }
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
