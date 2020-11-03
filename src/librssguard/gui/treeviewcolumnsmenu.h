// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TREEVIEWCOLUMNSMENU_H
#define TREEVIEWCOLUMNSMENU_H

#include "gui/nonclosablemenu.h"

class QHeaderView;

class TreeViewColumnsMenu : public NonClosableMenu {
  public:
    explicit TreeViewColumnsMenu(QHeaderView* parent);

  private slots:
    void prepareMenu();
    void actionTriggered(bool toggle);

  private:
    QHeaderView* header();
};

#endif // TREEVIEWCOLUMNSMENU_H
