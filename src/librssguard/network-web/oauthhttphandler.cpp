// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/oauthhttphandler.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <cctype>

#include <QTcpSocket>
#include <QUrlQuery>

OAuthHttpHandler::OAuthHttpHandler(QObject* parent) : QObject(parent) {
  m_text = tr("You can close this window now. Go back to %1").arg(APP_NAME);

  connect(&m_httpServer, &QTcpServer::newConnection, this, &OAuthHttpHandler::clientConnected);

  if (!m_httpServer.listen(m_listenAddress, 13377)) {
    qCritical("OAuth HTTP handler: Failed to start listening.");
  }
}

OAuthHttpHandler::~OAuthHttpHandler() {
  if (m_httpServer.isListening()) {
    m_httpServer.close();
  }
}

void OAuthHttpHandler::clientConnected() {
  QTcpSocket* socket = m_httpServer.nextPendingConnection();

  QObject::connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
  QObject::connect(socket, &QTcpSocket::readyRead, [this, socket]() {
    readReceivedData(socket);
  });
}

void OAuthHttpHandler::handleRedirection(const QVariantMap& data) {
  if (data.isEmpty()) {
    return;
  }

  const QString error = data.value(QSL("error")).toString();
  const QString code = data.value(QSL("code")).toString();
  const QString received_state = data.value(QSL("state")).toString();

  if (error.size() != 0) {
    const QString uri = data.value(QSL("error_uri")).toString();
    const QString description = data.value(QSL("error_description")).toString();

    qWarning("OAuth HTTP handler: AuthenticationError: %s(%s): %s", qPrintable(error), qPrintable(uri), qPrintable(description));
    emit authRejected(description, received_state);
  }
  else if (code.isEmpty()) {
    qWarning("OAuth HTTP handler: AuthenticationError: Code not received");
    emit authRejected(QSL("AuthenticationError: Code not received"), received_state);
  }
  else if (received_state.isEmpty()) {
    qWarning("OAuth HTTP handler: State not received");
    emit authRejected(QSL("State not received"), received_state);
  }
  else {
    emit authGranted(code, received_state);
  }
}

void OAuthHttpHandler::answerClient(QTcpSocket* socket, const QUrl& url) {
  if (!url.path().remove(QL1C('/')).isEmpty()) {
    qWarning("OAuth HTTP handler: Invalid request: %s", qPrintable(url.toString()));
  }
  else {
    QVariantMap received_data;
    const QUrlQuery query(url.query());
    const auto items = query.queryItems();

    for (const auto & item : items) {
      received_data.insert(item.first, item.second);
    }

    handleRedirection(received_data);

    const QByteArray html = QByteArrayLiteral("<html><head><title>") +
                            qApp->applicationName().toUtf8() +
                            QByteArrayLiteral("</title></head><body>") +
                            m_text.toUtf8() +
                            QByteArrayLiteral("</body></html>");
    const QByteArray html_size = QString::number(html.size()).toUtf8();
    const QByteArray reply_message = QByteArrayLiteral("HTTP/1.0 200 OK \r\n"
                                                       "Content-Type: text/html; "
                                                       "charset=\"utf-8\"\r\n"
                                                       "Content-Length: ") + html_size +
                                     QByteArrayLiteral("\r\n\r\n") + html;

    socket->write(reply_message);
  }

  socket->disconnectFromHost();
}

void OAuthHttpHandler::readReceivedData(QTcpSocket* socket) {
  if (!m_connectedClients.contains(socket)) {
    m_connectedClients[socket].m_port = m_httpServer.serverPort();
  }

  QHttpRequest* request = &m_connectedClients[socket];
  bool error = false;

  if (Q_LIKELY(request->m_state == QHttpRequest::State::ReadingMethod)) {
    if (Q_UNLIKELY(error = !request->readMethod(socket))) {
      qWarning("OAuth HTTP handler: Invalid dethod");
    }
  }

  if (Q_LIKELY(!error && request->m_state == QHttpRequest::State::ReadingUrl)) {
    if (Q_UNLIKELY(error = !request->readUrl(socket))) {
      qWarning("OAuth HTTP handler: Invalid URL");
    }
  }

  if (Q_LIKELY(!error && request->m_state == QHttpRequest::State::ReadingStatus)) {
    if (Q_UNLIKELY(error = !request->readStatus(socket))) {
      qWarning("OAuth HTTP handler: Invalid status");
    }
  }

  if (Q_LIKELY(!error && request->m_state == QHttpRequest::State::ReadingHeader)) {
    if (Q_UNLIKELY(error = !request->readHeader(socket))) {
      qWarning("OAuth HTTP handler: Invalid header");
    }
  }

  if (error) {
    socket->disconnectFromHost();
    m_connectedClients.remove(socket);
  }
  else if (!request->m_url.isEmpty()) {
    Q_ASSERT(request->m_state != QHttpRequest::State::ReadingUrl);

    answerClient(socket, request->m_url);
    m_connectedClients.remove(socket);
  }
}

