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

#include "gui/formwelcome.h"

#include "core/defs.h"

#if !defined(Q_OS_WIN)
#include "gui/messagebox.h"
#endif

#include <QDesktopServices>
#include <QUrl>
#include <QLabel>
#include <QDesktopWidget>


FormWelcome::FormWelcome(QWidget *parent) : QDialog(parent), m_ui(new Ui::FormWelcome) {
  m_ui->setupUi(this);

  // Set flags.
  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog);

#if !defined(Q_OS_WIN)
  MessageBox::iconify(m_ui->m_buttonBox);
#endif

  // Set icon.
  setWindowIcon(QIcon(APP_ICON_PATH));
  m_ui->m_lblLogo->setPixmap(QPixmap(APP_ICON_PATH));

  // Move the dialog into the middle of the screen.
  QRect screen = qApp->desktop()->screenGeometry();
  move(screen.center() - rect().center());

  // Make sure that clicked hyperlinks are opened in defult web browser.
  connect(m_ui->m_lblInfo, SIGNAL(linkActivated(QString)),
          this, SLOT(openLink(QString)));

  // Setup the text.
  m_ui->m_lblInfo->setText(
        tr("<p>RSS Guard is a (very) easy-to-use feed reader. "
           "It supports all major feed formats, including RSS, "
           "ATOM and RDF.</p>"
           "<p>Make sure you explore all available features. "
           "If you find a bug or if you want to propose new "
           "feature, then create new "
           "<a href=\"%1\"><span "
           "style=\"text-decoration: underline; color:#0000ff;\">issue "
           "report</span></a>.</p>"
           "<p>RSS Guard can be translated to any language. "
           "Contact its <a href=\"mailto:%2\"><span "
           "style=\"text-decoration: underline; color:#0000ff;\">author</span></a> "
           "in case of your interest.</p><p><br/></p>").arg(APP_URL_ISSUES,
                                                            APP_EMAIL));
}

void FormWelcome::openLink(const QString &link) {
  QDesktopServices::openUrl(QUrl(link));
}

FormWelcome::~FormWelcome() {
  delete m_ui;
}
