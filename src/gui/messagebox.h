#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QMessageBox>


class MessageBox : public QMessageBox {
    Q_OBJECT

  public:
    explicit MessageBox(QWidget *parent = 0);
    virtual ~MessageBox();

    // TODO: http://libqxt.bitbucket.org/doc/0.6/qxtconfirmationmessage.html

  signals:

  public slots:

};

#endif // MESSAGEBOX_H
