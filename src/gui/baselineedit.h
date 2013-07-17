#ifndef BASELINEEDIT_H
#define BASELINEEDIT_H

#include <QLineEdit>


class BaseLineEdit : public QLineEdit {
    Q_OBJECT
  public:
    explicit BaseLineEdit(QWidget *parent = 0);
    virtual ~BaseLineEdit();
    
  protected:
    void keyPressEvent(QKeyEvent *event);

  signals:
    void submitted(const QString &text);
    
  public slots:
    
};

#endif // BASELINEEDIT_H
