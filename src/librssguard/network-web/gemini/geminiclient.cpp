#include "network-web/gemini/geminiclient.h"

#include <cassert>

#include <QDebug>
#include <QRegExp>
#include <QSslConfiguration>
#include <QUrl>

bool CryptoIdentity::isHostFiltered(const QUrl& url) const {
  if (this->host_filter.isEmpty())
    return false;

  QString url_text = url.toString(QUrl::FullyEncoded);

  QRegExp pattern{this->host_filter, Qt::CaseInsensitive, QRegExp::Wildcard};

  return not pattern.exactMatch(url_text);
}

bool CryptoIdentity::isAutomaticallyEnabledOn(const QUrl& url) const {
  if (this->host_filter.isEmpty())
    return false;
  if (not this->auto_enable)
    return false;

  QString url_text = url.toString(QUrl::FullyEncoded);

  QRegExp pattern{this->host_filter, Qt::CaseInsensitive, QRegExp::Wildcard};

  return pattern.exactMatch(url_text);
}

GeminiClient::GeminiClient(QObject* parent) : QObject(parent) {
  connect(&socket, &QSslSocket::encrypted, this, &GeminiClient::socketEncrypted);
  connect(&socket, &QSslSocket::readyRead, this, &GeminiClient::socketReadyRead);
  connect(&socket, &QSslSocket::disconnected, this, &GeminiClient::socketDisconnected);
  //    connect(&socket, &QSslSocket::stateChanged, [](QSslSocket::SocketState state) {
  //        qDebug() << "Socket state changed to " << state;
  //    });
  connect(&socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors), this, &GeminiClient::sslErrors);

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  connect(&socket, &QTcpSocket::errorOccurred, this, &GeminiClient::socketError);
#else
  connect(&socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, &GeminiClient::socketError);
#endif

  // States
  connect(&socket, &QAbstractSocket::hostFound, this, [this]() {
    emit this->requestStateChange(RequestState::HostFound);
  });
  connect(&socket, &QAbstractSocket::connected, this, [this]() {
    emit this->requestStateChange(RequestState::Connected);
  });
  connect(&socket, &QAbstractSocket::disconnected, this, [this]() {
    emit this->requestStateChange(RequestState::None);
  });
  emit this->requestStateChange(RequestState::None);
}

GeminiClient::~GeminiClient() {
  is_receiving_body = false;
}

bool GeminiClient::supportsScheme(const QString& scheme) const {
  return (scheme == "gemini");
}

bool GeminiClient::startRequest(const QUrl& url, RequestOptions options) {
  if (url.scheme() != "gemini")
    return false;

  // qDebug() << "start request" << url;

  if (socket.state() != QTcpSocket::UnconnectedState) {
    socket.disconnectFromHost();
    socket.close();
    if (not socket.waitForDisconnected(1500))
      return false;
  }

  emit this->requestStateChange(RequestState::Started);

  this->is_error_state = false;

  this->options = options;

  QSslConfiguration ssl_config = socket.sslConfiguration();
  ssl_config.setProtocol(QSsl::TlsV1_2OrLater);
  /*
  if (not kristall::globals().trust.gemini.enable_ca)
    ssl_config.setCaCertificates(QList<QSslCertificate>{});
  else
  */
  ssl_config.setCaCertificates(QSslConfiguration::systemCaCertificates());
  /*
   */

  socket.setSslConfiguration(ssl_config);

  socket.connectToHostEncrypted(url.host(), url.port(1965));

  this->buffer.clear();
  this->body.clear();
  this->is_receiving_body = false;
  this->suppress_socket_tls_error = true;

  if (not socket.isOpen())
    return false;

  target_url = url;
  mime_type = "<invalid>";

  return true;
}

bool GeminiClient::isInProgress() const {
  return (socket.state() != QTcpSocket::UnconnectedState);
}

bool GeminiClient::cancelRequest() {
  // qDebug() << "cancel request" << isInProgress();
  if (isInProgress()) {
    this->is_receiving_body = false;
    this->socket.disconnectFromHost();
    this->buffer.clear();
    this->body.clear();
    if (socket.state() != QTcpSocket::UnconnectedState) {
      socket.disconnectFromHost();
    }
    this->socket.waitForDisconnected(500);
    this->socket.close();
    bool success = not isInProgress();
    // qDebug() << "cancel success" << success;
    return success;
  }
  else {
    return true;
  }
}

