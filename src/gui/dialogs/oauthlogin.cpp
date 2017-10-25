// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/dialogs/oauthlogin.h"

#include "gui/guiutilities.h"
#include "miscellaneous/application.h"
#include "miscellaneous/iconfactory.h"

#include <QUrlQuery>
#include <QWebEngineCookieStore>
#include <QWebEngineProfile>

OAuthLogin::OAuthLogin(QWidget* parent) : QDialog(parent) {
  m_ui.setupUi(this);

  GuiUtilities::applyDialogProperties(*this);

  connect(this, &OAuthLogin::rejected, this, &OAuthLogin::authRejected);
  connect(m_ui.m_loginPage, &WebViewer::urlChanged, this, &OAuthLogin::urlChanged);
}

void OAuthLogin::login(const QString& consentPageUrl, const QString& redirect_uri) {
  m_ui.m_loginPage->page()->profile()->clearHttpCache();
  m_ui.m_loginPage->page()->profile()->cookieStore()->deleteAllCookies();

  m_redirectUri = redirect_uri;
  m_ui.m_loginPage->setUrl(QUrl(consentPageUrl));
  m_ui.m_buttonBox->setFocus();
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
