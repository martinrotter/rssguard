#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QMessageBox>
#include <QDialogButtonBox>


class MessageBox : public QMessageBox {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit MessageBox(QWidget *parent = 0);
    virtual ~MessageBox();

    // Custom icon setting.
    void setIcon(Icon icon);

    // Performs icon replacements for given button box.
    static void iconify(QDialogButtonBox *button_box);

    // Displays custom message box.
    static QMessageBox::StandardButton show(QWidget *parent,
                                            QMessageBox::Icon icon,
                                            const QString &title,
                                            const QString &text,
                                            const QString &informative_text = QString(),
                                            const QString &detailed_text = QString(),
                                            QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                            QMessageBox::StandardButton default_button = QMessageBox::Ok);

  private:
    // Returns icons for standard roles/statuses.
    static QIcon iconForRole(QDialogButtonBox::StandardButton button);
    static QIcon iconForStatus(QMessageBox::Icon status);
};

#endif // MESSAGEBOX_H
