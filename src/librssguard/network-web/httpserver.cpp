// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/httpserver.h"

#include "definitions/definitions.h"

#include <QDateTime>
#include <QRegularExpression>

HttpServer::HttpServer(QObject* parent) : QObject(parent), m_listenAddress(QHostAddress()), m_listenPort(0) {
  connect(&m_httpServer, &QTcpServer::newConnection, this, &HttpServer::clientConnected);

  // NOTE: We do not want to start handler immediately, sometimes
  // we want to start it later, perhaps when correct redirect URL/port comes in.
}

HttpServer::~HttpServer() {
  if (m_httpServer.isListening()) {
    qWarningNN << LOGSEC_NETWORK << "Redirection OAuth handler is listening. Stopping it now.";
    stop();
  }
}

bool HttpServer::isListening() const {
  return m_httpServer.isListening();
}

void HttpServer::setListenAddressPort(const QString& full_uri, bool start_handler) {
  QUrl url = QUrl::fromUserInput(full_uri);
  QHostAddress listen_address;
  quint16 listen_port = quint16(url.port(80));

  if (url.host() == QL1S("localhost")) {
    listen_address = QHostAddress(QHostAddress::SpecialAddress::LocalHost);
  }
  else {
    listen_address = QHostAddress(url.host());
  }

  if (listen_address == m_listenAddress && listen_port == m_listenPort && start_handler == m_httpServer.isListening()) {
    // NOTE: We do not need to change listener's settings or re-start it.
    return;
  }

  if (m_httpServer.isListening()) {
    qWarningNN << LOGSEC_NETWORK << "Redirection OAuth handler is listening. Stopping it now.";
    stop();
  }

  m_listenAddress = listen_address;
  m_listenPort = listen_port;
  m_listenAddressPort = full_uri;

  if (!start_handler) {
    qDebugNN << LOGSEC_NETWORK << "User does not want handler to be running.";
    return;
  }

  if (!m_httpServer.listen(listen_address, listen_port)) {
    qCriticalNN << LOGSEC_NETWORK << "OAuth redirect handler FAILED TO START TO LISTEN on address"
                << QUOTE_W_SPACE(listen_address.toString()) << "and port" << QUOTE_W_SPACE(listen_port) << "with error"
                << QUOTE_W_SPACE_DOT(m_httpServer.errorString());
  }
  else {
    qDebugNN << LOGSEC_NETWORK << "OAuth redirect handler IS LISTENING on address"
             << QUOTE_W_SPACE(m_listenAddress.toString()) << "and port" << QUOTE_W_SPACE_DOT(m_listenPort);
  }
}

void HttpServer::clientConnected() {
  QTcpSocket* socket = m_httpServer.nextPendingConnection();

  QObject::connect(socket, &QTcpSocket::disconnected, this, [this, socket]() {
    m_connectedClients.remove(socket);
    socket->deleteLater();
  });
  QObject::connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
    readReceivedData(socket);
  });
}

void HttpServer::readReceivedData(QTcpSocket* socket) {
  if (!m_connectedClients.contains(socket)) {
    m_connectedClients[socket].m_address = m_httpServer.serverAddress().toString();
    m_connectedClients[socket].m_port = m_httpServer.serverPort();
  }

  HttpRequest* request = &m_connectedClients[socket];
  bool error = false;

  if (Q_LIKELY(request->m_state == HttpRequest::State::ReadingMethod)) {
    if (Q_UNLIKELY(error = !request->readMethod(socket))) {
      qWarningNN << LOGSEC_NETWORK << "Invalid method.";
    }
  }

  if (Q_LIKELY(!error && request->m_state == HttpRequest::State::ReadingUrl)) {
    if (Q_UNLIKELY(error = !request->readUrl(socket))) {
      qWarningNN << LOGSEC_NETWORK << "Invalid URL.";
    }
  }

  if (Q_LIKELY(!error && request->m_state == HttpRequest::State::ReadingStatus)) {
    if (Q_UNLIKELY(error = !request->readStatus(socket))) {
      qWarningNN << LOGSEC_NETWORK << "Invalid status.";
    }
  }

  if (Q_LIKELY(!error && request->m_state == HttpRequest::State::ReadingHeader)) {
    if (Q_UNLIKELY(error = !request->readHeader(socket))) {
      qWarningNN << LOGSEC_NETWORK << "Invalid header.";
    }
  }

  if (error) {
    socket->disconnectFromHost();
    m_connectedClients.remove(socket);
  }
  else if (request->m_state == HttpRequest::State::ReadingBody) {
    answerClient(socket, *request);
    m_connectedClients.remove(socket);
  }
}

