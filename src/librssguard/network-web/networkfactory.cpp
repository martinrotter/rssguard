// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/networkfactory.h"

#include "definitions/definitions.h"
#include "network-web/downloader.h"

#include <QEventLoop>
#include <QIcon>
#include <QMetaEnum>
#include <QPixmap>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTimer>

QStringList NetworkFactory::extractFeedLinksFromHtmlPage(const QUrl& url, const QString& html) {
  QStringList feeds;
  QRegularExpression rx(QSL(FEED_REGEX_MATCHER), QRegularExpression::PatternOption::CaseInsensitiveOption);
  QRegularExpression rx_href(QSL(FEED_HREF_REGEX_MATCHER), QRegularExpression::PatternOption::CaseInsensitiveOption);

  rx_href.optimize();

  QRegularExpressionMatchIterator it_rx = rx.globalMatch(html);

  while (it_rx.hasNext()) {
    QRegularExpressionMatch mat_tx = it_rx.next();
    QString link_tag = mat_tx.captured();
    QString feed_link = rx_href.match(link_tag).captured(1);

    if (feed_link.startsWith(QL1S("//"))) {
      feed_link = QSL(URI_SCHEME_HTTP) + feed_link.mid(2);
    }
    else if (feed_link.startsWith(QL1C('/'))) {
      feed_link = url.toString(QUrl::UrlFormattingOption::RemovePath | QUrl::UrlFormattingOption::RemoveQuery |
                               QUrl::UrlFormattingOption::StripTrailingSlash) +
                  feed_link;
    }

    feeds.append(feed_link);
  }

  return feeds;
}

QPair<QByteArray, QByteArray> NetworkFactory::generateBasicAuthHeader(NetworkAuthentication protection,
                                                                      const QString& username,
                                                                      const QString& password) {
  switch (protection) {
    case NetworkFactory::NetworkAuthentication::Basic: {
      if (username.isEmpty()) {
        return {};
      }
      else {
        QString basic_value = username + QSL(":") + password;
        QString header_value = QSL("Basic ") + QString(basic_value.toUtf8().toBase64());

        return QPair<QByteArray, QByteArray>(HTTP_HEADERS_AUTHORIZATION, header_value.toLocal8Bit());
      }
    }

    case NetworkFactory::NetworkAuthentication::Token: {
      QString header_value = QSL("Bearer ") + username;

      return QPair<QByteArray, QByteArray>(HTTP_HEADERS_AUTHORIZATION, header_value.toLocal8Bit());
    }

    case NetworkFactory::NetworkAuthentication::NoAuthentication:
    default:
      return {};
  }
}

QString NetworkFactory::networkErrorText(QNetworkReply::NetworkError error_code) {
  switch (error_code) {
    case QNetworkReply::ProtocolUnknownError:
    case QNetworkReply::ProtocolFailure:

      //: Network status.
      return tr("protocol error");

    case QNetworkReply::ContentAccessDenied:
      return tr("access to content was denied");

    case QNetworkReply::HostNotFoundError:

      //: Network status.
      return tr("host not found");

    case QNetworkReply::OperationCanceledError:
    case QNetworkReply::TimeoutError:
      return tr("connection timed out or was cancelled");

    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::ConnectionRefusedError:

      //: Network status.
      return tr("connection refused");

    case QNetworkReply::ProxyTimeoutError:

      //: Network status.
      return tr("connection timed out");

    case QNetworkReply::SslHandshakeFailedError:

      //: Network status.
      return tr("SSL handshake failed");

    case QNetworkReply::ProxyConnectionClosedError:
    case QNetworkReply::ProxyConnectionRefusedError:

      //: Network status.
      return tr("proxy server connection refused");

    case QNetworkReply::TemporaryNetworkFailureError:

      //: Network status.
      return tr("temporary failure");

    case QNetworkReply::AuthenticationRequiredError:

      //: Network status.
      return tr("authentication failed");

    case QNetworkReply::ProxyAuthenticationRequiredError:

      //: Network status.
      return tr("proxy authentication required");

    case QNetworkReply::ProxyNotFoundError:

      //: Network status.
      return tr("proxy server not found");

    case QNetworkReply::NoError:

      //: Network status.
      return tr("no errors");

    case QNetworkReply::UnknownContentError:

      //: Network status.
      return tr("unknown content");

    case QNetworkReply::ContentNotFoundError:

      //: Network status.
      return tr("content not found");

    default: {
      QMetaEnum enumer = QMetaEnum::fromType<QNetworkReply::NetworkError>();

      //: Network status.
      return tr("unknown error (%1)").arg(enumer.valueToKey(error_code));
    }
  }
}

