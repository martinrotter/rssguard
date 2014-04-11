#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>

#include <QNetworkReply>
#include <QSslError>

#include "definitions/definitions.h"


class SilentNetworkAccessManager;
class QTimer;

class Downloader : public QObject {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit Downloader(QObject *parent = 0);
    virtual ~Downloader();

  public slots:
    // Performs asynchronous download of given file.
    // Redirections are handled.
    void downloadFile(const QString &url,
                      bool protected_contents = false,
                      const QString &username = QString(),
                      const QString &password = QString());

  signals:
    // Emitted when new progress is known.
    void progress(qint64 bytes_received, qint64 bytes_total);
    void completed(QNetworkReply::NetworkError status, QByteArray contents = QByteArray());

  private slots:
    // Called when current reply is processed.
    void finished(QNetworkReply *reply);

    // Called when progress of downloaded file changes.
    void progressInternal(qint64 bytes_received, qint64 bytes_total);

    // Called when current operation times out.
    void timeout();

  private:
    void runRequest(const QNetworkRequest &request);

  private:
    QNetworkReply *m_activeReply;

    SilentNetworkAccessManager *m_downloadManager;
    QTimer *m_timer;
};

#endif // DOWNLOADER_H
