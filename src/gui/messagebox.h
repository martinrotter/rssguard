#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QMessageBox>
#include <QDialogButtonBox>


class MessageBox {
  private:
    // Constructors and destructors.
    explicit MessageBox();

  public:
    // TODO: tudle metodu udelat private a udelat public
    // metody information, warning atd a ty budou tudle volat
    // se spravnejma parametrama
    // a napsat taky metodu iconifyMessageButtonBox(qmessabebuttonbox)
    // ktera nahraje do daneho boxu aktualni ikony

    // Performs icon replacements for given button box.
    static void iconify(QDialogButtonBox *button_box);

    // Returns icons for standard roles/statuses.
    static QIcon iconForRole(QDialogButtonBox::StandardButton button);
    static QIcon iconForStatus(QMessageBox::Icon status);

    static QMessageBox::StandardButton show(QWidget *parent,
                                            QMessageBox::Icon icon,
                                            const QString& title, const QString& text,
                                            QMessageBox::StandardButtons buttons,
                                            QMessageBox::StandardButton defaultButton);
};

#endif // MESSAGEBOX_H
