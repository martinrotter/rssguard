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

#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssserviceroot.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "miscellaneous/iconfactory.h"


FormEditAccount::FormEditAccount(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormEditAccount), m_editableRoot(NULL) {
  m_ui->setupUi(this);
  m_btnOk = m_ui->m_buttonBox->button(QDialogButtonBox::Ok);

  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("application-ttrss")));

  m_ui->m_txtPassword->lineEdit()->setPlaceholderText(tr("Password for your TT-RSS account."));
  m_ui->m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your TT-RSS account."));
  m_ui->m_txtUrl->lineEdit()->setPlaceholderText(tr("FULL URL of your TT-RSS instance WITH trailing \"/api/\" string."));
  m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Information,
                                   tr("No test done yet."),
                                   tr("Here, results of connection test are shown."));

  setTabOrder(m_ui->m_txtUrl->lineEdit(), m_ui->m_txtUsername->lineEdit());
  setTabOrder(m_ui->m_txtUsername->lineEdit(), m_ui->m_txtPassword->lineEdit());
  setTabOrder(m_ui->m_txtPassword->lineEdit(), m_ui->m_checkShowPassword);
  setTabOrder(m_ui->m_checkShowPassword, m_ui->m_btnTestSetup);
  setTabOrder(m_ui->m_btnTestSetup, m_ui->m_buttonBox);

  connect(m_ui->m_checkShowPassword, SIGNAL(toggled(bool)), this, SLOT(displayPassword(bool)));
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
  displayPassword(false);
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

void FormEditAccount::displayPassword(bool display) {
  m_ui->m_txtPassword->lineEdit()->setEchoMode(display ? QLineEdit::Normal : QLineEdit::Password);
}

void FormEditAccount::performTest() {
  TtRssNetworkFactory factory;

  factory.setUsername(m_ui->m_txtUsername->lineEdit()->text());
  factory.setPassword(m_ui->m_txtPassword->lineEdit()->text());
  factory.setUrl(m_ui->m_txtUrl->lineEdit()->text());

  LoginResult result = factory.login();

  if (result.first == QNetworkReply::NoError) {
    if (result.second.hasError()) {
      QString error = result.second.error();

      if (error == API_DISABLED) {
        m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Error,
                                         tr("API access on selected server is not enabled."),
                                         tr("API access on selected server is not enabled."));
      }
      else if (error == LOGIN_ERROR) {
        m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Error,
                                         tr("Entered credentials are incorrect."),
                                         tr("Entered credentials are incorrect."));
      }
      else {
        m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Error,
                                         tr("Other error occurred, contact developers."),
                                         tr("Other error occurred, contact developers."));
      }
    }
    else if (result.second.apiLevel() < MINIMAL_API_LEVEL) {
      m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Error,
                                       tr("Selected Tiny Tiny RSS server is running unsupported version of API (%1). At least API level %2 is required.").arg(QString::number(result.second.apiLevel()),
                                                                                                                                                              QString::number(MINIMAL_API_LEVEL)),
                                       tr("Selected Tiny Tiny RSS server is running unsupported version of API."));
    }
    else {
      m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Ok,
                                       tr("Tiny Tiny RSS server is okay, running with API level %1, while at least API level %2 is required.").arg(QString::number(result.second.apiLevel()),
                                                                                                                                                   QString::number(MINIMAL_API_LEVEL)),
                                       tr("Tiny Tiny RSS server is okay."));
    }
  }
  else {
    m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Error,
                                     tr("Network error, have you entered correct Tiny Tiny RSS API endpoint?"),
                                     tr("Network error, have you entered correct Tiny Tiny RSS API endpoint?"));
  }
}

void FormEditAccount::onClickedOk() {
  if (m_editableRoot == NULL) {
    // We want to confirm newly created account.
    // So save new account into DB, setup its properties.
    m_editableRoot = new TtRssServiceRoot(false);
  }

  m_editableRoot->network()->setUrl(m_ui->m_txtUrl->lineEdit()->text());
  m_editableRoot->network()->setUsername(m_ui->m_txtUsername->lineEdit()->text());
  m_editableRoot->network()->setPassword(m_ui->m_txtPassword->lineEdit()->text());
  m_editableRoot->saveToDatabase();

  accept();
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
