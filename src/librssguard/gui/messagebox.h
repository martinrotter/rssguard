// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <functional>

#include <QDialogButtonBox>
#include <QMessageBox>

class RSSGUARD_DLLSPEC MsgBox : public QMessageBox {
    Q_OBJECT

  public:
    struct CustomBoxAction {
        QString m_title;
        std::function<void()> m_function;
    };

    // Displays custom message box.
    static QMessageBox::StandardButton show(QWidget* parent,
                                            QMessageBox::Icon icon,
                                            const QString& title,
                                            const QString& text,
                                            const QString& informative_text = QString(),
                                            const QString& detailed_text = QString(),
                                            QMessageBox::StandardButtons buttons = QMessageBox::Ok,
                                            QMessageBox::StandardButton default_button = QMessageBox::Ok,
                                            const QString& dont_show_again_id = {},
                                            const QList<CustomBoxAction>& custom_actions = {});
    static QIcon iconForStatus(QMessageBox::Icon status);

  private:
    explicit MsgBox(QWidget* parent = nullptr);

    static bool isDontShowAgain(const QString& dont_show_again_id);
    static void setDontShowAgain(const QString& dont_show_again_id, bool dont_show_again);

    void setIcon(Icon icon);
    void setCheckBox(const QString& text, bool* data);
};

#endif // MESSAGEBOX_H
