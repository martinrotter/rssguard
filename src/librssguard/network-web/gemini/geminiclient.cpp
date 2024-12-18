// For license of this file, see <project-root-folder>/LICENSE.md.
//
// This file is heavily inspired by https://github.com/ikskuh/kristall.

#include "network-web/gemini/geminiclient.h"

#include "definitions/definitions.h"

#include <cassert>

#include <QDebug>
#include <QRegularExpression>
#include <QSslConfiguration>
#include <QUrl>

bool CryptoIdentity::isValid() const {
  return !certificate.isNull() && !private_key.isNull();
}

bool CryptoIdentity::isHostFiltered(const QUrl& url) const {
  if (host_filter.isEmpty()) {
    return false;
  }

  QString url_text = url.toString(QUrl::FullyEncoded);
  QRegularExpression pattern(QRegularExpression::wildcardToRegularExpression(host_filter),
                             QRegularExpression::PatternOption::CaseInsensitiveOption);

  return !pattern.match(url_text).hasMatch();
}

bool CryptoIdentity::isAutomaticallyEnabledOn(const QUrl& url) const {
  if (host_filter.isEmpty()) {
    return false;
  }

  if (!auto_enable) {
    return false;
  }

  QString url_text = url.toString(QUrl::FullyEncoded);
  QRegularExpression pattern(QRegularExpression::wildcardToRegularExpression(host_filter),
                             QRegularExpression::PatternOption::CaseInsensitiveOption);

  return pattern.match(url_text).hasMatch();
}

GeminiClient::GeminiClient(QObject* parent) : QObject(parent) {
  connect(&m_socket, &QSslSocket::encrypted, this, &GeminiClient::socketEncrypted);
  connect(&m_socket, &QSslSocket::readyRead, this, &GeminiClient::socketReadyRead);
  connect(&m_socket, &QSslSocket::disconnected, this, &GeminiClient::socketDisconnected);
  connect(&m_socket, QOverload<const QList<QSslError>&>::of(&QSslSocket::sslErrors), this, &GeminiClient::sslErrors);

  //    connect(&socket, &QSslSocket::stateChanged, [](QSslSocket::SocketState state) {
  //        qDebug() << "Socket state changed to " << state;
  //    });

#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  connect(&m_socket, &QTcpSocket::errorOccurred, this, &GeminiClient::socketError);
#else
  connect(&m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error), this, &GeminiClient::socketError);
#endif

  // States
  connect(&m_socket, &QAbstractSocket::hostFound, this, [this]() {
    emit this->requestStateChange(RequestState::HostFound);
  });

  connect(&m_socket, &QAbstractSocket::connected, this, [this]() {
    emit this->requestStateChange(RequestState::Connected);
  });

  connect(&m_socket, &QAbstractSocket::disconnected, this, [this]() {
    emit this->requestStateChange(RequestState::None);
  });

  emit requestStateChange(RequestState::None);
}

GeminiClient::~GeminiClient() {
  m_isReceivingBody = false;
}

bool GeminiClient::supportsUrl(const QString& url) const {
  return url.startsWith(QL1S("gemini://"));
}

bool GeminiClient::supportsUrl(const QUrl& url) const {
  return url.scheme() == QL1S("gemini");
}

bool GeminiClient::startRequest(const QUrl& url, RequestOptions options) {
  if (!supportsUrl(url)) {
    return false;
  }

  // qDebug() << "start request" << url;

  if (m_socket.state() != QTcpSocket::UnconnectedState) {
    m_socket.disconnectFromHost();
    m_socket.close();

    if (!m_socket.waitForDisconnected(1500)) {
      return false;
    }
  }

  emit requestStateChange(RequestState::Started);

  m_inErrorState = false;
  options = options;

  QSslConfiguration ssl_config = m_socket.sslConfiguration();
  ssl_config.setProtocol(QSsl::TlsV1_2OrLater);

  /*
  if (not kristall::globals().trust.gemini.enable_ca)
    ssl_config.setCaCertificates(QList<QSslCertificate>{});
  else
  */

  ssl_config.setCaCertificates(QSslConfiguration::systemCaCertificates());

  /*
   */

  m_socket.setSslConfiguration(ssl_config);
  m_socket.connectToHostEncrypted(url.host(), url.port(1965));

  m_buffer.clear();
  m_body.clear();
  m_isReceivingBody = false;
  m_suppressSocketTlsErrors = true;

  if (!m_socket.isOpen()) {
    return false;
  }

  m_targetUrl = url;
  m_mimeType = "<invalid>";

  return true;
}

bool GeminiClient::inProgress() const {
  return m_socket.state() != QTcpSocket::UnconnectedState;
}

bool GeminiClient::cancelRequest() {
  // qDebug() << "cancel request" << isInProgress();
  if (inProgress()) {
    m_isReceivingBody = false;
    m_socket.disconnectFromHost();
    m_buffer.clear();
    m_body.clear();

    if (m_socket.state() != QTcpSocket::UnconnectedState) {
      m_socket.disconnectFromHost();
    }

    m_socket.waitForDisconnected(500);
    m_socket.close();

    bool success = !inProgress();

    // qDebug() << "cancel success" << success;

    return success;
  }
  else {
    return true;
  }
}

