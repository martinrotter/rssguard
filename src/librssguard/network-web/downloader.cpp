// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/downloader.h"

#include "miscellaneous/application.h"
#include "network-web/cookiejar.h"
#include "network-web/gemini/geminiparser.h"
#include "network-web/networkfactory.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/webfactory.h"

#include <QHttpMultiPart>
#include <QNetworkCookie>
#include <QRegularExpression>
#include <QTimer>

Downloader::Downloader(QObject* parent, NetworkFactory::CookiePolicy cookie_policy)
  : QObject(parent), m_geminiClient(new GeminiClient(this)), m_geminiParser(GeminiParser()), m_activeReply(nullptr),
    m_downloadManager(new SilentNetworkAccessManager(this)), m_geminiTimer(new QTimer(this)),
    m_geminiTimeout(DOWNLOAD_TIMEOUT), m_inputData(QByteArray()), m_inputMultipartData(nullptr),
    m_targetProtected(false), m_targetUsername(QString()), m_targetPassword(QString()),
    m_ignoreCookies(cookie_policy == NetworkFactory::CookiePolicy::IgnoreCookies), m_outputDevice(nullptr),
    m_outputDeviceWriteFailed(false), m_lastOutputData({}),
    m_lastOutputError(QNetworkReply::NetworkError::NoError), m_lastHttpStatusCode(0), m_lastHeaders({}) {
  m_geminiTimer->setInterval(DOWNLOAD_TIMEOUT);
  m_geminiTimer->setSingleShot(true);

  connect(m_geminiTimer, &QTimer::timeout, this, &Downloader::cancelGemini);

  connect(m_geminiClient, &GeminiClient::redirected, this, &Downloader::geminiRedirect);
  connect(m_geminiClient, &GeminiClient::requestComplete, this, &Downloader::geminiFinished);
  connect(m_geminiClient, &GeminiClient::networkError, this, &Downloader::geminiError);

  if (m_ignoreCookies) {
    m_downloadManager->setCookieJar(new DiscardingCookieJar(m_downloadManager.data()));
  }
  else {
    m_downloadManager->setCookieJar(qApp->web()->cookieJar());
    qApp->web()->cookieJar()->setParent(nullptr);
  }
}

Downloader::~Downloader() {
  qDebugNN << LOGSEC_NETWORK << "Destroying Downloader instance.";
}

void Downloader::geminiFinished(const QByteArray& data, const QString& mime) {
  m_geminiTimer->stop();
  m_activeReply = nullptr;

  m_lastContentType = mime;
  m_lastUrl = m_geminiClient->targetUrl();
  m_lastCookies = {};
  m_lastHeaders = {};
  m_lastHttpStatusCode = 0;
  m_lastOutputError = QNetworkReply::NetworkError::NoError;
  m_lastOutputMultipartData = {};

  if (!m_outputDevice.isNull()) {
    m_outputDeviceWriteFailed = m_outputDevice->write(data) != data.size();
  }

  /*if (mime.startsWith(QSL(GEMINI_MIME_TYPE))) {
    m_lastOutputData = m_geminiParser.geminiToHtml(data).toUtf8();
  }
  else {*/
  m_lastOutputData = m_outputDevice.isNull() ? data : QByteArray();
  //}

  if (m_outputDeviceWriteFailed) {
    m_lastOutputError = QNetworkReply::NetworkError::UnknownNetworkError;
  }

  emit completed(m_lastUrl, m_lastOutputError, m_lastHttpStatusCode, m_lastOutputData);
}

void Downloader::geminiError(GeminiClient::NetworkError error, const QString& reason) {
  m_geminiTimer->stop();
  m_activeReply = nullptr;

  m_lastContentType = QString();
  m_lastUrl = m_geminiClient->targetUrl();
  m_lastCookies = {};
  m_lastHeaders = {};
  m_lastHttpStatusCode = 404;
  m_lastOutputData = QByteArray();
  m_lastOutputError = QNetworkReply::NetworkError::UnknownNetworkError;
  m_lastOutputMultipartData = {};

  emit completed(m_lastUrl, m_lastOutputError, m_lastHttpStatusCode);
}

void Downloader::downloadFile(const QString& url,
                              int timeout,
                              bool protected_contents,
                              const QString& username,
                              const QString& password) {
  resetOutputDevice();

  manipulateData(url,
                 QNetworkAccessManager::Operation::GetOperation,
                 QByteArray(),
                 timeout,
                 protected_contents,
                 username,
                 password);
}

