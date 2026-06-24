// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TREEVIEWCOLUMNSMENU_H
#define TREEVIEWCOLUMNSMENU_H

#include "gui/reusable/nonclosablemenu.h"

#include <QHeaderView>

class QAction;
class QActionGroup;

class TreeViewColumnsMenu : public NonClosableMenu {
  public:
    explicit TreeViewColumnsMenu(QHeaderView* parent, int highlighted_section = -1);

  protected:
    virtual bool shouldActionClose(QAction* action) const override;

  private slots:
    void prepareMenu();
    void showHideColumn(bool toggle);
    void setStretchLastSection(bool stretch);
    void setCascadingSectionResizes(bool cascade);
    void autosizeColumn();
    void autosizeVisibleColumns();
    void setColumnResizeMode();

  private:
    void addColumnMenu(int section);
    void addResizeModeAction(QMenu* menu,
                             int section,
                             QHeaderView::ResizeMode mode,
                             const QString& title,
                             QActionGroup* group);

    QHeaderView* header();

    int m_highlightedSection;
};

#endif // TREEVIEWCOLUMNSMENU_H
