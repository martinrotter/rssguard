// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "gui/messagepreviewer.h"

#include "miscellaneous/application.h"
#include "network-web/webfactory.h"
#include "gui/messagebox.h"
#include "gui/dialogs/formmain.h"

#include <QScrollBar>


MessagePreviewer::MessagePreviewer(QWidget *parent) : QWidget(parent),
  m_ui(new Ui::MessagePreviewer) {
  m_ui->setupUi(this);
  m_ui->m_txtMessage->viewport()->setAutoFillBackground(true);

  connect(m_ui->m_txtMessage, &QTextBrowser::anchorClicked, [=](const QUrl &url) {
    // User clicked some URL. Open it in external browser or download?
    MessageBox box(qApp->mainForm());

    box.setText(tr("You clicked link \"%1\". You can download the link contents or open it in external web browser.").arg(url.toString()));
    box.setInformativeText(tr("What action do you want to take?"));
    QAbstractButton *btn_open = box.addButton(tr("Open in external browser"), QMessageBox::AcceptRole);
    QAbstractButton *btn_download = box.addButton(tr("Download"), QMessageBox::RejectRole);

    box.exec();

    if (box.clickedButton() == btn_open) {
      WebFactory::instance()->openUrlInExternalBrowser(url.toString());
    }
    else if (box.clickedButton() == btn_download) {
      qApp->downloadManager()->download(url);
    }
  });

  clear();
}


MessagePreviewer::~MessagePreviewer() {
  delete m_ui;
}

void MessagePreviewer::clear() {
  m_ui->m_lblTitle->clear();
  m_ui->m_txtMessage->clear();

  hide();
}

void MessagePreviewer::loadMessage(const Message &message) {
  m_ui->m_lblTitle->setText(message.m_title);
  m_ui->m_txtMessage->setHtml(prepareHtmlForMessage(message));
  show();

  m_ui->m_txtMessage->verticalScrollBar()->triggerAction(QScrollBar::SliderToMinimum);
}

QString MessagePreviewer::prepareHtmlForMessage(const Message &message) {
  QString html = QString("<p><a href=\"%1\">%1</a><p/>").arg(message.m_url);

  foreach (const Enclosure &enc, message.m_enclosures) {
    html += QString("<p>[%2] <a href=\"%1\">%1</a><p/>").arg(enc.m_url, enc.m_mimeType);
  }

  if (!message.m_enclosures.isEmpty()) {
    html += "<hr/>";
  }

  html += message.m_contents;

  return html;
}