void Downloader::downloadFile(const QString& url,
                              QIODevice* output_device,
                              int timeout,
                              bool protected_contents,
                              const QString& username,
                              const QString& password) {
  m_outputDevice = output_device;
  m_outputDeviceWriteFailed = false;

  manipulateData(url,
                 QNetworkAccessManager::Operation::GetOperation,
                 QByteArray(),
                 timeout,
                 protected_contents,
                 username,
                 password);
}

void Downloader::uploadFile(const QString& url,
                            const QByteArray& data,
                            int timeout,
                            bool protected_contents,
                            const QString& username,
                            const QString& password) {
  resetOutputDevice();

  manipulateData(url,
                 QNetworkAccessManager::Operation::PostOperation,
                 data,
                 timeout,
                 protected_contents,
                 username,
                 password);
}

void Downloader::manipulateData(const QString& url,
                                QNetworkAccessManager::Operation operation,
                                QHttpMultiPart* multipart_data,
                                int timeout,
                                bool protected_contents,
                                const QString& username,
                                const QString& password) {
  resetOutputDevice();

  manipulateData(url, operation, QByteArray(), multipart_data, timeout, protected_contents, username, password);
}

void Downloader::manipulateData(const QString& url,
                                QNetworkAccessManager::Operation operation,
                                const QByteArray& data,
                                int timeout,
                                bool protected_contents,
                                const QString& username,
                                const QString& password) {
  resetOutputDevice();

  manipulateData(url, operation, data, nullptr, timeout, protected_contents, username, password);
}

void Downloader::geminiRedirect(const QUrl& uri, bool is_permanent) {
  m_geminiTimer->stop();

  QUrl new_url = m_geminiClient->targetUrl().resolved(uri);

  runGeminiRequest(new_url, m_geminiTimeout);
}

void Downloader::runGeminiRequest(const QUrl& url, int timeout) {
  m_geminiTimeout = timeout;

  if (m_geminiTimeout > 0) {
    m_geminiTimer->start(m_geminiTimeout);
  }

  m_geminiClient->startRequest(url, GeminiClient::RequestOptions::IgnoreTlsErrors);
}

void Downloader::manipulateData(const QString& url,
                                QNetworkAccessManager::Operation operation,
                                const QByteArray& data,
                                QHttpMultiPart* multipart_data,
                                int timeout,
                                bool protected_contents,
                                const QString& username,
                                const QString& password) {
  QString sanitized_url = NetworkFactory::sanitizeUrl(url);

  if (m_geminiClient->supportsUrl(sanitized_url)) {
    QUrl gemini_url = QUrl::fromUserInput(sanitized_url);

    runGeminiRequest(gemini_url, timeout);
  }
  else {
    QNetworkRequest request;
    QHashIterator<QByteArray, QByteArray> i(m_customHeaders);

    while (i.hasNext()) {
      i.next();
      request.setRawHeader(i.key(), i.value());
    }

    m_inputData = data;
    m_inputMultipartData = multipart_data;

    // Set url for this request and fire it up.
    request.setTransferTimeout(timeout);
    request.setUrl(qApp->web()->processFeedUriScheme(sanitized_url));

    m_targetProtected = protected_contents;
    m_targetUsername = username;
    m_targetPassword = password;

    if (operation == QNetworkAccessManager::Operation::PostOperation) {
      if (m_inputMultipartData == nullptr) {
        runPostRequest(request, m_inputData);
      }
      else {
        runPostRequest(request, m_inputMultipartData);
      }
    }
    else if (operation == QNetworkAccessManager::GetOperation) {
      runGetRequest(request);
    }
    else if (operation == QNetworkAccessManager::PutOperation) {
      runPutRequest(request, m_inputData);
    }
    else if (operation == QNetworkAccessManager::DeleteOperation) {
      runDeleteRequest(request);
    }
  }
}

static int numberOfRedirections(QNetworkReply* reply) {
  return reply->property("redirections_count").toInt();
}

static int setNumberOfRedirections(QNetworkReply* reply, int number) {
  return reply->setProperty("redirections_count", number);
}

static int effectivePort(const QUrl& url) {
  if (url.port() >= 0) {
    return url.port();
  }

  return url.scheme() == QL1S("https") ? 443 : url.scheme() == QL1S("http") ? 80 : -1;
}

static bool hasSameOrigin(const QUrl& first, const QUrl& second) {
  return first.scheme().compare(second.scheme(), Qt::CaseSensitivity::CaseInsensitive) == 0 &&
         first.host().compare(second.host(), Qt::CaseSensitivity::CaseInsensitive) == 0 &&
         effectivePort(first) == effectivePort(second);
}