bool OAuthHttpHandler::QHttpRequest::readMethod(QTcpSocket* socket) {
  bool finished = false;

  while ((socket->bytesAvailable() != 0) && !finished) {
    const auto c = socket->read(1).at(0);

    if ((std::isupper(c) != 0) && m_fragment.size() < 6) {
      m_fragment += c;
    }
    else {
      finished = true;
    }
  }

  if (finished) {
    if (m_fragment == "HEAD") {
      m_method = Method::Head;
    }
    else if (m_fragment == "GET") {
      m_method = Method::Get;
    }
    else if (m_fragment == "PUT") {
      m_method = Method::Put;
    }
    else if (m_fragment == "POST") {
      m_method = Method::Post;
    }
    else if (m_fragment == "DELETE") {
      m_method = Method::Delete;
    }
    else {
      qWarning("OAuth HTTP handler: Invalid operation %s", m_fragment.data());
    }

    m_state = State::ReadingUrl;
    m_fragment.clear();

    return m_method != Method::Unknown;
  }

  return true;
}

bool OAuthHttpHandler::QHttpRequest::readUrl(QTcpSocket* socket) {
  bool finished = false;

  while ((socket->bytesAvailable() != 0) && !finished) {
    const auto c = socket->read(1).at(0);

    if (std::isspace(c) != 0) {
      finished = true;
    }
    else {
      m_fragment += c;
    }
  }

  if (finished) {
    if (!m_fragment.startsWith("/")) {
      qWarning("OAuth HTTP handler: Invalid URL path %s", m_fragment.constData());
      return false;
    }

    m_url.setUrl(QStringLiteral("http://localhost:") + QString::number(m_port) + QString::fromUtf8(m_fragment));
    m_state = State::ReadingStatus;

    if (!m_url.isValid()) {
      qWarning("OAuth HTTP handler: Invalid URL %s", m_fragment.constData());
      return false;
    }

    m_fragment.clear();
    return true;
  }

  return true;
}

bool OAuthHttpHandler::QHttpRequest::readStatus(QTcpSocket* socket) {
  bool finished = false;

  while ((socket->bytesAvailable() != 0) && !finished) {
    m_fragment += socket->read(1);

    if (m_fragment.endsWith("\r\n")) {
      finished = true;
      m_fragment.resize(m_fragment.size() - 2);
    }
  }

  if (finished) {
    if ((std::isdigit(m_fragment.at(m_fragment.size() - 3)) == 0) || (std::isdigit(m_fragment.at(m_fragment.size() - 1)) == 0)) {
      qWarning("OAuth HTTP handler: Invalid version");
      return false;
    }

    m_version = qMakePair(m_fragment.at(m_fragment.size() - 3) - '0', m_fragment.at(m_fragment.size() - 1) - '0');
    m_state = State::ReadingHeader;
    m_fragment.clear();
  }

  return true;
}

bool OAuthHttpHandler::QHttpRequest::readHeader(QTcpSocket* socket) {
  while (socket->bytesAvailable() != 0) {
    m_fragment += socket->read(1);

    if (m_fragment.endsWith("\r\n")) {
      if (m_fragment == "\r\n") {
        m_state = State::ReadingBody;
        m_fragment.clear();
        return true;
      }
      else {
        m_fragment.chop(2);
        const int index = m_fragment.indexOf(':');

        if (index == -1) {
          return false;
        }

        const QByteArray key = m_fragment.mid(0, index).trimmed();
        const QByteArray value = m_fragment.mid(index + 1).trimmed();

        m_headers.insert(key, value);
        m_fragment.clear();
      }
    }
  }

  return false;
}
