// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/oauthhttphandler.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <cctype>

#include <QTcpSocket>
#include <QUrlQuery>

OAuthHttpHandler::OAuthHttpHandler(const QString& success_text, QObject* parent)
  : QObject(parent), m_listenAddress(QHostAddress()), m_listenPort(0), m_successText(success_text) {
  connect(&m_httpServer, &QTcpServer::newConnection, this, &OAuthHttpHandler::clientConnected);

  // NOTE: We do not want to start handler immediately, sometimes
  // we want to start it later, perhaps when correct redirect URL/port comes in.
}

OAuthHttpHandler::~OAuthHttpHandler() {
  if (m_httpServer.isListening()) {
    qWarningNN << LOGSEC_OAUTH << "Redirection OAuth handler is listening. Stopping it now.";
    stop();
  }
}

bool OAuthHttpHandler::isListening() const {
  return m_httpServer.isListening();
}

void OAuthHttpHandler::setListenAddressPort(const QString& full_uri) {
  QUrl url = QUrl::fromUserInput(full_uri);
  QHostAddress listen_address;
  quint16 listen_port = quint16(url.port(80));

  if (url.host() == QL1S("localhost")) {
    listen_address = QHostAddress(QHostAddress::SpecialAddress::LocalHost);
  }
  else {
    listen_address = QHostAddress(url.host());
  }

  if (listen_address == m_listenAddress && listen_port == m_listenPort && m_httpServer.isListening()) {
    // NOTE: We do not need to change listener's settings or re-start it.
    return;
  }

  if (m_httpServer.isListening()) {
    qWarningNN << LOGSEC_OAUTH << "Redirection OAuth handler is listening. Stopping it now.";
    stop();
  }

  if (!m_httpServer.listen(listen_address, listen_port)) {
    qCriticalNN << LOGSEC_OAUTH
                << "OAuth redirect handler FAILED TO START TO LISTEN on address"
                << QUOTE_W_SPACE(listen_address.toString())
                << "and port"
                << QUOTE_W_SPACE(listen_port)
                << "with error"
                << QUOTE_W_SPACE_DOT(m_httpServer.errorString());
  }
  else {
    m_listenAddress = listen_address;
    m_listenPort = listen_port;
    m_listenAddressPort = full_uri;

    qDebugNN << LOGSEC_OAUTH
             << "OAuth redirect handler IS LISTENING on address"
             << QUOTE_W_SPACE(m_listenAddress.toString())
             << "and port"
             << QUOTE_W_SPACE_DOT(m_listenPort);
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

    qCriticalNN << LOGSEC_OAUTH
                << "AuthenticationError: " << error << "(" << uri << "): " << description;
    emit authRejected(description, received_state);
  }
  else if (code.isEmpty()) {
    qCriticalNN << LOGSEC_OAUTH
                << "We did not receive authentication code.";
    emit authRejected(QSL("Code not received"), received_state);
  }
  else if (received_state.isEmpty()) {
    qCriticalNN << LOGSEC_OAUTH
                << "State not received.";
    emit authRejected(QSL("State not received"), received_state);
  }
  else {
    emit authGranted(code, received_state);
  }
}

void OAuthHttpHandler::answerClient(QTcpSocket* socket, const QUrl& url) {
  if (!url.path().remove(QL1C('/')).isEmpty()) {
    qCriticalNN << LOGSEC_OAUTH << "Invalid request:" << QUOTE_W_SPACE_DOT(url.toString());
  }
  else {
    QVariantMap received_data;
    const QUrlQuery query(url.query());
    const auto items = query.queryItems();

    for (const auto& item : items) {
      received_data.insert(item.first, item.second);
    }

    handleRedirection(received_data);

    const QString html = QSL("<html><head><title>") +
                         qApp->applicationName() +
                         QSL("</title></head><body>") +
                         m_successText +
                         QSL("</body></html>");
    const QByteArray html_utf = html.toUtf8();
    const QByteArray html_size = QString::number(html_utf.size()).toLocal8Bit();
    const QByteArray reply_message = QByteArrayLiteral("HTTP/1.0 200 OK \r\n"
                                                       "Content-Type: text/html; charset=\"utf-8\"\r\n"
                                                       "Content-Length: ") + html_size +
                                     QByteArrayLiteral("\r\n\r\n") + html_utf;

    socket->write(reply_message);
  }

  socket->disconnectFromHost();
}

void OAuthHttpHandler::readReceivedData(QTcpSocket* socket) {
  if (!m_connectedClients.contains(socket)) {
    m_connectedClients[socket].m_address = QSL("http://") + m_httpServer.serverAddress().toString();
    m_connectedClients[socket].m_port = m_httpServer.serverPort();
  }

  QHttpRequest* request = &m_connectedClients[socket];
  bool error = false;

  if (Q_LIKELY(request->m_state == QHttpRequest::State::ReadingMethod)) {
    if (Q_UNLIKELY(error = !request->readMethod(socket))) {
      qWarningNN << LOGSEC_OAUTH << "Invalid method.";
    }
  }

  if (Q_LIKELY(!error && request->m_state == QHttpRequest::State::ReadingUrl)) {
    if (Q_UNLIKELY(error = !request->readUrl(socket))) {
      qWarningNN << LOGSEC_OAUTH << "Invalid URL.";
    }
  }

  if (Q_LIKELY(!error && request->m_state == QHttpRequest::State::ReadingStatus)) {
    if (Q_UNLIKELY(error = !request->readStatus(socket))) {
      qWarningNN << LOGSEC_OAUTH << "Invalid status.";
    }
  }

  if (Q_LIKELY(!error && request->m_state == QHttpRequest::State::ReadingHeader)) {
    if (Q_UNLIKELY(error = !request->readHeader(socket))) {
      qWarningNN << LOGSEC_OAUTH << "Invalid header.";
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

QHostAddress OAuthHttpHandler::listenAddress() const {
  return m_listenAddress;
}

QString OAuthHttpHandler::listenAddressPort() const {
  return m_listenAddressPort;
}

quint16 OAuthHttpHandler::listenPort() const {
  return m_listenPort;
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
      qWarningNN << LOGSEC_OAUTH << "Invalid operation:" << QUOTE_W_SPACE_DOT(m_fragment.data());
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
      qWarningNN << LOGSEC_OAUTH << "Invalid URL path" << QUOTE_W_SPACE_DOT(m_fragment);
      return false;
    }

    m_url.setUrl(m_address + QString::number(m_port) + QString::fromUtf8(m_fragment));
    m_state = State::ReadingStatus;

    if (!m_url.isValid()) {
      qWarningNN << LOGSEC_OAUTH << "Invalid URL" << QUOTE_W_SPACE_DOT(m_fragment);
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
      qWarningNN << LOGSEC_OAUTH << "Invalid version";
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

void OAuthHttpHandler::stop() {
  m_httpServer.close();
  m_connectedClients.clear();
  m_listenAddress = QHostAddress();
  m_listenPort = 0;
  m_listenAddressPort = QString();

  qDebugNN << LOGSEC_OAUTH << "Stopped redirection handler.";
}