QString NetworkFactory::sanitizeUrl(const QString& url) {
  static QRegularExpression reg_non_url(QSL("[^\\w\\-.~:\\/?#\\[\\]@!$&'()*+,;=% \\|]"),
                                        QRegularExpression::PatternOption::UseUnicodePropertiesOption);

  return QString(url).replace(reg_non_url, {});
}

QNetworkReply::NetworkError NetworkFactory::downloadIcon(const QList<IconLocation>& urls,
                                                         int timeout,
                                                         QPixmap& output,
                                                         const QList<QPair<QByteArray, QByteArray>>& additional_headers,
                                                         const QNetworkProxy& custom_proxy) {
  QNetworkReply::NetworkError network_result = QNetworkReply::NetworkError::UnknownNetworkError;

  for (const auto& url : urls) {
    if (url.m_url.isEmpty()) {
      continue;
    }

    QByteArray icon_data;

    if (url.m_isDirect) {
      // Download directly.
      network_result = performNetworkOperation(url.m_url,
                                               timeout,
                                               {},
                                               icon_data,
                                               QNetworkAccessManager::Operation::GetOperation,
                                               additional_headers,
                                               false,
                                               {},
                                               {},
                                               custom_proxy)
                         .m_networkError;

      if (network_result == QNetworkReply::NetworkError::NoError) {
        QPixmap icon_pixmap;

        icon_pixmap.loadFromData(icon_data);
        output = icon_pixmap;

        if (!output.isNull()) {
          if (output.width() > 128) {
            output = output.scaled(QSize(48, 48),
                                   Qt::AspectRatioMode::KeepAspectRatio,
                                   Qt::TransformationMode::SmoothTransformation);
          }
          break;
        }
      }
    }
    else {
      // Duck Duck Go.
      QUrl url_full = QUrl(url.m_url);
      QString host = url_full.host();

      if (host.startsWith(QSL("www."))) {
        host = host.mid(4);
      }

      const QString ddg_icon_service = QSL("https://external-content.duckduckgo.com/ip3/%1.ico").arg(host);

      // Google S2.
      host = url_full.scheme() + QSL("://") + url_full.host();

      const QString gs2_icon_service = QSL("https://t2.gstatic.com/faviconV2?"
                                           "client=SOCIAL&type=FAVICON&fallback_opts=TYPE,SIZE,URL&"
                                           "url=%1")
                                         .arg(host);

      for (const QString& service : {ddg_icon_service, gs2_icon_service}) {
        network_result = performNetworkOperation(service,
                                                 timeout,
                                                 QByteArray(),
                                                 icon_data,
                                                 QNetworkAccessManager::Operation::GetOperation,
                                                 {},
                                                 false,
                                                 {},
                                                 {},
                                                 custom_proxy)
                           .m_networkError;

        if (network_result == QNetworkReply::NetworkError::NoError) {
          QPixmap icon_pixmap;

          icon_pixmap.loadFromData(icon_data);
          output = icon_pixmap;

          if (!output.isNull()) {
            if (output.width() > 128) {
              output = output.scaled(QSize(48, 48),
                                     Qt::AspectRatioMode::KeepAspectRatio,
                                     Qt::TransformationMode::SmoothTransformation);
            }

            return network_result;
          }
        }
      }
    }
  }

  return network_result;
}

