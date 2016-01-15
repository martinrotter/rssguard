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


#ifndef FORMEDITACCOUNT_H
#define FORMEDITACCOUNT_H

#include <QDialog>

#include "ui_formeditaccount.h"


namespace Ui {
  class FormEditAccount;
}

class TtRssServiceRoot;

class FormEditAccount : public QDialog {
    Q_OBJECT

  public:
    explicit FormEditAccount(QWidget *parent = 0);
    virtual ~FormEditAccount();

    TtRssServiceRoot *execForCreate();
    void execForEdit(TtRssServiceRoot *existing_root);

  private slots:
    void displayPassword(bool display);
    void displayHttpPassword(bool display);
    void performTest();
    void onClickedOk();
    void onClickedCancel();

    void onUsernameChanged();
    void onPasswordChanged();
    void onHttpUsernameChanged();
    void onHttpPasswordChanged();
    void onUrlChanged();
    void checkOkButton();

  private:
    QScopedPointer<Ui::FormEditAccount> m_ui;
    TtRssServiceRoot *m_editableRoot;
    QPushButton *m_btnOk;
};

#endif // FORMEDITACCOUNT_H
