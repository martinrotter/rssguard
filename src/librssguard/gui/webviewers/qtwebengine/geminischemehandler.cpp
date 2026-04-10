// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/gemini/geminischemehandler.h"

#include "definitions/definitions.h"
#include "miscellaneous/iofactory.h"
#include "network-web/gemini/geminiparser.h"

#include <QBuffer>

GeminiSchemeHandler::GeminiSchemeHandler(QObject* parent)
  : QWebEngineUrlSchemeHandler(parent), m_geminiParser(GeminiParser(true)) {}

void GeminiSchemeHandler::requestStarted(QWebEngineUrlRequestJob* request) {
  GeminiClient* gemini_client = new GeminiClient(this);

  m_jobs.insert(request, gemini_client);

  connect(gemini_client, &GeminiClient::redirected, this, &GeminiSchemeHandler::onRedirect);
  connect(gemini_client, &GeminiClient::requestComplete, this, &GeminiSchemeHandler::onCompleted);
  connect(gemini_client, &GeminiClient::networkError, this, &GeminiSchemeHandler::onNetworkError);

  connect(request, &QWebEngineUrlRequestJob::destroyed, this, &GeminiSchemeHandler::onJobDeleted);

  gemini_client->startRequest(request->requestUrl(), GeminiClient::RequestOptions::IgnoreTlsErrors);
}

void GeminiSchemeHandler::onRedirect(const QUrl& uri, bool is_permanent) {
  GeminiClient* gemini_client = qobject_cast<GeminiClient*>(sender());
  auto* job = m_jobs.key(gemini_client);

  if (job != nullptr) {
    job->redirect(uri);
    m_jobs.remove(job);
    gemini_client->deleteLater();
  }
}

void GeminiSchemeHandler::onCompleted(const QByteArray& data, const QString& mime) {
  GeminiClient* gemini_client = qobject_cast<GeminiClient*>(sender());
  auto* job = m_jobs.key(gemini_client);

  if (job != nullptr) {
    QBuffer* buf = new QBuffer();
    QString target_mime;
    buf->open(QBuffer::ReadWrite);

    if (mime.startsWith(QSL(GEMINI_MIME_TYPE))) {
      QString htmlized_gemini = m_geminiParser.geminiToHtml(data);
      buf->write(htmlized_gemini.toUtf8());

#if !defined(NDEBUG)
      IOFactory::writeFile("aa.html", htmlized_gemini.toUtf8());
#endif

      target_mime = QSL("text/html");
    }
    else {
      buf->write(data);
      target_mime = mime;
    }

    buf->seek(0);

    connect(job, &QWebEngineUrlRequestJob::destroyed, buf, &QBuffer::deleteLater);
    job->reply(target_mime.toLocal8Bit(), buf);
    m_jobs.remove(job);
    gemini_client->deleteLater();
  }
}

void GeminiSchemeHandler::onNetworkError(GeminiClient::NetworkError error, const QString& reason) {
  GeminiClient* gemini_client = qobject_cast<GeminiClient*>(sender());
  auto* job = m_jobs.key(gemini_client);

  if (job != nullptr) {
    job->fail(QWebEngineUrlRequestJob::Error::RequestFailed);
    m_jobs.remove(job);
    gemini_client->deleteLater();
  }
}

void GeminiSchemeHandler::onJobDeleted(QObject* job) {
  auto* key = qobject_cast<QWebEngineUrlRequestJob*>(job);
  auto* gemini_client = m_jobs.value(key);

  if (gemini_client != nullptr) {
    gemini_client->deleteLater();
  }

  if (key != nullptr) {
    m_jobs.remove(key);
  }
}
