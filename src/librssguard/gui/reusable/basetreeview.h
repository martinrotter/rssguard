// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef BASETREEVIEW_H
#define BASETREEVIEW_H

#include <QByteArray>
#include <QElapsedTimer>
#include <QList>
#include <QPair>
#include <QTreeView>

class QContextMenuEvent;
class QPoint;
class TreeViewColumnsMenu;

class RSSGUARD_DLLSPEC BaseTreeView : public QTreeView {
    Q_OBJECT

  public:
    using ColumnSortStates = QList<QPair<int, Qt::SortOrder>>;

    explicit BaseTreeView(QWidget* parent = nullptr);

    bool isIndexHidden(const QModelIndex& idx) const;
    virtual QByteArray saveHeaderState() const;
    virtual void restoreHeaderState(const QByteArray& dta);

  protected:
    virtual void wheelEvent(QWheelEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);

    void displayColumnsContextMenu(const QPoint& global_pos, int highlighted_section = -1);
    virtual bool addColumnsContextMenuItems(TreeViewColumnsMenu* menu, int highlighted_section);
    virtual ColumnSortStates columnSortStates() const;
    virtual void restoreColumnSortStates(const ColumnSortStates& states);

  private:
    QList<int> m_allowedKeyboardKeys;
    QElapsedTimer m_scrollingTimer;
    qint64 m_lastWheelTime;
    double m_scrollSpeedFactor;
};

#endif // BASETREEVIEW_H
