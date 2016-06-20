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


#include "services/tt-rss/gui/formeditaccount.h"

#include "services/tt-rss/definitions.h"
#include "services/tt-rss/ttrssserviceroot.h"
#include "services/tt-rss/network/ttrssnetworkfactory.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/networkfactory.h"


FormEditAccount::FormEditAccount(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::FormEditAccount), m_editableRoot(nullptr) {
  m_ui->setupUi(this);
  m_btnOk = m_ui->m_buttonBox->button(QDialogButtonBox::Ok);

  setWindowFlags(Qt::MSWindowsFixedSizeDialogHint | Qt::Dialog | Qt::WindowSystemMenuHint);
  setWindowIcon(qApp->icons()->fromTheme(QSL("tinytinyrss")));

  m_ui->m_lblServerSideUpdateInformation->setText(tr("Leaving this option on causes that updates "
                                                     "of feeds will be probably much slower and may time-out often."));
  m_ui->m_lblDescription->setText(tr("Note that at least API level %1 is required.").arg(MINIMAL_API_LEVEL));
  m_ui->m_txtHttpUsername->lineEdit()->setPlaceholderText(tr("HTTP authentication username"));
  m_ui->m_txtHttpPassword->lineEdit()->setPlaceholderText(tr("HTTP authentication password"));
  m_ui->m_txtPassword->lineEdit()->setPlaceholderText(tr("Password for your TT-RSS account"));
  m_ui->m_txtUsername->lineEdit()->setPlaceholderText(tr("Username for your TT-RSS account"));
  m_ui->m_txtUrl->lineEdit()->setPlaceholderText(tr("FULL URL of your TT-RSS instance WITH trailing \"/api/\" string"));
  m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Information,
                                   tr("No test done yet."),
                                   tr("Here, results of connection test are shown."));

  setTabOrder(m_ui->m_txtUrl->lineEdit(), m_ui->m_checkServerSideUpdate);
  setTabOrder(m_ui->m_checkServerSideUpdate, m_ui->m_txtUsername->lineEdit());
  setTabOrder(m_ui->m_txtUsername->lineEdit(), m_ui->m_txtPassword->lineEdit());
  setTabOrder(m_ui->m_txtPassword->lineEdit(), m_ui->m_checkShowPassword);
  setTabOrder(m_ui->m_checkShowPassword, m_ui->m_gbHttpAuthentication);
  setTabOrder(m_ui->m_gbHttpAuthentication, m_ui->m_txtHttpUsername->lineEdit());
  setTabOrder(m_ui->m_txtHttpUsername->lineEdit(), m_ui->m_txtHttpPassword->lineEdit());
  setTabOrder(m_ui->m_txtHttpPassword->lineEdit(), m_ui->m_checkShowHttpPassword);
  setTabOrder(m_ui->m_checkShowHttpPassword, m_ui->m_btnTestSetup);
  setTabOrder(m_ui->m_btnTestSetup, m_ui->m_buttonBox);

  connect(m_ui->m_checkShowPassword, SIGNAL(toggled(bool)), this, SLOT(displayPassword(bool)));
  connect(m_ui->m_buttonBox, SIGNAL(accepted()), this, SLOT(onClickedOk()));
  connect(m_ui->m_buttonBox, SIGNAL(rejected()), this, SLOT(onClickedCancel()));
  connect(m_ui->m_txtPassword->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(onPasswordChanged()));
  connect(m_ui->m_txtUsername->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(onUsernameChanged()));
  connect(m_ui->m_txtHttpPassword->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(onHttpPasswordChanged()));
  connect(m_ui->m_txtHttpUsername->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(onHttpUsernameChanged()));
  connect(m_ui->m_txtUrl->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(onUrlChanged()));
  connect(m_ui->m_txtPassword->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(checkOkButton()));
  connect(m_ui->m_txtUsername->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(checkOkButton()));
  connect(m_ui->m_txtUrl->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(checkOkButton()));
  connect(m_ui->m_btnTestSetup, SIGNAL(clicked()), this, SLOT(performTest()));
  connect(m_ui->m_gbHttpAuthentication, SIGNAL(toggled(bool)), this, SLOT(onHttpPasswordChanged()));
  connect(m_ui->m_gbHttpAuthentication, SIGNAL(toggled(bool)), this, SLOT(onHttpUsernameChanged()));
  connect(m_ui->m_checkShowHttpPassword, SIGNAL(toggled(bool)), this, SLOT(displayHttpPassword(bool)));

  onPasswordChanged();
  onUsernameChanged();
  onUrlChanged();
  onHttpPasswordChanged();
  onHttpUsernameChanged();
  checkOkButton();
  displayPassword(false);
  displayHttpPassword(false);
}

