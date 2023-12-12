// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/httpserver.h"

#include "definitions/definitions.h"

#include <QDateTime>

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

QByteArray HttpServer::generateHttpAnswer(int http_code,
                                          const QList<HttpHeader>& headers,
                                          const QByteArray& body) const {
  QList<HttpHeader> my_headers = headers;
  QByteArray answer = QSL("HTTP/1.0 %1  \r\n").arg(http_code).toLocal8Bit();
  int body_length = body.size();

  // Append body length.
  if (body_length > 0) {
    my_headers.append({QSL("Content-Length"), QString::number(body_length)});
  }

  // Append server ID and other common headers.
  my_headers.append({QSL("Date"), QDateTime::currentDateTimeUtc().toString(Qt::DateFormat::RFC2822Date)});
  my_headers.append({QSL("Server"), QSL(APP_LONG_NAME)});

  for (const HttpHeader& header : my_headers) {
    answer.append(QSL("%1: %2\r\n").arg(header.m_name, header.m_value).toLocal8Bit());
  }

  answer.append(QSL("\r\n").toLocal8Bit());

  if (body_length > 0) {
    answer.append(body);
  }

  return answer;
}

void HttpServer::clientConnected() {
  QTcpSocket* socket = m_httpServer.nextPendingConnection();

  QObject::connect(socket, &QTcpSocket::disconnected, socket, &QTcpSocket::deleteLater);
  QObject::connect(socket, &QTcpSocket::readyRead, [this, socket]() {
    readReceivedData(socket);
  });
}

void HttpServer::readReceivedData(QTcpSocket* socket) {
  if (!m_connectedClients.contains(socket)) {
    m_connectedClients[socket].m_address = QSL(URI_SCHEME_HTTP) + m_httpServer.serverAddress().toString();
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
  else if (!request->m_url.isEmpty()) {
    Q_ASSERT(request->m_state != HttpRequest::State::ReadingUrl);

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

    m_url.setUrl(m_address + QString::number(m_port) + QString::fromUtf8(m_fragment));
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
    if ((std::isdigit(m_fragment.at(m_fragment.size() - 3)) == 0) ||
        (std::isdigit(m_fragment.at(m_fragment.size() - 1)) == 0)) {
      qWarningNN << LOGSEC_NETWORK << "Invalid version";
      return false;
    }

    m_version = qMakePair(m_fragment.at(m_fragment.size() - 3) - '0', m_fragment.at(m_fragment.size() - 1) - '0');
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

  return false;
}

void HttpServer::stop() {
  m_httpServer.close();
  m_connectedClients.clear();
  m_listenAddress = QHostAddress();
  m_listenPort = 0;
  m_listenAddressPort = QString();

  qDebugNN << LOGSEC_NETWORK << "Stopped redirection handler.";
}
