#ifndef CLOSEBUTTON_H
#define CLOSEBUTTON_H

#include <QToolButton>


class CloseButton : public QToolButton {
    Q_OBJECT

  public:
    // Contructors and destructors.
    explicit CloseButton(QWidget *parent = 0);
    virtual ~CloseButton();

  protected:
    // Custom look.
    void paintEvent(QPaintEvent *e);
};

#endif // CLOSEBUTTON_H
