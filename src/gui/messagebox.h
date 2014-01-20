#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QMessageBox>


class MessageBox {
  private:
    // Constructors and destructors.
    explicit MessageBox();

  public:
    // TODO: tudle metodu udelat private a udelat public
    // metody information, warning atd a ty budou tudle volat
    // se spravnejma parametrama
    static QMessageBox::StandardButton showMessageBox(QWidget *parent,
                                                      QMessageBox::Icon icon,
                                                      const QString& title, const QString& text,
                                                      QMessageBox::StandardButtons buttons,
                                                      QMessageBox::StandardButton defaultButton);
};

#endif // MESSAGEBOX_H
