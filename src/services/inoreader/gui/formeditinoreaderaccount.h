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

#ifndef FORMEDITINOREADERACCOUNT_H
#define FORMEDITINOREADERACCOUNT_H

#include <QDialog>

#include "ui_formeditinoreaderaccount.h"

#include "services/inoreader/network/inoreadernetworkfactory.h"

namespace Ui {
  class FormEditInoreaderAccount;
}

class InoreaderServiceRoot;

class FormEditInoreaderAccount : public QDialog {
  Q_OBJECT

  public:
    explicit FormEditInoreaderAccount(QWidget* parent = nullptr);
    virtual ~FormEditInoreaderAccount();

    InoreaderServiceRoot* execForCreate();

    void execForEdit(InoreaderServiceRoot* existing_root);

  private slots:
    void testSetup();
    void onClickedOk();
    void onClickedCancel();
    void checkOAuthValue(const QString& value);
    void checkUsername(const QString& username);
    void onAuthFailed();
    void onAuthError(const QString& error, const QString& detailed_description);
    void onAuthGranted();

  private:
    void hookNetwork();
    void unhookNetwork();

    Ui::FormEditInoreaderAccount m_ui;
    InoreaderNetworkFactory* m_network;
    InoreaderServiceRoot* m_editableRoot;
};

#endif // FORMEDITINOREADERACCOUNT_H