bool GeminiClient::enableClientCertificate(const CryptoIdentity& ident) {
  this->socket.setLocalCertificate(ident.certificate);
  this->socket.setPrivateKey(ident.private_key);
  return true;
}

void GeminiClient::disableClientCertificate() {
  this->socket.setLocalCertificate(QSslCertificate{});
  this->socket.setPrivateKey(QSslKey{});
}

void GeminiClient::emitNetworkError(QAbstractSocket::SocketError error_code, const QString& textual_description) {
  NetworkError network_error = UnknownError;

  switch (error_code) {
    case QAbstractSocket::ConnectionRefusedError:
      network_error = ConnectionRefused;
      break;
    case QAbstractSocket::HostNotFoundError:
      network_error = HostNotFound;
      break;
    case QAbstractSocket::SocketTimeoutError:
      network_error = Timeout;
      break;
    case QAbstractSocket::SslHandshakeFailedError:
      network_error = TlsFailure;
      break;
    case QAbstractSocket::SslInternalError:
      network_error = TlsFailure;
      break;
    case QAbstractSocket::SslInvalidUserDataError:
      network_error = TlsFailure;
      break;
    default:
      qDebug() << "unhandled network error:" << error_code;
      break;
  }

  emit this->networkError(network_error, textual_description);
}

void GeminiClient::socketEncrypted() {
  emit this->hostCertificateLoaded(this->socket.peerCertificate());

  QString request = target_url.toString(QUrl::FormattingOptions(QUrl::FullyEncoded)) + "\r\n";

  QByteArray request_bytes = request.toUtf8();

  qint64 offset = 0;
  while (offset < request_bytes.size()) {
    const auto len = socket.write(request_bytes.constData() + offset, request_bytes.size() - offset);
    if (len <= 0) {
      socket.close();
      return;
    }
    offset += len;
  }
}

void GeminiClient::socketReadyRead() {
  if (this->is_error_state) // don't do any further
    return;
  QByteArray response = socket.readAll();

  if (is_receiving_body) {
    body.append(response);
    emit this->requestProgress(body.size());
  }
  else {
    for (int i = 0; i < response.size(); i++) {
      if (response[i] == '\n') {
        buffer.append(response.data(), i);
        body.append(response.data() + i + 1, response.size() - i - 1);

        // "XY " <META> <CR> <LF>
        if (buffer.size() < 4) { // we allow an empty <META>
          socket.close();
          qDebug() << buffer;
          emit networkError(ProtocolViolation, QObject::tr("Line is too short for valid protocol"));
          return;
        }
        if (buffer.size() >= 1200) {
          emit networkError(ProtocolViolation, QObject::tr("response too large!"));
          socket.close();
        }
        if (buffer[buffer.size() - 1] != '\r') {
          socket.close();
          qDebug() << buffer;
          emit networkError(ProtocolViolation, QObject::tr("Line does not end with <CR> <LF>"));
          return;
        }
        if (not isdigit(buffer[0])) {
          socket.close();
          qDebug() << buffer;
          emit networkError(ProtocolViolation, QObject::tr("First character is not a digit."));
          return;
        }
        if (not isdigit(buffer[1])) {
          socket.close();
          qDebug() << buffer;
          emit networkError(ProtocolViolation, QObject::tr("Second character is not a digit."));
          return;
        }
        // TODO: Implement stricter version
        // if(buffer[2] != ' ') {
        if (not isspace(buffer[2])) {
          socket.close();
          qDebug() << buffer;
          emit networkError(ProtocolViolation, QObject::tr("Third character is not a space."));
          return;
        }

        QString meta = QString::fromUtf8(buffer.data() + 3, buffer.size() - 4);

        int primary_code = buffer[0] - '0';
        int secondary_code = buffer[1] - '0';

        qDebug() << primary_code << secondary_code << meta;

        // We don't need to receive any data after that.
        if (primary_code != 2)
          socket.close();

        switch (primary_code) {
          case 1: // requesting input
            switch (secondary_code) {
              case 1:
                emit inputRequired(meta, true);
                break;
              case 0:
              default:
                emit inputRequired(meta, false);
            }
            return;

          case 2: // success
            is_receiving_body = true;
            mime_type = meta;
            return;

          case 3: { // redirect
            QUrl new_url(meta);
            if (new_url.isValid()) {
              if (new_url.isRelative())
                new_url = target_url.resolved(new_url);
              assert(not new_url.isRelative());

              emit redirected(new_url, (secondary_code == 1));
            }
            else {
              emit networkError(ProtocolViolation, QObject::tr("Invalid URL for redirection!"));
            }
            return;
          }

          case 4: { // temporary failure
            NetworkError type = UnknownError;
            switch (secondary_code) {
              case 1:
                type = InternalServerError;
                break;
              case 2:
                type = InternalServerError;
                break;
              case 3:
                type = InternalServerError;
                break;
              case 4:
                type = UnknownError;
                break;
            }
            emit networkError(type, meta);
            return;
          }

          case 5: { // permanent failure
            NetworkError type = UnknownError;
            switch (secondary_code) {
              case 1:
                type = ResourceNotFound;
                break;
              case 2:
                type = ResourceNotFound;
                break;
              case 3:
                type = ProxyRequest;
                break;
              case 9:
                type = BadRequest;
                break;
            }
            emit networkError(type, meta);
            return;
          }

          case 6: // client certificate required
            switch (secondary_code) {
              case 0:
                emit certificateRequired(meta);
                return;

              case 1:
                emit networkError(Unauthorized, meta);
                return;

              default:
              case 2:
                emit networkError(InvalidClientCertificate, meta);
                return;
            }
            return;

          default:
            emit networkError(ProtocolViolation, QObject::tr("Unspecified status code used!"));
            return;
        }

        assert(false and "unreachable");
      }
    }
    if ((buffer.size() + response.size()) >= 1200) {
      emit networkError(ProtocolViolation, QObject::tr("META too large!"));
      socket.close();
    }
    buffer.append(response);
  }
}

