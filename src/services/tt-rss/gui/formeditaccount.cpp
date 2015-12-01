// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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


#include "services/tt-rss/gui/formeditaccount.h"

#include "services/tt-rss/ttrssserviceroot.h"


FormEditAccount::FormEditAccount(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormEditAccount), m_editableRoot(NULL) {
  m_ui->setupUi(this);
  m_btnOk = m_ui->m_buttonBox->button(QDialogButtonBox::Ok);

  m_ui->m_txtPassword->lineEdit()->setPlaceholderText(tr("Password for your TT-RSS account."));
  m_ui->m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your TT-RSS account."));
  m_ui->m_txtUrl->lineEdit()->setPlaceholderText(tr("URL of your TT-RSS instance WITHOUT trailing \"/api/\" string."));
  m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Information,
                                   tr("No test done yet."),
                                   tr("Here, results of connection test are shown."));

  connect(m_ui->m_buttonBox, SIGNAL(accepted()), this, SLOT(onClickedOk()));
  connect(m_ui->m_buttonBox, SIGNAL(rejected()), this, SLOT(onClickedCancel()));
  connect(m_ui->m_txtPassword->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(onPasswordChanged()));
  connect(m_ui->m_txtUsername->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(onUsernameChanged()));
  connect(m_ui->m_txtUrl->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(onUrlChanged()));
  connect(m_ui->m_txtPassword->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(checkOkButton()));
  connect(m_ui->m_txtUsername->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(checkOkButton()));
  connect(m_ui->m_txtUrl->lineEdit(), SIGNAL(textEdited(QString)), this, SLOT(checkOkButton()));
  connect(m_ui->m_btnTestSetup, SIGNAL(clicked()), this, SLOT(performTest()));

  onPasswordChanged();
  onUsernameChanged();
  onUrlChanged();
  checkOkButton();
}

FormEditAccount::~FormEditAccount() {
  delete m_ui;
}

TtRssServiceRoot *FormEditAccount::execForCreate() {
  setWindowTitle(tr("Add new Tiny Tiny RSS account"));
  exec();
  return m_editableRoot;
}

void FormEditAccount::execForEdit(TtRssServiceRoot *existing_root) {
  setWindowTitle(tr("Edit existing Tiny Tiny RSS account"));
  m_editableRoot = existing_root;
  exec();
}

void FormEditAccount::performTest() {

}

void FormEditAccount::onClickedOk() {
  if (m_editableRoot == NULL) {
    // We want to confirm newly created account.
  }
  else {
    // We want to edit existing account.
  }
}

void FormEditAccount::onClickedCancel() {
  reject();
}

void FormEditAccount::onUsernameChanged() {
  QString username = m_ui->m_txtUsername->lineEdit()->text();

  if (username.isEmpty()) {
    m_ui->m_txtUsername->setStatus(WidgetWithStatus::Error, tr("Username cannot be empty."));
  }
  else {
    m_ui->m_txtUsername->setStatus(WidgetWithStatus::Ok, tr("Username is okay."));
  }
}

void FormEditAccount::onPasswordChanged() {
  QString password = m_ui->m_txtPassword->lineEdit()->text();

  if (password.isEmpty()) {
    m_ui->m_txtPassword->setStatus(WidgetWithStatus::Error, tr("Password cannot be empty."));
  }
  else {
    m_ui->m_txtPassword->setStatus(WidgetWithStatus::Ok, tr("Password is okay."));
  }
}

void FormEditAccount::onUrlChanged() {
  QString url = m_ui->m_txtUrl->lineEdit()->text();

  if (url.isEmpty()) {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::Error, tr("URL cannot be empty."));
  }
  else {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::Ok, tr("URL is okay."));
  }
}

void FormEditAccount::checkOkButton() {
  m_btnOk->setEnabled(!m_ui->m_txtUsername->lineEdit()->text().isEmpty() &&
                      !m_ui->m_txtPassword->lineEdit()->text().isEmpty() &&
                      !m_ui->m_txtUrl->lineEdit()->text().isEmpty());
}
