// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TREEVIEWCOLUMNSMENU_H
#define TREEVIEWCOLUMNSMENU_H

#include <QMenu>

class QHeaderView;

class TreeViewColumnsMenu : public QMenu {
  public:
    explicit TreeViewColumnsMenu(QHeaderView* parent);
    virtual ~TreeViewColumnsMenu();

  private slots:
    void prepareMenu();
    void actionTriggered(bool toggle);

  private:
    QHeaderView* header();
};

#endif // TREEVIEWCOLUMNSMENU_H
