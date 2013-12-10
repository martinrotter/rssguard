#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QMessageBox>


class MessageBox : public QMessageBox {
    Q_OBJECT

  public:
    explicit MessageBox(QWidget *parent = 0);
    virtual ~MessageBox();

  signals:

  public slots:

};

#endif // MESSAGEBOX_H