void Downloader::finished() {
  auto* reply = qobject_cast<QNetworkReply*>(sender());

  QNetworkAccessManager::Operation reply_operation = reply->operation();

  QUrl original_url = reply->property("original_url").toUrl();

  if (!original_url.isValid()) {
    original_url = reply->request().url();
  }

  // In this phase, some part of downloading process is completed.
  QUrl redirection_url = reply->attribute(QNetworkRequest::Attribute::RedirectionTargetAttribute).toUrl();

  if (redirection_url.isValid()) {
    auto redir_number = numberOfRedirections(reply);

    qDebugNN << LOGSEC_NETWORK << "This network request was redirected" << QUOTE_W_SPACE(redir_number) << "times.";

    redir_number++;

    if (redir_number > MAX_NUMBER_OF_REDIRECTIONS) {
      qDebugNN << LOGSEC_NETWORK << "Aborting request due too many redirections.";

      emit completed(redirection_url, QNetworkReply::NetworkError::TooManyRedirectsError, 404, {});
      return;
    }

    // Communication indicates that HTTP redirection is needed.
    // Setup redirection URL and download again.
    QNetworkRequest request = reply->request();

    qWarningNN << LOGSEC_NETWORK << "Network layer indicates HTTP redirection is needed.";
    qWarningNN << LOGSEC_NETWORK << "Origin URL:" << QUOTE_W_SPACE_DOT(request.url().toString());
    qWarningNN << LOGSEC_NETWORK << "Proposed redirection URL:" << QUOTE_W_SPACE_DOT(redirection_url.toString());

    redirection_url = request.url().resolved(redirection_url);

    qWarningNN << LOGSEC_NETWORK << "Resolved redirection URL:" << QUOTE_W_SPACE_DOT(redirection_url.toString());

    if (!hasSameOrigin(request.url(), redirection_url)) {
      request.setRawHeader(HTTP_HEADERS_AUTHORIZATION, {});
      request.setRawHeader(HTTP_HEADERS_COOKIE, {});
      m_targetProtected = false;
      m_targetUsername.clear();
      m_targetPassword.clear();
    }

    request.setUrl(redirection_url);

    m_activeReply->deleteLater();
    m_activeReply = nullptr;

    if (reply_operation == QNetworkAccessManager::GetOperation) {
      runGetRequest(request);
    }
    else if (reply_operation == QNetworkAccessManager::PostOperation) {
      if (m_inputMultipartData == nullptr) {
        runPostRequest(request, m_inputData);
      }
      else {
        runPostRequest(request, m_inputMultipartData);
      }
    }
    else if (reply_operation == QNetworkAccessManager::PutOperation) {
      runPutRequest(request, m_inputData);
    }
    else if (reply_operation == QNetworkAccessManager::DeleteOperation) {
      runDeleteRequest(request);
    }

    if (m_activeReply != nullptr) {
      m_activeReply->setProperty("original_url", original_url);
      setNumberOfRedirections(m_activeReply, redir_number);
    }
  }
  else {
    // No redirection is indicated. Final file is obtained in our "reply" object.
    // Read the data into output buffer.
    if (m_inputMultipartData == nullptr) {
      if (m_outputDevice.isNull()) {
        m_lastOutputData = reply->readAll();
      }
      else {
        writeReplyDataToOutputDevice(reply);
        m_lastOutputData = {};
      }
    }
    else {
      m_lastOutputMultipartData = decodeMultipartAnswer(reply);
    }

    QVariant set_cookies_header = reply->header(QNetworkRequest::KnownHeaders::SetCookieHeader);

    if (m_ignoreCookies) {
      m_lastCookies = {};
    }
    else if (set_cookies_header.isValid()) {
      QList<QNetworkCookie> cookies = set_cookies_header.value<QList<QNetworkCookie>>();

      m_lastCookies = cookies;
    }
    else {
      m_lastCookies = {};
    }

    m_lastUrl = reply->url();
    m_lastContentType = reply->header(QNetworkRequest::KnownHeaders::ContentTypeHeader).toString();
    m_lastOutputError =
      m_outputDeviceWriteFailed ? QNetworkReply::NetworkError::UnknownNetworkError : reply->error();
    m_lastHttpStatusCode = reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();
    m_lastHeaders.clear();

    for (const QNetworkReply::RawHeaderPair& head : reply->rawHeaderPairs()) {
      m_lastHeaders.insert(QString::fromLocal8Bit(head.first).toLower(), head.second);
    }

    // original_url = m_activeReply->property("original_url").toUrl();

    if (m_activeReply != nullptr) {
      m_activeReply->deleteLater();
      m_activeReply = nullptr;
    }

    if (m_inputMultipartData != nullptr) {
      m_inputMultipartData->deleteLater();
    }

    emit completed(original_url, m_lastOutputError, m_lastHttpStatusCode, m_lastOutputData);
  }
}