bool GeminiClient::enableClientCertificate(const CryptoIdentity& ident) {
  m_socket.setLocalCertificate(ident.certificate);
  m_socket.setPrivateKey(ident.private_key);

  return true;
}

void GeminiClient::disableClientCertificate() {
  m_socket.setLocalCertificate(QSslCertificate{});
  m_socket.setPrivateKey(QSslKey{});
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

  emit networkError(network_error, textual_description);
}

void GeminiClient::socketEncrypted() {
  emit hostCertificateLoaded(m_socket.peerCertificate());

  QString request = m_targetUrl.toString(QUrl::FormattingOptions(QUrl::FullyEncoded)) + QSL("\r\n");

  QByteArray request_bytes = request.toUtf8();

  qint64 offset = 0;

  while (offset < request_bytes.size()) {
    const auto len = m_socket.write(request_bytes.constData() + offset, request_bytes.size() - offset);

    if (len <= 0) {
      m_socket.close();
      return;
    }

    offset += len;
  }
}

void GeminiClient::socketReadyRead() {
  if (m_inErrorState) {
    return;
  }

  QByteArray response = m_socket.readAll();

  if (m_isReceivingBody) {
    m_body.append(response);

    emit requestProgress(m_body.size());
  }
  else {
    for (int i = 0; i < response.size(); i++) {
      if (response[i] == '\n') {
        m_buffer.append(response.data(), i);
        m_body.append(response.data() + i + 1, response.size() - i - 1);

        // "XY " <META> <CR> <LF>
        if (m_buffer.size() < 4) { // we allow an empty <META>
          m_socket.close();

          qDebug() << m_buffer;

          emit networkError(ProtocolViolation, tr("Line is too short for valid protocol"));

          return;
        }
        if (m_buffer.size() >= 1200) {
          emit networkError(ProtocolViolation, tr("response too large!"));

          m_socket.close();
        }
        if (m_buffer[m_buffer.size() - 1] != '\r') {
          m_socket.close();

          qDebug() << m_buffer;

          emit networkError(ProtocolViolation, tr("Line does not end with <CR> <LF>"));

          return;
        }
        if (!isdigit(m_buffer[0])) {
          m_socket.close();

          qDebug() << m_buffer;

          emit networkError(ProtocolViolation, tr("First character is not a digit."));

          return;
        }
        if (not isdigit(m_buffer[1])) {
          m_socket.close();

          qDebug() << m_buffer;

          emit networkError(ProtocolViolation, tr("Second character is not a digit."));

          return;
        }

        // TODO: Implement stricter version
        // if(buffer[2] != ' ') {
        if (!isspace(m_buffer[2])) {
          m_socket.close();

          qDebug() << m_buffer;

          emit networkError(ProtocolViolation, tr("Third character is not a space."));

          return;
        }

        QString meta = QString::fromUtf8(m_buffer.data() + 3, m_buffer.size() - 4);

        int primary_code = m_buffer[0] - '0';
        int secondary_code = m_buffer[1] - '0';

        qDebug() << primary_code << secondary_code << meta;

        // We don't need to receive any data after that.
        if (primary_code != 2) {
          m_socket.close();
        }

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
            m_isReceivingBody = true;
            m_mimeType = meta;

            return;

          case 3: { // redirect
            QUrl new_url(meta);
            if (new_url.isValid()) {
              if (new_url.isRelative()) {
                new_url = m_targetUrl.resolved(new_url);
              }

              assert(not new_url.isRelative());

              emit redirected(new_url, (secondary_code == 1));
            }
            else {
              emit networkError(ProtocolViolation, tr("Invalid URL for redirection!"));
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
            emit networkError(ProtocolViolation, tr("Unspecified status code used!"));

            return;
        }

        assert(false && "unreachable");
      }
    }
    if ((m_buffer.size() + response.size()) >= 1200) {
      emit networkError(ProtocolViolation, tr("META too large!"));

      m_socket.close();
    }

    m_buffer.append(response);
  }
}

void GeminiClient::socketDisconnected() {
  if (m_isReceivingBody && !m_inErrorState) {
    m_body.append(m_socket.readAll());
    emit requestComplete(m_body, m_mimeType);
  }
}

void GeminiClient::sslErrors(const QList<QSslError>& errors) {
  emit hostCertificateLoaded(m_socket.peerCertificate());

  if (m_options & IgnoreTlsErrors) {
    m_socket.ignoreSslErrors(errors);
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

  m_socket.ignoreSslErrors(ignored_errors);

  qDebug() << "ignoring" << ignored_errors.size() << "out of" << errors.size();

  for (const auto& error : remaining_errors) {
    qWarning() << int(error.error()) << error.errorString();
  }

  if (remaining_errors.size() > 0) {
    emit networkError(TlsFailure, remaining_errors.first().errorString());
  }
}

void GeminiClient::socketError(QAbstractSocket::SocketError socketError) {
  // When remote host closes TLS session, the client closes the socket.
  // This is more sane then erroring out here as it's a perfectly legal
  // state and we know the TLS connection has ended.
  if (socketError == QAbstractSocket::RemoteHostClosedError) {
    m_socket.close();
    return;
  }

  m_inErrorState = true;

  if (!m_suppressSocketTlsErrors) {
    emitNetworkError(socketError, m_socket.errorString());
  }
}

QUrl GeminiClient::targetUrl() const {
  return m_targetUrl;
}
