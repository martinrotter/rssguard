// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagebox.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStyle>
#include <QtGlobal>

MessageBox::MessageBox(QWidget* parent) : QMessageBox(parent) {}

MessageBox::~MessageBox() {}

void MessageBox::setIcon(QMessageBox::Icon icon) {
  // Determine correct status icon size.
  const int icon_size = qApp->style()->pixelMetric(QStyle::PM_MessageBoxIconSize, 0, this);

  // Setup status icon.
  setIconPixmap(iconForStatus(icon).pixmap(icon_size, icon_size));
}

void MessageBox::setCheckBox(QMessageBox* msg_box, const QString& text, bool* data) {
  // Add "don't show this again checkbox.
  QCheckBox* check_box = new QCheckBox(msg_box);

  check_box->setChecked(*data);
  check_box->setText(text);
  connect(check_box, &QCheckBox::toggled, [=](bool checked) {
    *data = checked;
  });
  msg_box->setCheckBox(check_box);
}

QIcon MessageBox::iconForStatus(QMessageBox::Icon status) {
  switch (status) {
    case QMessageBox::Information:
      return qApp->icons()->fromTheme(QSL("dialog-information"));

    case QMessageBox::Warning:
      return qApp->icons()->fromTheme(QSL("dialog-warning"));

    case QMessageBox::Critical:
      return qApp->icons()->fromTheme(QSL("dialog-error"));

    case QMessageBox::Question:
      return qApp->icons()->fromTheme(QSL("dialog-question"));

    case QMessageBox::NoIcon:
    default:
      return QIcon();
  }
}

QMessageBox::StandardButton MessageBox::show(QWidget* parent,
                                             QMessageBox::Icon icon,
                                             const QString& title,
                                             const QString& text,
                                             const QString& informative_text,
                                             const QString& detailed_text,
                                             QMessageBox::StandardButtons buttons,
                                             QMessageBox::StandardButton default_button,
                                             bool* dont_show_again) {
  // Create and find needed components.
  MessageBox msg_box(parent);

  // Initialize message box properties.
  msg_box.setWindowTitle(title);
  msg_box.setText(text);
  msg_box.setInformativeText(informative_text);
  msg_box.setDetailedText(detailed_text);
  msg_box.setIcon(icon);
  msg_box.setStandardButtons(buttons);
  msg_box.setDefaultButton(default_button);

  if (dont_show_again != nullptr) {
    MessageBox::setCheckBox(&msg_box, tr("Do not show this dialog again."), dont_show_again);
  }

  // Display it.
  if (msg_box.exec() == -1) {
    return QMessageBox::Cancel;
  }
  else {
    return msg_box.standardButton(msg_box.clickedButton());
  }
}
