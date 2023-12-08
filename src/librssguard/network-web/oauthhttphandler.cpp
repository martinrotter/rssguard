// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/oauthhttphandler.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <cctype>

#include <QTcpSocket>
#include <QUrlQuery>

OAuthHttpHandler::OAuthHttpHandler(const QString& success_text, QObject* parent)
  : HttpServer(parent), m_successText(success_text) {}

OAuthHttpHandler::~OAuthHttpHandler() {}

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

    qCriticalNN << LOGSEC_OAUTH << "AuthenticationError: " << error << "(" << uri << "): " << description;
    emit authRejected(description, received_state);
  }
  else if (code.isEmpty()) {
    qCriticalNN << LOGSEC_OAUTH << "We did not receive authentication code.";
    emit authRejected(QSL("Code not received"), received_state);
  }
  else if (received_state.isEmpty()) {
    qCriticalNN << LOGSEC_OAUTH << "State not received.";
    emit authRejected(QSL("State not received"), received_state);
  }
  else {
    emit authGranted(code, received_state);
  }
}

void OAuthHttpHandler::answerClient(QTcpSocket* socket, const QHttpRequest& request) {
  if (!request.m_url.path().remove(QL1C('/')).isEmpty()) {
    qCriticalNN << LOGSEC_OAUTH << "Invalid request:" << QUOTE_W_SPACE_DOT(request.m_url.toString());
  }
  else {
    QVariantMap received_data;
    const QUrlQuery query(request.m_url.query());
    const auto items = query.queryItems();

    for (const auto& item : items) {
      received_data.insert(item.first, item.second);
    }

    handleRedirection(received_data);

    const QString html = QSL("<html><head><title>") + qApp->applicationName() + QSL("</title></head><body>") +
                         m_successText + QSL("</body></html>");
    const QByteArray html_utf = html.toUtf8();
    const QByteArray reply_message = QSL("HTTP/1.0 200 OK \r\n"
                                         "Content-Type: text/html; charset=\"utf-8\"\r\n"
                                         "Content-Length: %1"
                                         "\r\n\r\n"
                                         "%2")
                                       .arg(QString::number(html_utf.size()), html_utf)
                                       .toLocal8Bit();

    socket->write(reply_message);
  }

  socket->disconnectFromHost();
}
