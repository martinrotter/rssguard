// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef BASETREEVIEW_H
#define BASETREEVIEW_H

#include <QElapsedTimer>
#include <QTreeView>

class RSSGUARD_DLLSPEC BaseTreeView : public QTreeView {
    Q_OBJECT

  public:
    explicit BaseTreeView(QWidget* parent = nullptr);

    bool isIndexHidden(const QModelIndex& idx) const;

  protected:
    virtual void wheelEvent(QWheelEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);

  private:
    QList<int> m_allowedKeyboardKeys;
    QElapsedTimer m_scrollingTimer;
    qint64 m_lastWheelTime;
    double m_scrollSpeedFactor;
};

#endif // BASETREEVIEW_H