NetworkResult NetworkFactory::performNetworkOperation(const QString& url,
                                                      int timeout,
                                                      const QByteArray& input_data,
                                                      QByteArray& output,
                                                      QNetworkAccessManager::Operation operation,
                                                      const QList<QPair<QByteArray, QByteArray>>& additional_headers,
                                                      bool protected_contents,
                                                      const QString& username,
                                                      const QString& password,
                                                      const QNetworkProxy& custom_proxy) {
  Downloader downloader;
  QEventLoop loop;
  NetworkResult result;

  // We need to quit event loop when the download finishes.
  QObject::connect(&downloader, &Downloader::completed, &loop, &QEventLoop::quit);

  for (const auto& header : additional_headers) {
    if (!header.first.isEmpty()) {
      downloader.appendRawHeader(header.first, header.second);
    }
  }

  if (custom_proxy.type() != QNetworkProxy::ProxyType::DefaultProxy) {
    downloader.setProxy(custom_proxy);
  }

  downloader.manipulateData(url, operation, input_data, timeout, protected_contents, username, password);
  loop.exec();

  output = downloader.lastOutputData();

  result.m_networkError = downloader.lastOutputError();
  result.m_contentType = downloader.lastContentType();
  result.m_cookies = downloader.lastCookies();
  result.m_httpCode = downloader.lastHttpStatusCode();
  result.m_headers = downloader.lastHeaders();
  result.m_url = downloader.lastUrl();

  qDebugNN << LOGSEC_NETWORK << "URLS\n" << url << "\n" << result.m_url.toString();

  return result;
}

NetworkResult NetworkFactory::performNetworkOperation(const QString& url,
                                                      int timeout,
                                                      QHttpMultiPart* input_data,
                                                      QList<HttpResponse>& output,
                                                      QNetworkAccessManager::Operation operation,
                                                      const QList<QPair<QByteArray, QByteArray>>& additional_headers,
                                                      bool protected_contents,
                                                      const QString& username,
                                                      const QString& password,
                                                      const QNetworkProxy& custom_proxy) {
  Downloader downloader;
  QEventLoop loop;
  NetworkResult result;

  // We need to quit event loop when the download finishes.
  QObject::connect(&downloader, &Downloader::completed, &loop, &QEventLoop::quit);

  for (const auto& header : additional_headers) {
    if (!header.first.isEmpty()) {
      downloader.appendRawHeader(header.first, header.second);
    }
  }

  if (custom_proxy.type() != QNetworkProxy::ProxyType::DefaultProxy) {
    downloader.setProxy(custom_proxy);
  }

  downloader.manipulateData(url, operation, input_data, timeout, protected_contents, username, password);
  loop.exec();

  output = downloader.lastOutputMultipartData();

  result.m_networkError = downloader.lastOutputError();
  result.m_contentType = downloader.lastContentType();
  result.m_cookies = downloader.lastCookies();
  result.m_httpCode = downloader.lastHttpStatusCode();
  result.m_headers = downloader.lastHeaders();
  result.m_url = downloader.lastUrl();

  qDebugNN << LOGSEC_NETWORK << "URLS\n" << url << "\n" << result.m_url.toString();

  return result;
}

NetworkResult::NetworkResult()
  : m_networkError(QNetworkReply::NetworkError::NoError), m_httpCode(0), m_contentType(QString()), m_cookies({}),
    m_headers({}), m_url(QUrl()) {}

NetworkResult::NetworkResult(QNetworkReply::NetworkError err,
                             int http_code,
                             const QString& ct,
                             const QList<QNetworkCookie>& cook)
  : m_networkError(err), m_httpCode(http_code), m_contentType(ct), m_cookies(cook), m_url(QUrl()) {}
