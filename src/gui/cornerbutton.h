#ifndef CORNERBUTTON_H
#define CORNERBUTTON_H

#include <QToolButton>


class CornerButton : public QToolButton {
    Q_OBJECT

  public:
    // Contructors and destructors.
    explicit CornerButton(QWidget *parent = 0);
    virtual ~CornerButton();
};

#endif // CORNERBUTTON_H
