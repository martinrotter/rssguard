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

#include "network-web/webpage.h"

#include "network-web/webbrowsernetworkaccessmanager.h"
#include "network-web/webbrowser.h"
#include "miscellaneous/application.h"

#include "network-web/adblock/adblockmanager.h"

#include <QNetworkReply>
#include <QWebElement>
#include <QWebFrame>


QList<WebPage*> WebPage::s_livingPages;

WebPage::WebPage(QObject *parent)
  : QWebPage(parent), m_loadProgress(-1) {
  // Setup global network access manager.
  // NOTE: This makes network settings easy for all web browsers.
  setNetworkAccessManager(new WebBrowserNetworkAccessManager(this, this));
  setForwardUnsupportedContent(true);
  connect(this, SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(handleUnsupportedContent(QNetworkReply*)));

  connect(this, SIGNAL(loadProgress(int)), this, SLOT(progress(int)));
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(finished()));

  s_livingPages.append(this);
}

WebPage::~WebPage() {
  s_livingPages.removeOne(this);
}

bool WebPage::isLoading() const {
  return m_loadProgress < 100;
}

void WebPage::progress(int prog) {
  m_loadProgress = prog;
}

void WebPage::finished() {
  progress(100);
  cleanBlockedObjects();
}

void WebPage::cleanBlockedObjects() {
  AdBlockManager *manager = AdBlockManager::instance();

  if (!manager->isEnabled()) {
    return;
  }

  const QWebElement doc_element = mainFrame()->documentElement();

  foreach (const AdBlockedEntry &entry, m_adBlockedEntries) {
    const QString url_string = entry.url.toString();
    if (url_string.endsWith(QL1S(".js")) || url_string.endsWith(QL1S(".css"))) {
      continue;
    }

    QString url_end;

    int pos = url_string.lastIndexOf(QL1C('/'));
    if (pos > 8) {
      url_end = url_string.mid(pos + 1);
    }

    if (url_string.endsWith(QL1C('/'))) {
      url_end = url_string.left(url_string.size() - 1);
    }

    QString selector(QSL("img[src$=\"%1\"], iframe[src$=\"%1\"],embed[src$=\"%1\"]"));
    QWebElementCollection elements = doc_element.findAll(selector.arg(url_end));

    foreach (QWebElement element, elements) {
      QString src = element.attribute(QSL("src"));
      src.remove(QL1S("../"));

      if (url_string.contains(src)) {
        element.setStyleProperty(QSL("display"), QSL("none"));
      }
    }
  }

  // Apply domain-specific element hiding rules
  QString element_hiding = manager->elementHidingRulesForDomain(mainFrame()->url());

  if (element_hiding.isEmpty()) {
    return;
  }

  element_hiding.append(QL1S("\n</style>"));

  QWebElement body_element = doc_element.findFirst(QSL("body"));
  body_element.appendInside(QSL("<style type=\"text/css\">\n/* AdBlock for RSS Guard */\n") + element_hiding);

  // When hiding some elements, scroll position of page will change
  // If user loaded anchor link in background tab (and didn't show it yet), fix the scroll position
  if (view() && !view()->isVisible() && !mainFrame()->url().fragment().isEmpty()) {
    mainFrame()->scrollToAnchor(mainFrame()->url().fragment());
  }
}

void WebPage::urlChanged(const QUrl &url) {
  Q_UNUSED(url)

  if (isLoading()) {
    m_adBlockedEntries.clear();
  }
}

void WebPage::addAdBlockRule(const AdBlockRule *rule, const QUrl &url) {
  AdBlockedEntry entry;

  entry.rule = rule;
  entry.url = url;

  if (!m_adBlockedEntries.contains(entry)) {
    m_adBlockedEntries.append(entry);
  }
}

QVector<WebPage::AdBlockedEntry> WebPage::adBlockedEntries() const {
  return m_adBlockedEntries;
}

bool WebPage::isPointerSafeToUse(WebPage *page) {
  // Pointer to WebPage is passed with every QNetworkRequest casted to void*
  // So there is no way to test whether pointer is still valid or not, except
  // this hack.

  return page == 0 ? false : s_livingPages.contains(page);
}

QString WebPage::toPlainText() const {
  return mainFrame()->toPlainText();
}

void WebPage::populateNetworkRequest(QNetworkRequest &request) {
  WebPage *page_pointer = this;

  QVariant variant = QVariant::fromValue((void*) page_pointer);
  request.setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 100), variant);
}

void WebPage::handleUnsupportedContent(QNetworkReply *reply) {
  if (reply != NULL) {
    const QUrl reply_url = reply->url();

    if (reply_url.scheme() == QL1S("abp")) {
      return;
    }

    switch (reply->error()) {
      case QNetworkReply::NoError:
        if (reply->header(QNetworkRequest::ContentTypeHeader).isValid()) {
          qApp->downloadManager()->handleUnsupportedContent(reply);
          return;
        }

      default:
        return;
    }
  }
}

QString WebPage::toHtml() const {
  return mainFrame()->toHtml();
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame,
                                      const QNetworkRequest &request,
                                      QWebPage::NavigationType type) {
  const QString scheme = request.url().scheme();

  if (scheme == QL1S("mailto") || scheme == QL1S("ftp")) {
    qWarning("Received request with scheme '%s', blocking it.", qPrintable(scheme));
    return false;
  }

  if (type == QWebPage::NavigationTypeLinkClicked &&
      frame == mainFrame()) {
    // Make sure that appropriate signal is emitted even if
    // no delegation is enabled.
    emit linkClicked(request.url());
  }

  qDebug("Accepting request '%s'.", qPrintable(request.url().toString()));
  return QWebPage::acceptNavigationRequest(frame, request, type);
}
