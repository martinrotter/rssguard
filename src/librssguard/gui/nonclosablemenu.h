// For license of this file, see <project-root-folder>/LICENSE.md.

#include <QMenu>

#ifndef NONCLOSABLEMENU_H
#define NONCLOSABLEMENU_H

class NonClosableMenu : public QMenu {
  public:
    explicit NonClosableMenu(QWidget* parent = nullptr);
    explicit NonClosableMenu(const QString& title, QWidget* parent = nullptr);

  protected:
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
};

#endif // NONCLOSABLEMENU_H
