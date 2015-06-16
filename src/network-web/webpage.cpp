// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
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


QList<WebPage*> WebPage::livingPages_;

WebPage::WebPage(QObject *parent)
  : QWebPage(parent), loadProgress_(-1) {
  // Setup global network access manager.
  // NOTE: This makes network settings easy for all web browsers.
  setNetworkAccessManager(new WebBrowserNetworkAccessManager(this, this));
  setForwardUnsupportedContent(true);
  connect(this, SIGNAL(unsupportedContent(QNetworkReply*)), this, SLOT(handleUnsupportedContent(QNetworkReply*)));

  connect(this, SIGNAL(loadProgress(int)), this, SLOT(progress(int)));
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(finished()));

  livingPages_.append(this);
}

WebPage::~WebPage() {
    livingPages_.removeOne(this);
}

bool WebPage::isLoading() const
{
  return loadProgress_ < 100;
}

void WebPage::progress(int prog)
{
  loadProgress_ = prog;
}

void WebPage::finished()
{
  progress(100);

  // AdBlock
  cleanBlockedObjects();
}

void WebPage::cleanBlockedObjects()
{
  AdBlockManager* manager = AdBlockManager::instance();
  if (!manager->isEnabled()) {
    return;
  }

  const QWebElement docElement = mainFrame()->documentElement();

  foreach (const AdBlockedEntry &entry, adBlockedEntries_) {
    const QString urlString = entry.url.toString();
    if (urlString.endsWith(QLatin1String(".js")) || urlString.endsWith(QLatin1String(".css"))) {
      continue;
    }

    QString urlEnd;

    int pos = urlString.lastIndexOf(QLatin1Char('/'));
    if (pos > 8) {
      urlEnd = urlString.mid(pos + 1);
    }

    if (urlString.endsWith(QLatin1Char('/'))) {
      urlEnd = urlString.left(urlString.size() - 1);
    }

    QString selector("img[src$=\"%1\"], iframe[src$=\"%1\"],embed[src$=\"%1\"]");
    QWebElementCollection elements = docElement.findAll(selector.arg(urlEnd));

    foreach (QWebElement element, elements) {
      QString src = element.attribute("src");
      src.remove(QLatin1String("../"));

      if (urlString.contains(src)) {
        element.setStyleProperty("display", "none");
      }
    }
  }

  // Apply domain-specific element hiding rules
  QString elementHiding = manager->elementHidingRulesForDomain(mainFrame()->url());
  if (elementHiding.isEmpty()) {
    return;
  }

  elementHiding.append(QLatin1String("\n</style>"));

  QWebElement bodyElement = docElement.findFirst("body");
  bodyElement.appendInside("<style type=\"text/css\">\n/* AdBlock for QupZilla */\n" + elementHiding);

  // When hiding some elements, scroll position of page will change
  // If user loaded anchor link in background tab (and didn't show it yet), fix the scroll position
  if (view() && !view()->isVisible() && !mainFrame()->url().fragment().isEmpty()) {
    mainFrame()->scrollToAnchor(mainFrame()->url().fragment());
  }
}

void WebPage::urlChanged(const QUrl &url)
{
  Q_UNUSED(url)

  if (isLoading()) {
    adBlockedEntries_.clear();
  }
}

void WebPage::addAdBlockRule(const AdBlockRule* rule, const QUrl &url)
{
  AdBlockedEntry entry;
  entry.rule = rule;
  entry.url = url;

  if (!adBlockedEntries_.contains(entry)) {
    adBlockedEntries_.append(entry);
  }
}

QVector<WebPage::AdBlockedEntry> WebPage::adBlockedEntries() const
{
  return adBlockedEntries_;
}

bool WebPage::isPointerSafeToUse(WebPage* page)
{
  // Pointer to WebPage is passed with every QNetworkRequest casted to void*
  // So there is no way to test whether pointer is still valid or not, except
  // this hack.

  return page == 0 ? false : livingPages_.contains(page);
}

QString WebPage::toPlainText() const {
  return mainFrame()->toPlainText();
}

void WebPage::populateNetworkRequest(QNetworkRequest &request)
{
  WebPage* pagePointer = this;

  QVariant variant = QVariant::fromValue((void*) pagePointer);
  request.setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 100), variant);
/*
  if (lastRequestUrl_ == request.url()) {
    request.setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 101), lastRequestType_);
    if (lastRequestType_ == NavigationTypeLinkClicked) {
      request.setRawHeader("X-QuiteRSS-UserLoadAction", QByteArray("1"));
    }
  }*/
}

void WebPage::handleUnsupportedContent(QNetworkReply *reply) {
  if (!reply) {
    return;
  }

  QUrl replyUrl = reply->url();

  if (replyUrl.scheme() == QLatin1String("abp")) {
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

QString WebPage::toHtml() const {
  return mainFrame()->toHtml();
}

bool WebPage::acceptNavigationRequest(QWebFrame *frame,
                                      const QNetworkRequest &request,
                                      QWebPage::NavigationType type) {
  QString scheme = request.url().scheme();

  if (scheme == "mailto" || scheme == "ftp") {
    qWarning("Received request with scheme '%s', blocking it.", qPrintable(scheme));
    return false;
  }

  lastRequestType_ = type;
  lastRequestUrl_ = request.url();

  if (type == QWebPage::NavigationTypeLinkClicked &&
      frame == mainFrame()) {
    // Make sure that appropriate signal is emitted even if
    // no delegation is enabled.
    emit linkClicked(request.url());
  }

  qDebug("Accepting request '%s'.", qPrintable(request.url().toString()));
  return QWebPage::acceptNavigationRequest(frame, request, type);
}
