// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "network-web/downloader.h"

#include "network-web/silentnetworkaccessmanager.h"

#include <QTimer>


Downloader::Downloader(QObject *parent)
  : QObject(parent), m_activeReply(NULL), m_downloadManager(new SilentNetworkAccessManager(this)),
    m_timer(new QTimer(this)), m_lastOutputData(QByteArray()), m_lastOutputError(QNetworkReply::NoError) {

  m_timer->setInterval(DOWNLOAD_TIMEOUT);
  m_timer->setSingleShot(true);

  connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
  connect(m_downloadManager, SIGNAL(finished(QNetworkReply*)), this, SLOT(finished(QNetworkReply*)));
}

Downloader::~Downloader() {
  m_downloadManager->deleteLater();
}

void Downloader::downloadFile(const QString &url, int timeout, bool protected_contents,
                              const QString &username, const QString &password) {
  QNetworkRequest request;
  QObject originatingObject;

  // Set credential information as originating object.
  originatingObject.setProperty("protected", protected_contents);
  originatingObject.setProperty("username", username);
  originatingObject.setProperty("password", password);
  request.setOriginatingObject(&originatingObject);

  // Set url for this request and fire it up.
  request.setUrl(url);
  runGetRequest(request);
}

void Downloader::finished(QNetworkReply *reply) {
  m_timer->stop();

  // In this phase, some part of downloading process is completed.
  QUrl redirection_url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

  if (redirection_url.isValid()) {
    // Communication indicates that HTTP redirection is needed.
    // Setup redirection URL and download again.
    QNetworkRequest request = reply->request();
    request.setUrl(redirection_url);

    m_activeReply->deleteLater();
    m_activeReply = NULL;

    runGetRequest(request);
  }
  else {
    // No redirection is indicated. Final file is obtained in our "reply" object.
    // Read the data into output buffer.
    m_lastOutputData = reply->readAll();
    m_lastOutputError = reply->error();

    m_activeReply->deleteLater();
    m_activeReply = NULL;

    emit completed(m_lastOutputError, m_lastOutputData);
  }
}

void Downloader::progressInternal(qint64 bytes_received, qint64 bytes_total) {
  if (m_timer->interval() > 0) {
    m_timer->start();
  }

  emit progress(bytes_received, bytes_total);
}

void Downloader::timeout() {
  if (m_activeReply != NULL) {
    // Download action timed-out, too slow connection or target is no reachable.
    m_activeReply->abort();
  }
}

void Downloader::runGetRequest(const QNetworkRequest &request) {
  m_timer->start();
  m_activeReply = m_downloadManager->get(request);

  connect(m_activeReply, SIGNAL(downloadProgress(qint64,qint64)),
          this, SLOT(progressInternal(qint64,qint64)));
}

QNetworkReply::NetworkError Downloader::lastOutputError() const {
  return m_lastOutputError;
}

QByteArray Downloader::lastOutputData() const {
  return m_lastOutputData;
}
