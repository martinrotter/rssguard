#ifndef SHORTCUTBUTTON_H
#define SHORTCUTBUTTON_H

#include <QPushButton>


class ShortcutCatcher;

class ShortcutButton : public QPushButton {
    Q_OBJECT
  public:
    explicit ShortcutButton(ShortcutCatcher *catcher, QWidget *parent = 0);
    virtual ~ShortcutButton();

  protected:
    void keyPressEvent(QKeyEvent *event);
    void keyReleaseEvent(QKeyEvent *event);

  private:
    ShortcutCatcher *m_catcher;
};

#endif // SHORTCUTBUTTON_H
