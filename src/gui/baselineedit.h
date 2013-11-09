#ifndef BASELINEEDIT_H
#define BASELINEEDIT_H

#include <QLineEdit>


class BaseLineEdit : public QLineEdit {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit BaseLineEdit(QWidget *parent = 0);
    virtual ~BaseLineEdit();

  protected:
    void keyPressEvent(QKeyEvent *event);

  signals:
    // Emitted if user hits ENTER button.
    void submitted(const QString &text);
};

#endif // BASELINEEDIT_H