FormEditAccount::~FormEditAccount() {
}

TtRssServiceRoot *FormEditAccount::execForCreate() {
  setWindowTitle(tr("Add new Tiny Tiny RSS account"));
  exec();
  return m_editableRoot;
}

void FormEditAccount::execForEdit(TtRssServiceRoot *existing_root) {
  setWindowTitle(tr("Edit existing Tiny Tiny RSS account"));
  m_editableRoot = existing_root;

  m_ui->m_gbHttpAuthentication->setChecked(existing_root->network()->authIsUsed());
  m_ui->m_txtHttpPassword->lineEdit()->setText(existing_root->network()->authPassword());
  m_ui->m_txtHttpUsername->lineEdit()->setText(existing_root->network()->authUsername());
  m_ui->m_txtUsername->lineEdit()->setText(existing_root->network()->username());
  m_ui->m_txtPassword->lineEdit()->setText(existing_root->network()->password());
  m_ui->m_txtUrl->lineEdit()->setText(existing_root->network()->url());
  m_ui->m_checkServerSideUpdate->setChecked(existing_root->network()->forceServerSideUpdate());

  exec();
}

void FormEditAccount::displayPassword(bool display) {
  m_ui->m_txtPassword->lineEdit()->setEchoMode(display ? QLineEdit::Normal : QLineEdit::Password);
}

void FormEditAccount::displayHttpPassword(bool display) {
  m_ui->m_txtHttpPassword->lineEdit()->setEchoMode(display ? QLineEdit::Normal : QLineEdit::Password);
}

void FormEditAccount::performTest() {
  TtRssNetworkFactory factory;

  factory.setUsername(m_ui->m_txtUsername->lineEdit()->text());
  factory.setPassword(m_ui->m_txtPassword->lineEdit()->text());
  factory.setUrl(m_ui->m_txtUrl->lineEdit()->text());
  factory.setAuthIsUsed(m_ui->m_gbHttpAuthentication->isChecked());
  factory.setAuthUsername(m_ui->m_txtHttpUsername->lineEdit()->text());
  factory.setAuthPassword(m_ui->m_txtHttpPassword->lineEdit()->text());
  factory.setForceServerSideUpdate(m_ui->m_checkServerSideUpdate->isChecked());

  TtRssLoginResponse result = factory.login();

  if (result.isLoaded()) {
    if (result.hasError()) {
      QString error = result.error();

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
    else if (result.apiLevel() < MINIMAL_API_LEVEL) {
      m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Error,
                                       tr("Selected Tiny Tiny RSS server is running unsupported version of API (%1). At least API level %2 is required.").arg(QString::number(result.apiLevel()),
                                                                                                                                                              QString::number(MINIMAL_API_LEVEL)),
                                       tr("Selected Tiny Tiny RSS server is running unsupported version of API."));
    }
    else {
      m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Ok,
                                       tr("Tiny Tiny RSS server is okay, running with API level %1, while at least API level %2 is required.").arg(QString::number(result.apiLevel()),
                                                                                                                                                   QString::number(MINIMAL_API_LEVEL)),
                                       tr("Tiny Tiny RSS server is okay."));
    }
  }
  else if (factory.lastError()  != QNetworkReply::NoError ) {
    m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Error,
                                     tr("Network error: '%1'.").arg(NetworkFactory::networkErrorText(factory.lastError())),
                                     tr("Network error, have you entered correct Tiny Tiny RSS API endpoint and password?"));
  }
  else {
    m_ui->m_lblTestResult->setStatus(WidgetWithStatus::Error,
                                     tr("Unspecified error, did you enter correct URL?"),
                                     tr("Unspecified error, did you enter correct URL?"));
  }
}

