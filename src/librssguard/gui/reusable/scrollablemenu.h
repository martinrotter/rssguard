// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SCROLLABLEMENU_H
#define SCROLLABLEMENU_H

#include "gui/reusable/nonclosablemenu.h"

#include <QPointer>

class ScrollableMenu : public NonClosableMenu {
    Q_OBJECT

  public:
    explicit ScrollableMenu(QWidget* parent = nullptr);
    explicit ScrollableMenu(const QString& title, QWidget* parent = nullptr);

    void setActions(const QList<QAction*> actions, bool close_on_trigger);

  protected:
    virtual void wheelEvent(QWheelEvent* e) override;
    virtual bool shouldActionClose(QAction* action) const override;

  private:
    void clearCurrentPage();
    void setArrowsVisible(bool visible);
    void initialize();
    void setPageName(const QString& name);
    void setPage(int page);

    QPointer<QAction> m_actUp;
    QPointer<QAction> m_actDown;
    QList<QAction*> m_actions;
    bool m_actionsClosing = true;
    int m_page = -1;
    bool m_arrowsVisible = false;
};

#endif // SCROLLABLEMENU_H
