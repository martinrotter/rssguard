// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef BASETREEVIEW_H
#define BASETREEVIEW_H

#include <QTreeView>

class BaseTreeView : public QTreeView {
    Q_OBJECT

  public:
    explicit BaseTreeView(QWidget* parent = nullptr);

    bool isIndexHidden(const QModelIndex& idx) const;

  protected:
    virtual void keyPressEvent(QKeyEvent* event);

  private:
    QList<int> m_allowedKeyboardKeys;
};

#endif // BASETREEVIEW_H