void FormEditAccount::onClickedOk() {
  bool editing_account = true;

  if (m_editableRoot == nullptr) {
    // We want to confirm newly created account.
    // So save new account into DB, setup its properties.
    m_editableRoot = new TtRssServiceRoot();
    editing_account = false;
  }

  m_editableRoot->network()->setUrl(m_ui->m_txtUrl->lineEdit()->text());
  m_editableRoot->network()->setUsername(m_ui->m_txtUsername->lineEdit()->text());
  m_editableRoot->network()->setPassword(m_ui->m_txtPassword->lineEdit()->text());
  m_editableRoot->network()->setAuthIsUsed(m_ui->m_gbHttpAuthentication->isChecked());
  m_editableRoot->network()->setAuthUsername(m_ui->m_txtHttpUsername->lineEdit()->text());
  m_editableRoot->network()->setAuthPassword(m_ui->m_txtHttpPassword->lineEdit()->text());
  m_editableRoot->network()->setForceServerSideUpdate(m_ui->m_checkServerSideUpdate->isChecked());
  m_editableRoot->saveAccountDataToDatabase();

  accept();

  if (editing_account) {
    m_editableRoot->network()->logout();
    m_editableRoot->completelyRemoveAllData();
    m_editableRoot->syncIn();
  }
}

void FormEditAccount::onClickedCancel() {
  reject();
}

void FormEditAccount::onUsernameChanged() {
  const QString username = m_ui->m_txtUsername->lineEdit()->text();

  if (username.isEmpty()) {
    m_ui->m_txtUsername->setStatus(WidgetWithStatus::Error, tr("Username cannot be empty."));
  }
  else {
    m_ui->m_txtUsername->setStatus(WidgetWithStatus::Ok, tr("Username is okay."));
  }
}

void FormEditAccount::onPasswordChanged() {
  const QString password = m_ui->m_txtPassword->lineEdit()->text();

  if (password.isEmpty()) {
    m_ui->m_txtPassword->setStatus(WidgetWithStatus::Error, tr("Password cannot be empty."));
  }
  else {
    m_ui->m_txtPassword->setStatus(WidgetWithStatus::Ok, tr("Password is okay."));
  }
}

void FormEditAccount::onHttpUsernameChanged() {
  const  bool is_username_ok = !m_ui->m_gbHttpAuthentication->isChecked() || !m_ui->m_txtHttpUsername->lineEdit()->text().isEmpty();

  m_ui->m_txtHttpUsername->setStatus(is_username_ok ?
                                       LineEditWithStatus::Ok :
                                       LineEditWithStatus::Warning,
                                     is_username_ok ?
                                       tr("Username is ok or it is not needed.") :
                                       tr("Username is empty."));
}

void FormEditAccount::onHttpPasswordChanged() {
  const bool is_username_ok = !m_ui->m_gbHttpAuthentication->isChecked() || !m_ui->m_txtHttpPassword->lineEdit()->text().isEmpty();

  m_ui->m_txtHttpPassword->setStatus(is_username_ok ?
                                       LineEditWithStatus::Ok :
                                       LineEditWithStatus::Warning,
                                     is_username_ok ?
                                       tr("Password is ok or it is not needed.") :
                                       tr("Password is empty."));
}

void FormEditAccount::onUrlChanged() {
  const QString url = m_ui->m_txtUrl->lineEdit()->text();

  if (url.isEmpty()) {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::Error, tr("URL cannot be empty."));
  }
  else if (!url.endsWith(QL1S("/api/"))) {
    m_ui->m_txtUrl->setStatus(WidgetWithStatus::Warning, tr("URL should end with \"/api/\"."));
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
