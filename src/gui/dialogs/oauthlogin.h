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

#ifndef OAUTHLOGIN_H
#define OAUTHLOGIN_H

#include <QDialog>

#include "ui_oauthlogin.h"

namespace Ui {
  class OAuthLogin;
}

class OAuthLogin : public QDialog {
  Q_OBJECT

  public:
    explicit OAuthLogin(QWidget* parent = 0);

    void login(const QString& consentPageUrl, const QString& redirect_uri);

  private slots:
    void urlChanged(QUrl url);

  signals:
    void authRejected();
    void authGranted(QString authCode);

  private:
    Ui::OAuthLogin m_ui;
    QString m_redirectUri;
};

#endif // OAUTHLOGIN_H
