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

#include "gui/dialogs/oauthlogin.h"

#include <QUrlQuery>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

OAuthLogin::OAuthLogin(QWidget* parent) : QDialog(parent) {
  m_ui.setupUi(this);

  connect(this, &OAuthLogin::rejected, this, &OAuthLogin::authRejected);
  connect(m_ui.m_loginPage, &WebViewer::urlChanged, this, &OAuthLogin::urlChanged);
}

void OAuthLogin::login(const QString& consentPageUrl, const QString& redirect_uri) {
  m_ui.m_loginPage->page()->profile()->clearHttpCache();
  m_ui.m_loginPage->page()->profile()->cookieStore()->deleteAllCookies();

  m_redirectUri = redirect_uri;
  m_ui.m_loginPage->setUrl(QUrl(consentPageUrl));
  exec();
}

void OAuthLogin::urlChanged(QUrl url) {
  QString redirected_uri = url.toString();
  QUrlQuery query(QUrl(redirected_uri).query());

  if (redirected_uri.startsWith(m_redirectUri)) {
    if (query.hasQueryItem(QSL("code"))) {
      emit authGranted(query.queryItemValue(QSL("code")));

      accept();
    }
    else {
      emit authRejected();

      reject();
    }
  }
}
