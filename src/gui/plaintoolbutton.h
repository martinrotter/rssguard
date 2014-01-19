#ifndef CLOSEBUTTON_H
#define CLOSEBUTTON_H

#include <QToolButton>


class PlainToolButton : public QToolButton {
    Q_OBJECT

  public:
    // Contructors and destructors.
    explicit PlainToolButton(QWidget *parent = 0);
    virtual ~PlainToolButton();

  protected:
    // Custom look.
    void paintEvent(QPaintEvent *e);
};

#endif // CLOSEBUTTON_H
