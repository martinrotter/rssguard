// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/messagebox.h"

#include "gui/iconthemefactory.h"

#include <QDialogButtonBox>
#include <QtGlobal>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QStyle>
#include <QApplication>

MessageBox::MessageBox(QWidget *parent) : QMessageBox(parent) {
}

MessageBox::~MessageBox() {
}

void MessageBox::setIcon(QMessageBox::Icon icon) {
  // Determine correct status icon size.
  int icon_size = qApp->style()->pixelMetric(QStyle::PM_MessageBoxIconSize,
                                             0,
                                             this);
  // Setup status icon.
  setIconPixmap(iconForStatus(icon).pixmap(icon_size,
                                           icon_size));
}

void MessageBox::iconify(QDialogButtonBox *button_box) {
  foreach (QAbstractButton *button, button_box->buttons()) {
    button->setIcon(iconForRole(button_box->standardButton(button)));
  }
}

QIcon MessageBox::iconForRole(QDialogButtonBox::StandardButton button) {
  switch (button) {
    case QMessageBox::Ok:
      return IconThemeFactory::instance()->fromTheme("dialog-ok");

    case QMessageBox::Cancel:
    case QMessageBox::Close:
      return IconThemeFactory::instance()->fromTheme("dialog-cancel");

    case QMessageBox::Yes:
    case QMessageBox::YesToAll:
      return IconThemeFactory::instance()->fromTheme("dialog-yes");

    case QMessageBox::No:
    case QMessageBox::NoToAll:
      return IconThemeFactory::instance()->fromTheme("dialog-no");

    case QMessageBox::Help:
      return IconThemeFactory::instance()->fromTheme("dialog-question");

    default:
      return QIcon();
  }
}

QIcon MessageBox::iconForStatus(QMessageBox::Icon status) {
  switch (status) {
    case QMessageBox::Information:
      return IconThemeFactory::instance()->fromTheme("dialog-information");

    case QMessageBox::Warning:
      return IconThemeFactory::instance()->fromTheme("dialog-warning");

    case QMessageBox::Critical:
      return IconThemeFactory::instance()->fromTheme("dialog-error");

    case QMessageBox::Question:
      return IconThemeFactory::instance()->fromTheme("dialog-question");

    case QMessageBox::NoIcon:
    default:
      return QIcon();
  }
}

QMessageBox::StandardButton MessageBox::show(QWidget *parent,
                                             QMessageBox::Icon icon,
                                             const QString &title,
                                             const QString &text,
                                             const QString &informative_text,
                                             const QString &detailed_text,
                                             QMessageBox::StandardButtons buttons,
                                             QMessageBox::StandardButton default_button) {
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

  // Setup button box icons.
#if !defined(Q_OS_WIN)
  QDialogButtonBox *button_box = msg_box.findChild<QDialogButtonBox*>();
  iconify(button_box);
#endif

  // Display it.
  if (msg_box.exec() == -1) {
    return QMessageBox::Cancel;
  }
  else {
    return msg_box.standardButton(msg_box.clickedButton());
  }
}
