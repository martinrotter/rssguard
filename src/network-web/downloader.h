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

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>

#include "definitions/definitions.h"

#include <QNetworkReply>
#include <QSslError>


class SilentNetworkAccessManager;
class QTimer;

class Downloader : public QObject {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit Downloader(QObject *parent = 0);
    virtual ~Downloader();

    // Access to last received full output data/error/content-type.
    QByteArray lastOutputData() const;
    QNetworkReply::NetworkError lastOutputError() const;
    QVariant lastContentType() const;

  public slots:
    void cancel();

    void appendRawHeader(const QByteArray &name, const QByteArray &value);

    // Performs asynchronous download of given file. Redirections are handled.
    void downloadFile(const QString &url, int timeout = DOWNLOAD_TIMEOUT, bool protected_contents = false,
                      const QString &username = QString(), const QString &password = QString());

    void uploadFile(const QString &url, const QByteArray &data, int timeout = DOWNLOAD_TIMEOUT,
                    bool protected_contents = false, const QString &username = QString(),
                    const QString &password = QString());

    // Performs asynchronous upload of given data as HTTP POST.
    // User needs to setup "Content-Encoding" header which
    // matches encoding of the data.
    void manipulateData(const QString &url, QNetworkAccessManager::Operation operation,
                        const QByteArray &data = QByteArray(),
                        int timeout = DOWNLOAD_TIMEOUT, bool protected_contents = false,
                        const QString &username = QString(), const QString &password = QString());

  signals:
    // Emitted when new progress is known.
    void progress(qint64 bytes_received, qint64 bytes_total);
    void completed(QNetworkReply::NetworkError status, QByteArray contents = QByteArray());

  private slots:
    // Called when current reply is processed.
    void finished();

    // Called when progress of downloaded file changes.
    void progressInternal(qint64 bytes_received, qint64 bytes_total);

  private:
    void runDeleteRequest(const QNetworkRequest &request);
    void runPutRequest(const QNetworkRequest &request, const QByteArray &data);
    void runPostRequest(const QNetworkRequest &request, const QByteArray &data);
    void runGetRequest(const QNetworkRequest &request);

  private:
    QNetworkReply *m_activeReply;
    QScopedPointer<SilentNetworkAccessManager> m_downloadManager;
    QTimer *m_timer;
    QHash<QByteArray, QByteArray> m_customHeaders;
    QByteArray m_inputData;

    bool m_targetProtected;
    QString m_targetUsername;
    QString m_targetPassword;

    // Response data.
    QByteArray m_lastOutputData;
    QNetworkReply::NetworkError m_lastOutputError;
    QVariant m_lastContentType;
};

#endif // DOWNLOADER_H
