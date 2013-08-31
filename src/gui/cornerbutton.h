#ifndef CORNERBUTTON_H
#define CORNERBUTTON_H

#include <QPushButton>


class CornerButton : public QPushButton {
    Q_OBJECT

  public:
    explicit CornerButton(QWidget *parent = 0);
    virtual ~CornerButton();

  signals:

  public slots:
};

#endif // CORNERBUTTON_H