void GeminiClient::socketDisconnected() {
  if (this->is_receiving_body and not this->is_error_state) {
    body.append(socket.readAll());
    emit requestComplete(body, mime_type);
  }
}

void GeminiClient::sslErrors(const QList<QSslError>& errors) {
  emit this->hostCertificateLoaded(this->socket.peerCertificate());

  if (options & IgnoreTlsErrors) {
    socket.ignoreSslErrors(errors);
    return;
  }

  QList<QSslError> remaining_errors = errors;
  QList<QSslError> ignored_errors;

  int i = 0;
  while (i < remaining_errors.size()) {
    const auto& err = remaining_errors.at(i);

    bool ignore = false;

    /*
     */
    ignore = true;
    /*
    if (SslTrust::isTrustRelated(err.error())) {
      switch (kristall::globals().trust.gemini.getTrust(target_url, socket.peerCertificate())) {
        case SslTrust::Trusted:
          ignore = true;
          break;
        case SslTrust::Untrusted:
          this->is_error_state = true;
          this->suppress_socket_tls_error = true;
          emit this->networkError(UntrustedHost, toFingerprintString(socket.peerCertificate()));
          return;
        case SslTrust::Mistrusted:
          this->is_error_state = true;
          this->suppress_socket_tls_error = true;
          emit this->networkError(MistrustedHost, toFingerprintString(socket.peerCertificate()));
          return;
      }
    }

    else */
    if (err.error() == QSslError::UnableToVerifyFirstCertificate) {
      ignore = true;
    }

    if (ignore) {
      ignored_errors.append(err);
      remaining_errors.removeAt(0);
    }
    else {
      i += 1;
    }
  }

  socket.ignoreSslErrors(ignored_errors);

  qDebug() << "ignoring" << ignored_errors.size() << "out of" << errors.size();

  for (const auto& error : remaining_errors) {
    qWarning() << int(error.error()) << error.errorString();
  }

  if (remaining_errors.size() > 0) {
    emit this->networkError(TlsFailure, remaining_errors.first().errorString());
  }
}

void GeminiClient::socketError(QAbstractSocket::SocketError socketError) {
  // When remote host closes TLS session, the client closes the socket.
  // This is more sane then erroring out here as it's a perfectly legal
  // state and we know the TLS connection has ended.
  if (socketError == QAbstractSocket::RemoteHostClosedError) {
    socket.close();
    return;
  }

  this->is_error_state = true;
  if (not this->suppress_socket_tls_error) {
    this->emitNetworkError(socketError, socket.errorString());
  }
}
