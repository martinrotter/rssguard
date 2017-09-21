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
#include "services/inoreader/inoreaderserviceroot.h"

FormEditInoreaderAccount::FormEditInoreaderAccount(QWidget* parent) : QDialog(parent), m_editableRoot(nullptr) {
  m_ui.setupUi(this);
  GuiUtilities::applyDialogProperties(*this, qApp->icons()->miscIcon(QSL("inoreader")));
  m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                  tr("Not tested yet."),
                                  tr("Not tested yet."));
  m_ui.m_lblTestResult->label()->setWordWrap(true);

  connect(m_ui.m_btnTestSetup, &QPushButton::clicked, this, &FormEditInoreaderAccount::testSetup);
  connect(&m_network, &InoreaderNetworkFactory::accessGranted, [this]() {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Ok,
                                    tr("Tested successfully. You may be prompted to login once more."),
                                    tr("Your access was approved."));
  });
  connect(&m_network, &InoreaderNetworkFactory::error, [this](const QString& err) {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Error,
                                    tr("There is error. %1").arg(err),
                                    tr("There was error during testing."));
  });
}

FormEditInoreaderAccount::~FormEditInoreaderAccount() {}

void FormEditInoreaderAccount::testSetup() {
  if (m_network.isLoggedIn()) {
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Information,
                                    tr("Access granted successfully."),
                                    tr("Access granted successfully."));
  }
  else {
    m_network.logIn();
    m_ui.m_lblTestResult->setStatus(WidgetWithStatus::StatusType::Progress,
                                    tr("Requested access approval. Respond to it, please."),
                                    tr("Access approval was requested via OAuth 2.0 protocol."));
  }
}

InoreaderServiceRoot* FormEditInoreaderAccount::execForCreate() {
  setWindowTitle(tr("Add new Inoreader account"));
  exec();
  return m_editableRoot;
}
