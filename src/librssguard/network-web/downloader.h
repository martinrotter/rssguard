// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>

#include "definitions/definitions.h"
#include "network-web/httpresponse.h"

#include <QHttpMultiPart>
#include <QNetworkProxy>
#include <QNetworkReply>
#include <QSslError>

class SilentNetworkAccessManager;
class QTimer;

class Downloader : public QObject {
    Q_OBJECT

  public:
    explicit Downloader(QObject* parent = nullptr);
    virtual ~Downloader();

    // Access to last received full output data/error/content-type.
    QByteArray lastOutputData() const;
    QNetworkReply::NetworkError lastOutputError() const;
    QList<HttpResponse> lastOutputMultipartData() const;
    QString lastContentType() const;
    QList<QNetworkCookie> lastCookies() const;
    int lastHttpStatusCode() const;
    QMap<QString, QString> lastHeaders() const;

    void setProxy(const QNetworkProxy& proxy);

  public slots:
    void cancel();

    void appendRawHeader(const QByteArray& name, const QByteArray& value);

    // Performs asynchronous download of given file. Redirections are handled.
    void downloadFile(const QString& url,
                      int timeout = DOWNLOAD_TIMEOUT,
                      bool protected_contents = false,
                      const QString& username = QString(),
                      const QString& password = QString());

    void uploadFile(const QString& url,
                    const QByteArray& data,
                    int timeout = DOWNLOAD_TIMEOUT,
                    bool protected_contents = false,
                    const QString& username = QString(),
                    const QString& password = QString());

    void manipulateData(const QString& url,
                        QNetworkAccessManager::Operation operation,
                        QHttpMultiPart* multipart_data,
                        int timeout = DOWNLOAD_TIMEOUT,
                        bool protected_contents = false,
                        const QString& username = QString(),
                        const QString& password = QString());

    void manipulateData(const QString& url,
                        QNetworkAccessManager::Operation operation,
                        const QByteArray& data = QByteArray(),
                        int timeout = DOWNLOAD_TIMEOUT,
                        bool protected_contents = false,
                        const QString& username = QString(),
                        const QString& password = QString());

  signals:
    void progress(qint64 bytes_received, qint64 bytes_total);
    void completed(const QUrl& url, QNetworkReply::NetworkError status, int http_code, QByteArray contents = {});

  private slots:

    // Called when current reply is processed.
    void finished();

    // Called when progress of downloaded file changes.
    void progressInternal(qint64 bytes_received, qint64 bytes_total);

  private:
    void setCustomPropsToReply(QNetworkReply* reply);
    QList<HttpResponse> decodeMultipartAnswer(QNetworkReply* reply);
    void manipulateData(const QString& url,
                        QNetworkAccessManager::Operation operation,
                        const QByteArray& data,
                        QHttpMultiPart* multipart_data,
                        int timeout = DOWNLOAD_TIMEOUT,
                        bool protected_contents = false,
                        const QString& username = QString(),
                        const QString& password = QString());
    void runDeleteRequest(const QNetworkRequest& request);
    void runPutRequest(const QNetworkRequest& request, const QByteArray& data);
    void runPostRequest(const QNetworkRequest& request, QHttpMultiPart* multipart_data);
    void runPostRequest(const QNetworkRequest& request, const QByteArray& data);
    void runGetRequest(const QNetworkRequest& request);

  private:
    QNetworkReply* m_activeReply;
    QScopedPointer<SilentNetworkAccessManager> m_downloadManager;
    QTimer* m_timer;
    QHash<QByteArray, QByteArray> m_customHeaders;
    QByteArray m_inputData;
    QHttpMultiPart* m_inputMultipartData;
    bool m_targetProtected;
    QString m_targetUsername;
    QString m_targetPassword;

    // Response data.
    QByteArray m_lastOutputData;
    QList<HttpResponse> m_lastOutputMultipartData;
    QNetworkReply::NetworkError m_lastOutputError;
    int m_lastHttpStatusCode;
    QString m_lastContentType;
    QList<QNetworkCookie> m_lastCookies;
    QMap<QString, QString> m_lastHeaders;
};

#endif // DOWNLOADER_H
