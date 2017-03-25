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

#ifndef FORMEDITOWNCLOUDACCOUNT_H
#define FORMEDITOWNCLOUDACCOUNT_H

#include <QDialog>

#include "ui_formeditowncloudaccount.h"


namespace Ui {
  class FormEditAccount;
}

class OwnCloudServiceRoot;

class FormEditOwnCloudAccount : public QDialog {
    Q_OBJECT

  public:
    explicit FormEditOwnCloudAccount(QWidget *parent = 0);
    virtual ~FormEditOwnCloudAccount();

    OwnCloudServiceRoot *execForCreate();
    void execForEdit(OwnCloudServiceRoot *existing_root);

  private slots:
    void displayPassword(bool display);
    void performTest();
    void onClickedOk();
    void onClickedCancel();

    void onUsernameChanged();
    void onPasswordChanged();
    void onUrlChanged();
    void checkOkButton();

  private:
    QScopedPointer<Ui::FormEditOwnCloudAccount> m_ui;
    OwnCloudServiceRoot *m_editableRoot;
    QPushButton *m_btnOk;
};

#endif // FORMEDITOWNCLOUDACCOUNT_H