void Downloader::cancel() {
  if (m_activeReply != nullptr) {
    m_activeReply->abort();
  }
  else {
    cancelGemini();
  }
}

void Downloader::cancelGemini() {
  if (m_activeReply == nullptr) {
    m_geminiTimer->stop();

    if (m_geminiClient->cancelRequest()) {
      emit completed(m_geminiClient->targetUrl(), QNetworkReply::NetworkError::TimeoutError, 408);
    }
  }
}

void Downloader::progressInternal(qint64 bytes_received, qint64 bytes_total) {
  emit progress(bytes_received, bytes_total);
}

void Downloader::readyReadInternal() {
  auto* reply = qobject_cast<QNetworkReply*>(sender());

  if (reply == nullptr || m_outputDevice.isNull()) {
    return;
  }

  const int http_status_code = reply->attribute(QNetworkRequest::Attribute::HttpStatusCodeAttribute).toInt();

  if (reply->attribute(QNetworkRequest::Attribute::RedirectionTargetAttribute).toUrl().isValid() ||
      (http_status_code >= 300 && http_status_code < 400)) {
    reply->readAll();
    return;
  }

  writeReplyDataToOutputDevice(reply);
}

void Downloader::setCustomPropsToReply(QNetworkReply* reply) {
  reply->setProperty("protected", m_targetProtected);
  reply->setProperty("username", m_targetUsername);
  reply->setProperty("password", m_targetPassword);
}

void Downloader::resetOutputDevice() {
  m_outputDevice = nullptr;
  m_outputDeviceWriteFailed = false;
}

void Downloader::writeReplyDataToOutputDevice(QNetworkReply* reply) {
  if (reply == nullptr || m_outputDevice.isNull() || m_outputDeviceWriteFailed) {
    return;
  }

  while (reply->bytesAvailable() > 0) {
    const QByteArray chunk = reply->read(64 * 1024);

    if (chunk.isEmpty()) {
      break;
    }

    if (!m_outputDevice->isWritable() || m_outputDevice->write(chunk) != chunk.size()) {
      m_outputDeviceWriteFailed = true;
      qWarningNN << LOGSEC_NETWORK << "Failed to write downloaded data to output device.";
      reply->abort();
      return;
    }
  }
}

QList<HttpResponse> Downloader::decodeMultipartAnswer(QNetworkReply* reply) {
  QByteArray data = reply->readAll();

  if (data.isEmpty()) {
    return QList<HttpResponse>();
  }

  QString content_type = reply->header(QNetworkRequest::KnownHeaders::ContentTypeHeader).toString();
  QString boundary = content_type.mid(content_type.indexOf(QL1S("boundary=")) + 9);
  QRegularExpression regex(QL1S("--") + boundary + QL1S("(--)?(\\r\\n)?"));
  QStringList list = QString::fromUtf8(data).split(regex, SPLIT_BEHAVIOR::SkipEmptyParts);
  QList<HttpResponse> parts;

  parts.reserve(list.size());

  for (const QString& http_response_str : list) {
    // We separate headers and body.
    HttpResponse new_part;

    static QRegularExpression reg_headers(QSL("\\r\\r?\\n"));
    static QRegularExpression reg_body(QSL("(\\r\\r?\\n){2,}"));
    static QRegularExpression reg_whites(QSL("[\\n\\r]+"));

    int start_of_http = http_response_str.indexOf(QL1S("HTTP/1.1"));
    int start_of_headers = http_response_str.indexOf(reg_headers, start_of_http);
    int start_of_body = http_response_str.indexOf(reg_body, start_of_headers + 2);

    QString body = http_response_str.mid(start_of_body);
    QString headers =
      http_response_str.mid(start_of_headers, start_of_body - start_of_headers).replace(reg_whites, QSL("\n"));
    auto header_lines = headers.split(QL1C('\n'), SPLIT_BEHAVIOR::SkipEmptyParts);

    for (const QString& header_line : std::as_const(header_lines)) {
      int index_colon = header_line.indexOf(QL1C(':'));

      if (index_colon > 0) {
        new_part.appendHeader(header_line.mid(0, index_colon), header_line.mid(index_colon + 2));
      }
    }

    new_part.setBody(body);
    parts.append(new_part);
  }

  return parts;
}

