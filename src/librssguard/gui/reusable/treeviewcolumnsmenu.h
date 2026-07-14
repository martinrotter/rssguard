// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TREEVIEWCOLUMNSMENU_H
#define TREEVIEWCOLUMNSMENU_H

#include "gui/reusable/nonclosablemenu.h"

#include <functional>

#include <QHeaderView>

class QAction;
class QActionGroup;

class TreeViewColumnsMenu : public NonClosableMenu {
    Q_OBJECT

  public:
    using MenuExtensionBuilder = std::function<bool(TreeViewColumnsMenu*)>;

    explicit TreeViewColumnsMenu(QHeaderView* parent, int highlighted_section = -1);

    void setMenuExtensionBuilder(const MenuExtensionBuilder& builder);

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

    MenuExtensionBuilder m_menuExtensionBuilder;
    int m_highlightedSection;
};

#endif // TREEVIEWCOLUMNSMENU_H
