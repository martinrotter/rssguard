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

#include "network-web/webbrowser.h"
#include "miscellaneous/application.h"

#include "network-web/adblock/adblockmanager.h"

#include <QNetworkReply>


QList<WebPage*> WebPage::s_livingPages;

WebPage::WebPage(QObject *parent)
  : QWebEnginePage(parent), m_loadProgress(-1) {
  connect(this, SIGNAL(loadProgress(int)), this, SLOT(progress(int)));
  connect(this, SIGNAL(loadFinished(bool)), this, SLOT(finished()));
  connect(this, SIGNAL(urlChanged(QUrl)), this, SLOT(urlChanged(QUrl)));

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

void WebPage::populateNetworkRequest(QNetworkRequest &request) {
  WebPage *page_pointer = this;

  QVariant variant = QVariant::fromValue((void*) page_pointer);
  request.setAttribute((QNetworkRequest::Attribute)(QNetworkRequest::User + 100), variant);
}

QString WebPage::toHtml() const {
  return QString();
  // TODO: TODO
  //return mainFrame()->toHtml();
}
