// Substitute only the user's modal decision; exercise the real settings code.
#ifndef SHORTCUT_TEST_MESSAGEBOX_H
#define SHORTCUT_TEST_MESSAGEBOX_H

#include <functional>

#include <QMessageBox>

class MsgBox {
  public:
    inline static std::function<QMessageBox::StandardButton(QWidget*, const QString&)> decision;

    static QMessageBox::StandardButton show(QWidget* parent,
                                            QMessageBox::Icon,
                                            const QString&,
                                            const QString&,
                                            const QString&,
                                            const QString& details,
                                            QMessageBox::StandardButtons,
                                            QMessageBox::StandardButton) {
      return decision(parent, details);
    }
};

#endif
