#include "gui/messagebox.h"

#include <QDialogButtonBox>
#include <QtGlobal>
#include <QPushButton>


MessageBox::MessageBox() {
}

QMessageBox::StandardButton MessageBox::showMessageBox(QWidget *parent,
                                                          QMessageBox::Icon icon,
                                                          const QString &title,
                                                          const QString &text,
                                                          QMessageBox::StandardButtons buttons,
                                                          QMessageBox::StandardButton defaultButton) {
  QMessageBox msgBox(icon, title, text, QMessageBox::NoButton, parent);
  QDialogButtonBox *buttonBox = msgBox.findChild<QDialogButtonBox*>();

  uint mask = QMessageBox::FirstButton;
  while (mask <= QMessageBox::LastButton) {
    uint sb = buttons & mask;
    mask <<= 1;
    if (!sb)
      continue;

    // TODO: tady podle hodnoty masky switchnout prave pridanej button a podle toho mu dat ikonu.
    // neco jako
    switch (mask) {
      case QMessageBox::Ok:
        // TODO: nastav ikonu "ok"
        break;
      default:
        break;
    }

    QPushButton *button = msgBox.addButton((QMessageBox::StandardButton)sb);

    // Choose the first accept role as the default
    if (msgBox.defaultButton())
      continue;
    if ((defaultButton == QMessageBox::NoButton && buttonBox->buttonRole(button) == QDialogButtonBox::AcceptRole)
        || (defaultButton != QMessageBox::NoButton && sb == uint(defaultButton)))
      msgBox.setDefaultButton(button);
  }
  if (msgBox.exec() == -1)
    return QMessageBox::Cancel;
  return msgBox.standardButton(msgBox.clickedButton());
}