void Downloader::runDeleteRequest(const QNetworkRequest& request) {
  m_activeReply = m_downloadManager->deleteResource(request);
  setCustomPropsToReply(m_activeReply);
  connect(m_activeReply, &QNetworkReply::downloadProgress, this, &Downloader::progressInternal);
  connect(m_activeReply, &QNetworkReply::readyRead, this, &Downloader::readyReadInternal);
  connect(m_activeReply, &QNetworkReply::finished, this, &Downloader::finished);
}

void Downloader::runPutRequest(const QNetworkRequest& request, const QByteArray& data) {
  m_activeReply = m_downloadManager->put(request, data);
  setCustomPropsToReply(m_activeReply);
  connect(m_activeReply, &QNetworkReply::downloadProgress, this, &Downloader::progressInternal);
  connect(m_activeReply, &QNetworkReply::readyRead, this, &Downloader::readyReadInternal);
  connect(m_activeReply, &QNetworkReply::finished, this, &Downloader::finished);
}

void Downloader::runPostRequest(const QNetworkRequest& request, QHttpMultiPart* multipart_data) {
  m_activeReply = m_downloadManager->post(request, multipart_data);
  setCustomPropsToReply(m_activeReply);
  connect(m_activeReply, &QNetworkReply::downloadProgress, this, &Downloader::progressInternal);
  connect(m_activeReply, &QNetworkReply::readyRead, this, &Downloader::readyReadInternal);
  connect(m_activeReply, &QNetworkReply::finished, this, &Downloader::finished);
}

void Downloader::runPostRequest(const QNetworkRequest& request, const QByteArray& data) {
  m_activeReply = m_downloadManager->post(request, data);
  setCustomPropsToReply(m_activeReply);
  connect(m_activeReply, &QNetworkReply::downloadProgress, this, &Downloader::progressInternal);
  connect(m_activeReply, &QNetworkReply::readyRead, this, &Downloader::readyReadInternal);
  connect(m_activeReply, &QNetworkReply::finished, this, &Downloader::finished);
}

void Downloader::runGetRequest(const QNetworkRequest& request) {
  m_activeReply = m_downloadManager->get(request);
  setCustomPropsToReply(m_activeReply);
  connect(m_activeReply, &QNetworkReply::downloadProgress, this, &Downloader::progressInternal);
  connect(m_activeReply, &QNetworkReply::readyRead, this, &Downloader::readyReadInternal);
  connect(m_activeReply, &QNetworkReply::finished, this, &Downloader::finished);
}

QUrl Downloader::lastUrl() const {
  return m_lastUrl;
}

void Downloader::setHttp2Status(NetworkFactory::Http2Status status) {
  m_downloadManager->setSpecificHtpp2Status(status);
}

void Downloader::reloadSettings() {
  m_downloadManager->loadSettings();
}

QNetworkProxy Downloader::proxy() const {
  return m_downloadManager->proxy();
}

const QMap<QString, QString>& Downloader::lastHeaders() const {
  return m_lastHeaders;
}

int Downloader::lastHttpStatusCode() const {
  return m_lastHttpStatusCode;
}

const QList<QNetworkCookie>& Downloader::lastCookies() const {
  return m_lastCookies;
}

const QString& Downloader::lastContentType() const {
  return m_lastContentType;
}

void Downloader::setProxy(const QNetworkProxy& proxy) {
  qDebugNN << LOGSEC_NETWORK << "Setting specific downloader proxy, address:" << QUOTE_W_SPACE_COMMA(proxy.hostName())
           << " type:" << QUOTE_W_SPACE_DOT(proxy.type());

  m_downloadManager->setProxy(proxy);
}

void Downloader::appendRawHeaders(const QList<QPair<QByteArray, QByteArray>>& headers) {
  for (const QPair<QByteArray, QByteArray>& header : headers) {
    appendRawHeader(header.first, header.second);
  }
}

void Downloader::appendRawHeader(const QByteArray& name, const QByteArray& value) {
  if (!value.isEmpty()) {
    m_customHeaders.insert(name, value);
  }
}

QNetworkReply::NetworkError Downloader::lastOutputError() const {
  return m_lastOutputError;
}

const QList<HttpResponse>& Downloader::lastOutputMultipartData() const {
  return m_lastOutputMultipartData;
}

const QByteArray& Downloader::lastOutputData() const {
  return m_lastOutputData;
}
