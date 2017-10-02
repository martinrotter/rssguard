// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "services/inoreader/gui/formeditinoreaderaccount.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"
#include "network-web/oauth2service.h"
#include "services/inoreader/definitions.h"
#include "services/inoreader/inoreaderserviceroot.h"

FormEditInoreaderAccount::FormEditInoreaderAccount(QWidget* parent) : QDialog(parent),
  m_network(nullptr), m_editableRoot(nullptr) {
  m_ui.setupUi(this);

  GuiUtilities::setLabelAsNotice(*m_ui.m_lblAuthInfo, true);
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->miscIcon(QSL("inoreader")));

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("Not tested yet."),
                                  tr("Not tested yet."));
  m_ui.m_lblTestResult->label()->setWordWrap(true);
  m_ui.m_txtUsername->lineEdit()->setPlaceholderText(tr("User-visible username"));

  setTabOrder(m_ui.m_txtUsername->lineEdit(), m_ui.m_txtAppId);
  setTabOrder(m_ui.m_txtAppId, m_ui.m_txtAppKey);
  setTabOrder(m_ui.m_txtAppKey, m_ui.m_txtRedirectUrl);
  setTabOrder(m_ui.m_txtRedirectUrl, m_ui.m_spinLimitMessages);
  setTabOrder(m_ui.m_spinLimitMessages, m_ui.m_btnTestSetup);
  setTabOrder(m_ui.m_btnTestSetup, m_ui.m_buttonBox);

  connect(m_ui.m_txtAppId->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditInoreaderAccount::checkOAuthValue);
  connect(m_ui.m_txtAppKey->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditInoreaderAccount::checkOAuthValue);
  connect(m_ui.m_txtRedirectUrl->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditInoreaderAccount::checkOAuthValue);
  connect(m_ui.m_txtUsername->lineEdit(), &BaseLineEdit::textChanged, this, &FormEditInoreaderAccount::checkUsername);
  connect(m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditInoreaderAccount::testSetup);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::accepted, this, &FormEditInoreaderAccount::onClickedOk);
  connect(m_ui.m_buttonBox, &QDialogButtonBox::rejected, this, &FormEditInoreaderAccount::onClickedCancel);

  m_ui.m_spinLimitMessages->setValue(INOREADER_DEFAULT_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMinimum(INOREADER_MIN_BATCH_SIZE);
  m_ui.m_spinLimitMessages->setMaximum(INOREADER_MAX_BATCH_SIZE);

  checkUsername(m_ui.m_txtUsername->lineEdit()->text());
}

FormEditInoreaderAccount::~FormEditInoreaderAccount() {}

void FormEditInoreaderAccount::testSetup() {
  if (m_network->oauth()->login()) {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                    tr("You are already logged in."),
                                    tr("Access granted."));
  }
}

void FormEditInoreaderAccount::onClickedOk() {
  bool editing_account = true;

  if (m_editableRoot == nullptr) {
    // We want to confirm newly created account.
    // So save new account into DB, setup its properties.
    m_editableRoot = new InoreaderServiceRoot(m_network);
    editing_account = false;
  }

  m_editableRoot->network()->setUsername(m_ui.m_txtUsername->lineEdit()->text());
  m_editableRoot->network()->setBatchSize(m_ui.m_spinLimitMessages->value());
  m_editableRoot->saveAccountDataToDatabase();
  accept();

  if (editing_account) {
    m_editableRoot->completelyRemoveAllData();
    m_editableRoot->syncIn();
  }
}

void FormEditInoreaderAccount::onClickedCancel() {
  reject();
}

void FormEditInoreaderAccount::checkUsername(const QString& username) {
  if (username.isEmpty()) {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Error, tr("No username entered."));
  }
  else {
    m_ui.m_txtUsername->setStatus(WidgetWithStatus::StatusType::Ok, tr("Some username entered."));
  }
}

void FormEditInoreaderAccount::onAuthFailed() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("You did not grant access."),
                                  tr("There was error during testing."));
}

void FormEditInoreaderAccount::onAuthError(const QString& error, const QString& detailed_description) {
  Q_UNUSED(error)

  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                  tr("There is error. %1").arg(detailed_description),
                                  tr("There was error during testing."));
}

void FormEditInoreaderAccount::onAuthGranted() {
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                  tr("Tested successfully. You may be prompted to login once more."),
                                  tr("Your access was approved."));
}

void FormEditInoreaderAccount::hookNetwork() {
  connect(m_network->oauth(), &OAuth2Service::tokensReceived, this, &FormEditInoreaderAccount::onAuthGranted);
  connect(m_network->oauth(), &OAuth2Service::tokensRetrieveError, this, &FormEditInoreaderAccount::onAuthError);
  connect(m_network->oauth(), &OAuth2Service::authFailed, this, &FormEditInoreaderAccount::onAuthFailed);
}

void FormEditInoreaderAccount::unhookNetwork() {
  disconnect(m_network->oauth(), &OAuth2Service::tokensReceived, this, &FormEditInoreaderAccount::onAuthGranted);
  disconnect(m_network->oauth(), &OAuth2Service::tokensRetrieveError, this, &FormEditInoreaderAccount::onAuthError);
  disconnect(m_network->oauth(), &OAuth2Service::authFailed, this, &FormEditInoreaderAccount::onAuthFailed);
}

InoreaderServiceRoot* FormEditInoreaderAccount::execForCreate() {
  setWindowTitle(tr("Add new Inoreader account"));
  m_network = new InoreaderNetworkFactory(this);

  m_ui.m_txtAppId->lineEdit()->setText(INOREADER_OAUTH_CLI_ID);
  m_ui.m_txtAppKey->lineEdit()->setText(INOREADER_OAUTH_CLI_KEY);
  m_ui.m_txtRedirectUrl->lineEdit()->setText(INOREADER_OAUTH_CLI_REDIRECT);

  hookNetwork();
  exec();
  unhookNetwork();

  return m_editableRoot;
}

void FormEditInoreaderAccount::execForEdit(InoreaderServiceRoot* existing_root) {
  setWindowTitle(tr("Edit existing Inoreader account"));
  m_editableRoot = existing_root;
  m_ui.m_txtUsername->lineEdit()->setText(existing_root->network()->userName());
  m_ui.m_spinLimitMessages->setValue(existing_root->network()->batchSize());

  m_network = existing_root->network();
  hookNetwork();
  exec();
  unhookNetwork();
}

void FormEditInoreaderAccount::checkOAuthValue(const QString& value) {
  LineEditWithStatus* line_edit = qobject_cast<LineEditWithStatus*>(sender()->parent());

  if (line_edit != nullptr) {
    if (value.isEmpty()) {
      line_edit->setStatus(WidgetWithStatus::Error, tr("Empty value is entered."));
    }
    else {
      line_edit->setStatus(WidgetWithStatus::Ok, tr("Some value is entered."));
    }
  }
}
