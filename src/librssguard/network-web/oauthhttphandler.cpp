// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/oauthhttphandler.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <cctype>

#include <QFile>
#include <QIODevice>
#include <QTcpSocket>
#include <QUrlQuery>

namespace {

QString rssGuardIconDataUrl() {
  QFile icon_file(APP_ICON_PATH);

  if (!icon_file.open(QIODevice::OpenModeFlag::ReadOnly)) {
    return {};
  }

  return QSL("data:image/png;base64,") + QString::fromLatin1(icon_file.readAll().toBase64());
}

QString oauthCompletionPage(bool success,
                            const QString& success_text,
                            const QString& failure_text,
                            const QString& app_name) {
  const QString icon_data_url = rssGuardIconDataUrl();
  const QString title = success ? QObject::tr("Login successful") : QObject::tr("Login failed");
  const QString message = success ? success_text : failure_text;
  const QString status_class = success ? QSL("success") : QSL("failure");
  const QString escaped_app_name = app_name.toHtmlEscaped();
  const QString escaped_title = title.toHtmlEscaped();
  const QString escaped_message = message.toHtmlEscaped();
  const QString icon = icon_data_url.isEmpty()
                         ? QString()
                         : QSL("<img class=\"icon\" src=\"%1\" alt=\"%2\" />").arg(icon_data_url, escaped_app_name);

  return QSL(R"(<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>%1 - %2</title>
  <style>
    :root {
      color-scheme: light dark;
      font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background: #f5f7fb;
      color: #1f2937;
    }

    @media (prefers-color-scheme: dark) {
      :root {
        background: #111827;
        color: #e5e7eb;
      }
    }

    body {
      min-height: 100vh;
      margin: 0;
      display: grid;
      place-items: center;
    }

    main {
      width: min(480px, calc(100vw - 32px));
      box-sizing: border-box;
      padding: 32px;
      border: 1px solid color-mix(in srgb, currentColor 14%, transparent);
      border-radius: 8px;
      background: color-mix(in srgb, Canvas 92%, transparent);
      box-shadow: 0 20px 45px rgb(15 23 42 / 14%);
      text-align: center;
    }

    .icon {
      width: 72px;
      height: 72px;
      object-fit: contain;
      margin-bottom: 20px;
    }

    .status {
      display: inline-flex;
      align-items: center;
      justify-content: center;
      width: 34px;
      height: 34px;
      margin-bottom: 12px;
      border-radius: 50%;
      font-weight: 700;
      color: white;
    }

    .success {
      background: #15803d;
    }

    .failure {
      background: #b91c1c;
    }

    h1 {
      margin: 0 0 10px;
      font-size: 1.5rem;
      font-weight: 650;
      letter-spacing: 0;
    }

    p {
      margin: 0;
      line-height: 1.55;
      color: color-mix(in srgb, currentColor 78%, transparent);
    }
  </style>
</head>
<body>
  <main>
    %3
    <div class="status %4">%5</div>
    <h1>%6</h1>
    <p>%7</p>
  </main>
</body>
</html>)")
    .arg(escaped_app_name,
         escaped_title,
         icon,
         status_class,
         success ? QSL("OK") : QSL("!"),
         escaped_title,
         escaped_message);
}

} // namespace

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

void OAuthHttpHandler::answerClient(QTcpSocket* socket, const HttpRequest& request) {
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

    const QString error = received_data.value(QSL("error")).toString();
    const QString code = received_data.value(QSL("code")).toString();
    const QString state = received_data.value(QSL("state")).toString();
    const bool success = error.isEmpty() && !code.isEmpty() && !state.isEmpty();
    const QString failure_text = error.isEmpty()
                                   ? tr("Authorization did not finish correctly. Go back to %1 and try again.")
                                       .arg(QSL(APP_NAME))
                                   : tr("Authorization was rejected or failed. Go back to %1 and try again.")
                                       .arg(QSL(APP_NAME));

    handleRedirection(received_data);

    const QString html = oauthCompletionPage(success, m_successText, failure_text, qApp->applicationName());
    const QByteArray html_utf = html.toUtf8();
    const QByteArray reply_message = QSL("HTTP/1.1 200 OK \r\n"
                                         "Content-Type: text/html; charset=utf-8\r\n"
                                         "Content-Length: %1\r\n"
                                         "\r\n")
                                       .arg(QString::number(html_utf.size()))
                                       .toLocal8Bit()
                                       .append(html_utf);

    socket->write(reply_message);
  }

  socket->disconnectFromHost();
}