QHostAddress HttpServer::listenAddress() const {
  return m_listenAddress;
}

QString HttpServer::listenAddressPort() const {
  return m_listenAddressPort;
}

quint16 HttpServer::listenPort() const {
  return m_listenPort;
}

bool HttpServer::HttpRequest::readMethod(QTcpSocket* socket) {
  bool finished = false;

  while ((socket->bytesAvailable() != 0) && !finished) {
    const auto c = socket->read(1).at(0);

    if ((std::isupper(c) != 0) && m_fragment.size() < 7) {
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
    else if (m_fragment == "OPTIONS") {
      m_method = Method::Options;
    }
    else {
      qWarningNN << LOGSEC_NETWORK << "Invalid operation:" << QUOTE_W_SPACE_DOT(m_fragment.data());
    }

    m_state = State::ReadingUrl;
    m_fragment.clear();

    return m_method != Method::Unknown;
  }

  return true;
}

bool HttpServer::HttpRequest::readUrl(QTcpSocket* socket) {
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
      qWarningNN << LOGSEC_NETWORK << "Invalid URL path" << QUOTE_W_SPACE_DOT(m_fragment);
      return false;
    }

    QUrl base_url;

    base_url.setScheme(QSL("http"));
    base_url.setHost(m_address);
    base_url.setPort(m_port);

    m_url = base_url.resolved(QUrl::fromEncoded(m_fragment));
    m_state = State::ReadingStatus;

    if (!m_url.isValid()) {
      qWarningNN << LOGSEC_NETWORK << "Invalid URL" << QUOTE_W_SPACE_DOT(m_fragment);
      return false;
    }

    m_fragment.clear();
    return true;
  }

  return true;
}

bool HttpServer::HttpRequest::readStatus(QTcpSocket* socket) {
  bool finished = false;

  while ((socket->bytesAvailable() != 0) && !finished) {
    m_fragment += socket->read(1);

    if (m_fragment.endsWith("\r\n")) {
      finished = true;
      m_fragment.resize(m_fragment.size() - 2);
    }
  }

  if (finished) {
    static const QRegularExpression version_pattern(QSL("^HTTP/(\\d+)\\.(\\d+)$"));
    const auto match = version_pattern.match(QString::fromLatin1(m_fragment));
    bool major_ok;
    bool minor_ok;
    const uint major = match.captured(1).toUInt(&major_ok);
    const uint minor = match.captured(2).toUInt(&minor_ok);

    if (!match.hasMatch() || !major_ok || !minor_ok || major > 255 || minor > 255) {
      qWarningNN << LOGSEC_NETWORK << "Invalid HTTP version";
      return false;
    }

    m_version = qMakePair(quint8(major), quint8(minor));
    m_state = State::ReadingHeader;
    m_fragment.clear();
  }

  return true;
}

bool HttpServer::HttpRequest::readHeader(QTcpSocket* socket) {
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

  return true;
}

void HttpServer::stop() {
  m_httpServer.close();
  m_connectedClients.clear();
  m_listenAddress = QHostAddress();
  m_listenPort = 0;
  m_listenAddressPort = QString();

  qDebugNN << LOGSEC_NETWORK << "Stopped redirection handler.";
}
