// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GEMINISCHEMEHANDLER_H
#define GEMINISCHEMEHANDLER_H

#include "network-web/gemini/geminiclient.h"

#include <QWebEngineUrlRequestJob>
#include <QWebEngineUrlSchemeHandler>

class GeminiSchemeHandler : public QWebEngineUrlSchemeHandler {
  public:
    explicit GeminiSchemeHandler(QObject* parent = nullptr);

    virtual void requestStarted(QWebEngineUrlRequestJob* request);

  private slots:
    void onRedirect(const QUrl& uri, bool is_permanent);
    void onCompleted(const QByteArray& data, const QString& mime);
    void onNetworkError(GeminiClient::NetworkError error, const QString& reason);

    void onJobDeleted(QObject* job);

  private:
    QHash<QWebEngineUrlRequestJob*, GeminiClient*> m_jobs;
};

#endif // GEMINISCHEMEHANDLER_H
