// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/messagebox.h"

#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStyle>
#include <QtGlobal>

MsgBox::MsgBox(QWidget* parent) : QMessageBox(parent) {}

bool MsgBox::isDontShowAgain(const QString& dont_show_again_id) {
  if (dont_show_again_id.isEmpty()) {
    return false;
  }

  return qApp->settings()->value(GROUP(MessageBoxDontShows), dont_show_again_id).toBool();
}

void MsgBox::setDontShowAgain(const QString& dont_show_again_id, bool dont_show_again) {
  if (!dont_show_again_id.isEmpty()) {
    qApp->settings()->setValue(GROUP(MessageBoxDontShows), dont_show_again_id, dont_show_again);
  }
}

void MsgBox::setIcon(QMessageBox::Icon icon) {
  // Determine correct status icon size.
  const int icon_size = qApp->style()->pixelMetric(QStyle::PixelMetric::PM_MessageBoxIconSize, nullptr, this);

  // Setup status icon.
  setIconPixmap(iconForStatus(icon).pixmap(icon_size, icon_size));
}

void MsgBox::setCheckBox(const QString& text, bool* data) {
  // Add "don't show this again checkbox.
  auto* check_box = new QCheckBox(this);

  check_box->setChecked(*data);
  check_box->setText(text);

  connect(check_box, &QCheckBox::toggled, [=](bool checked) {
    *data = checked;
  });

  QMessageBox::setCheckBox(check_box);
}

QIcon MsgBox::iconForStatus(QMessageBox::Icon status) {
  switch (status) {
    case QMessageBox::Icon::Information:
      return qApp->icons()->fromTheme(QSL("dialog-information"));

    case QMessageBox::Icon::Warning:
      return qApp->icons()->fromTheme(QSL("dialog-warning"));

    case QMessageBox::Icon::Critical:
      return qApp->icons()->fromTheme(QSL("dialog-error"));

    case QMessageBox::Icon::Question:
      return qApp->icons()->fromTheme(QSL("dialog-question"));

    default:
      return QIcon();
  }
}

QMessageBox::StandardButton MsgBox::show(QWidget* parent,
                                         QMessageBox::Icon icon,
                                         const QString& title,
                                         const QString& text,
                                         const QString& informative_text,
                                         const QString& detailed_text,
                                         QMessageBox::StandardButtons buttons,
                                         QMessageBox::StandardButton default_button,
                                         const QString& dont_show_again_id,
                                         const QList<CustomBoxAction>& custom_actions) {
  if (MsgBox::isDontShowAgain(dont_show_again_id)) {
    return default_button;
  }

  if (parent == nullptr) {
    parent = qApp->mainFormWidget();
  }

  // Create and find needed components.
  MsgBox msg_box(parent);

  // Initialize message box properties.
  msg_box.setWindowTitle(title);
  msg_box.setText(text);
  msg_box.setInformativeText(informative_text);
  msg_box.setDetailedText(detailed_text);
  msg_box.setIcon(icon);
  msg_box.setStandardButtons(buttons);
  msg_box.setDefaultButton(default_button);

  bool dont_show_again = false;

  if (!dont_show_again_id.isEmpty()) {
    msg_box.setCheckBox(tr("Do not show again"), &dont_show_again);
  }

  for (const CustomBoxAction& act : custom_actions) {
    connect(msg_box.addButton(act.m_title, QMessageBox::ButtonRole::HelpRole),
            &QPushButton::clicked,
            &msg_box,
            act.m_function);
  }

  auto dialog_res = msg_box.exec();

  if (dont_show_again) {
    MsgBox::setDontShowAgain(dont_show_again_id, dont_show_again);
  }

  if (dialog_res == -1) {
    return QMessageBox::StandardButton::Cancel;
  }
  else {
    return msg_box.standardButton(msg_box.clickedButton());
  }
}
