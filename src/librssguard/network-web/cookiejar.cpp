// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/cookiejar.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "network-web/webfactory.h"

#include <QDateTime>
#include <QDir>
#include <QNetworkCookie>
#include <QSettings>

CookieJar::CookieJar(QObject* parent)
  : QNetworkCookieJar(parent), m_saver(AutoSaver(this, QSL("saveCookies"), 30, 45)) {
  // Load all cookies and also set them into WebEngine store.
  updateSettings();
  loadCookies();
}

QList<QNetworkCookie> CookieJar::extractCookiesFromUrl(const QString& url) {
  if (!url.contains(QSL(COOKIE_URL_IDENTIFIER))) {
    return {};
  }

  int index = url.lastIndexOf(QSL(COOKIE_URL_IDENTIFIER), -1, Qt::CaseSensitivity::CaseInsensitive);
  QString cookies_string = url.right(url.length() - index - QSL(COOKIE_URL_IDENTIFIER).size());
  QStringList cookies_list = cookies_string.split(';');
  QList<QNetworkCookie> cookies;

  for (const QString& single_cookie : cookies_list) {
    const QList<QNetworkCookie>& extracted_cookies = QNetworkCookie::parseCookies(single_cookie.toUtf8());

    if (extracted_cookies.isEmpty()) {
      continue;
    }

    QNetworkCookie cookie = extracted_cookies.at(0);
    QDateTime date = QDateTime::currentDateTime();

    date = date.addYears(30);
    cookie.setExpirationDate(date);
    cookies.append(cookie);
  }

  return cookies;
}

void CookieJar::loadCookies() {
  Settings* sett = qApp->settings();
  auto keys = sett->allKeys(GROUP(Cookies));

  for (const QString& cookie_key : std::as_const(keys)) {
    QByteArray encoded = sett->password(GROUP(Cookies), cookie_key, {}).toByteArray();

    if (!encoded.isEmpty()) {
      auto cookie = QNetworkCookie::parseCookies(encoded);

      if (!cookie.isEmpty()) {
        if (!insertCookieInternal(cookie.at(0), true, false)) {
          qCriticalNN << LOGSEC_NETWORK << "Failed to load cookie" << QUOTE_W_SPACE(cookie_key) << "from settings.";
          sett->remove(Cookies::ID, cookie_key);
        }
      }
    }
  }
}

void CookieJar::saveCookies() {
  auto cookies = allCookies();
  Settings* sett = qApp->settings();
  int i = 1;

  sett->beginGroup(GROUP(Cookies));
  qobject_cast<QSettings*>(sett)->remove(QString());
  sett->endGroup();

  for (const QNetworkCookie& cookie : cookies) {
    /*if (cookie.isSessionCookie()) {
      continue;
    }*/
    sett->setPassword(GROUP(Cookies),
                      QSL("%1-%2").arg(QString::number(i++), QString::fromUtf8(cookie.name())),
                      cookie.toRawForm(QNetworkCookie::RawForm::Full));
  }
}

QList<QNetworkCookie> CookieJar::cookiesForUrl(const QUrl& url) const {
  QReadLocker l(&m_lock);
  return QNetworkCookieJar::cookiesForUrl(url);
}

bool CookieJar::setCookiesFromUrl(const QList<QNetworkCookie>& cookie_list, const QUrl& url) {
  QWriteLocker l(&m_lock);
  return QNetworkCookieJar::setCookiesFromUrl(cookie_list, url);
}

bool CookieJar::insertCookieInternal(const QNetworkCookie& cookie, bool notify_others, bool should_save) {
  auto result = QNetworkCookieJar::insertCookie(cookie);

  if (result) {
    if (should_save) {
      m_saver.changeOccurred();
      // saveCookies();
    }

    Q_UNUSED(notify_others)
  }

  return result;
}

bool CookieJar::updateCookieInternal(const QNetworkCookie& cookie, bool notify_others) {
  auto result = QNetworkCookieJar::updateCookie(cookie);

  if (result) {
    m_saver.changeOccurred();
    // saveCookies();
    Q_UNUSED(notify_others)
  }

  return result;
}

bool CookieJar::deleteCookieInternal(const QNetworkCookie& cookie, bool notify_others) {
  auto result = QNetworkCookieJar::deleteCookie(cookie);

  if (result) {
    m_saver.changeOccurred();
    // saveCookies();
    Q_UNUSED(notify_others)
  }

  return result;
}

bool CookieJar::insertCookie(const QNetworkCookie& cookie) {
  if (m_ignoreAllCookies) {
    return {};
  }
  else {
    QWriteLocker l(&m_lock);
    return insertCookieInternal(cookie, false, true);
  }
}

bool CookieJar::deleteCookie(const QNetworkCookie& cookie) {
  QWriteLocker l(&m_lock);
  return deleteCookieInternal(cookie, false);
}

void CookieJar::updateSettings() {
  m_ignoreAllCookies = qApp->settings()->value(GROUP(Network), SETTING(Network::IgnoreAllCookies)).toBool();

  if (m_ignoreAllCookies) {
    setAllCookies({});
    qApp->settings()->remove(GROUP(Cookies));
  }
}

bool CookieJar::updateCookie(const QNetworkCookie& cookie) {
  QWriteLocker l(&m_lock);
  return updateCookieInternal(cookie, false);
}

/*
bool CookieJar::validateCookie(const QNetworkCookie &cookie, const QUrl &url) const {
  return QNetworkCookieJar::validateCookie(cookie, url);
}
*/
