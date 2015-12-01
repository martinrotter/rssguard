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

#include "network-web/downloader.h"

#include "network-web/silentnetworkaccessmanager.h"

#include <QTimer>


Downloader::Downloader(QObject *parent)
  : QObject(parent), m_activeReply(NULL), m_downloadManager(new SilentNetworkAccessManager(this)),
    m_timer(new QTimer(this)), m_customHeaders(QHash<QByteArray, QByteArray>()), m_inputData(QByteArray()),
    m_lastOutputData(QByteArray()), m_lastOutputError(QNetworkReply::NoError), m_lastContentType(QVariant()) {

  m_timer->setInterval(DOWNLOAD_TIMEOUT);
  m_timer->setSingleShot(true);

  connect(m_timer, SIGNAL(timeout()), this, SLOT(timeout()));
}

Downloader::~Downloader() {
  m_downloadManager->deleteLater();
}

void Downloader::downloadFile(const QString &url, int timeout, bool protected_contents, const QString &username, const QString &password) {
  QNetworkRequest request;
  QObject originatingObject;
  QString non_const_url = url;

  // Set credential information as originating object.
  originatingObject.setProperty("protected", protected_contents);
  originatingObject.setProperty("username", username);
  originatingObject.setProperty("password", password);
  request.setOriginatingObject(&originatingObject);

  foreach (const QByteArray &header_name, m_customHeaders.keys()) {
    request.setRawHeader(header_name, m_customHeaders.value(header_name));
  }

  // Set url for this request and fire it up.
  m_timer->setInterval(timeout);

  if (non_const_url.startsWith(URI_SCHEME_FEED)) {
    qDebug("Replacing URI schemes for '%s'.", qPrintable(non_const_url));
    request.setUrl(non_const_url.replace(QRegExp(QString('^') + URI_SCHEME_FEED), QString(URI_SCHEME_HTTP)));
  }
  else {
    request.setUrl(non_const_url);
  }

  runGetRequest(request);
}

void Downloader::uploadData(const QString &url, const QByteArray &data, int timeout) {
  QNetworkRequest request;
  QString non_const_url = url;

  foreach (const QByteArray &header_name, m_customHeaders.keys()) {
    request.setRawHeader(header_name, m_customHeaders.value(header_name));
  }

  m_inputData = data;

  // Set url for this request and fire it up.
  m_timer->setInterval(timeout);

  if (non_const_url.startsWith(URI_SCHEME_FEED)) {
    qDebug("Replacing URI schemes for '%s'.", qPrintable(non_const_url));
    request.setUrl(non_const_url.replace(QRegExp(QString('^') + URI_SCHEME_FEED), QString(URI_SCHEME_HTTP)));
  }
  else {
    request.setUrl(non_const_url);
  }

  runPostRequest(request, m_inputData);
}

void Downloader::finished() {
  QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
  QNetworkAccessManager::Operation reply_operation = reply->operation();

  m_timer->stop();

  // In this phase, some part of downloading process is completed.
  QUrl redirection_url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

  if (redirection_url.isValid()) {
    // Communication indicates that HTTP redirection is needed.
    // Setup redirection URL and download again.
    QNetworkRequest request = reply->request();

    if (redirection_url.host().isEmpty()) {
      request.setUrl(QUrl(reply->request().url().scheme() + QSL("://") + reply->request().url().host() + redirection_url.toString()));
    }
    else {
      request.setUrl(redirection_url);
    }

    m_activeReply->deleteLater();
    m_activeReply = NULL;

    if (reply_operation == QNetworkAccessManager::GetOperation) {
      runGetRequest(request);
    }
    else if (reply_operation == QNetworkAccessManager::PostOperation) {
      runPostRequest(request, m_inputData);
    }
  }
  else {
    // No redirection is indicated. Final file is obtained in our "reply" object.
    // Read the data into output buffer.
    m_lastOutputData = reply->readAll();
    m_lastContentType = reply->header(QNetworkRequest::ContentTypeHeader);
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

void Downloader::runPostRequest(const QNetworkRequest &request, const QByteArray &data) {
  m_timer->start();
  m_activeReply = m_downloadManager->post(request, data);

  connect(m_activeReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(progressInternal(qint64,qint64)));
  connect(m_activeReply, SIGNAL(finished()), this, SLOT(finished()));
}

void Downloader::runGetRequest(const QNetworkRequest &request) {
  m_timer->start();
  m_activeReply = m_downloadManager->get(request);

  connect(m_activeReply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(progressInternal(qint64,qint64)));
  connect(m_activeReply, SIGNAL(finished()), this, SLOT(finished()));
}

QVariant Downloader::lastContentType() const {
  return m_lastContentType;
}

void Downloader::appendRawHeader(const QByteArray &name, const QByteArray &value) {
  m_customHeaders.insert(name, value);
}

QNetworkReply::NetworkError Downloader::lastOutputError() const {
  return m_lastOutputError;
}

QByteArray Downloader::lastOutputData() const {
  return m_lastOutputData;
}
