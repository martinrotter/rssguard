// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QDialogButtonBox>
#include <QMessageBox>

#include <functional>

class MsgBox : public QMessageBox {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit MsgBox(QWidget* parent = nullptr);

    // Custom icon setting.
    void setIcon(Icon icon);

    static void setCheckBox(QMessageBox* msg_box, const QString& text, bool* data);

    // Displays custom message box.
    static QMessageBox::StandardButton show(QWidget* parent,
                                            QMessageBox::Icon icon,
                                            const QString& title,
                                            const QString& text,
                                            const QString& informative_text = QString(),
                                            const QString& detailed_text = QString(),
                                            QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                            QMessageBox::StandardButton default_button = QMessageBox::Ok,
                                            bool* dont_show_again = nullptr,
                                            const QString& functor_heading = {},
                                            const std::function<void()>& functor = nullptr);
    static QIcon iconForStatus(QMessageBox::Icon status);
};

#endif // MESSAGEBOX_H
