// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/networkfactory.h"

#include "definitions/definitions.h"
#include "miscellaneous/settings.h"
#include "network-web/downloader.h"
#include "network-web/silentnetworkaccessmanager.h"

#include <QEventLoop>
#include <QIcon>
#include <QPixmap>
#include <QRegularExpression>
#include <QTextDocument>
#include <QTimer>

QStringList NetworkFactory::extractFeedLinksFromHtmlPage(const QUrl& url, const QString& html) {
  QStringList feeds;
  QRegularExpression rx(FEED_REGEX_MATCHER, QRegularExpression::PatternOption::CaseInsensitiveOption);
  QRegularExpression rx_href(FEED_HREF_REGEX_MATCHER, QRegularExpression::PatternOption::CaseInsensitiveOption);

  rx_href.optimize();

  QRegularExpressionMatchIterator it_rx = rx.globalMatch(html);

  while (it_rx.hasNext()) {
    QRegularExpressionMatch mat_tx = it_rx.next();
    QString link_tag = mat_tx.captured();
    QString feed_link = rx_href.match(link_tag).captured(1);

    if (feed_link.startsWith(QL1S("//"))) {
      feed_link = QString(URI_SCHEME_HTTP) + feed_link.mid(2);
    }
    else if (feed_link.startsWith(QL1C('/'))) {
      feed_link = url.toString(QUrl::RemovePath | QUrl::RemoveQuery | QUrl::StripTrailingSlash) + feed_link;
    }

    feeds.append(feed_link);
  }

  return feeds;
}

QPair<QByteArray, QByteArray> NetworkFactory::generateBasicAuthHeader(const QString& username, const QString& password) {
  if (username.isEmpty()) {
    return QPair<QByteArray, QByteArray>(QByteArray(), QByteArray());
  }
  else {
    QString basic_value = username + ":" + password;
    QString header_value = QString("Basic ") + QString(basic_value.toUtf8().toBase64());

    return QPair<QByteArray, QByteArray>(HTTP_HEADERS_AUTHORIZATION, header_value.toLocal8Bit());
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

    default:

      //: Network status.
      return tr("unknown error");
  }
}

QString NetworkFactory::sanitizeUrl(const QString& url) {
  return QString(url).replace(QRegularExpression(QSL("[^\\w\\-.~:\\/?#\\[\\]@!$&'()*+,;=%]")),
                              QString());
}

QNetworkReply::NetworkError NetworkFactory::downloadIcon(const QList<QPair<QString, bool>>& urls, int timeout,
                                                         QIcon& output, const QNetworkProxy& custom_proxy) {
  QNetworkReply::NetworkError network_result = QNetworkReply::NetworkError::UnknownNetworkError;

  for (const auto& url : urls) {
    if (url.first.isEmpty()) {
      continue;
    }

    QByteArray icon_data;

    if (url.second) {
      // Download directly.
      network_result = performNetworkOperation(url.first,
                                               timeout,
                                               {},
                                               icon_data,
                                               QNetworkAccessManager::Operation::GetOperation,
                                               {},
                                               false,
                                               {},
                                               {},
                                               custom_proxy).first;

      if (network_result == QNetworkReply::NetworkError::NoError) {
        QPixmap icon_pixmap;

        icon_pixmap.loadFromData(icon_data);
        output = QIcon(icon_pixmap);

        if (!output.isNull()) {
          break;
        }
      }
    }
    else {
      // Use favicon fetching service.
      QString host = QUrl(url.first).host();

      if (host.startsWith(QSL("www."))) {
        host = host.mid(4);
      }

      const QString ddg_icon_service = QString("https://external-content.duckduckgo.com/ip3/%1.ico").arg(host);

      network_result = performNetworkOperation(ddg_icon_service,
                                               timeout,
                                               QByteArray(),
                                               icon_data,
                                               QNetworkAccessManager::Operation::GetOperation,
                                               {},
                                               false,
                                               {},
                                               {},
                                               custom_proxy).first;

      if (network_result == QNetworkReply::NetworkError::NoError) {
        QPixmap icon_pixmap;

        icon_pixmap.loadFromData(icon_data);
        output = QIcon(icon_pixmap);

        if (!output.isNull()) {
          break;
        }
      }
    }
  }

  return network_result;
}

NetworkResult NetworkFactory::performNetworkOperation(const QString& url, int timeout, const QByteArray& input_data,
                                                      QByteArray& output, QNetworkAccessManager::Operation operation,
                                                      QList<QPair<QByteArray, QByteArray>> additional_headers,
                                                      bool protected_contents,
                                                      const QString& username, const QString& password,
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
  result.first = downloader.lastOutputError();
  result.second = downloader.lastContentType();
  return result;
}

NetworkResult NetworkFactory::performNetworkOperation(const QString& url,
                                                      int timeout,
                                                      QHttpMultiPart* input_data,
                                                      QList<HttpResponse>& output,
                                                      QNetworkAccessManager::Operation operation,
                                                      QList<QPair<QByteArray, QByteArray>> additional_headers,
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
  result.first = downloader.lastOutputError();
  result.second = downloader.lastContentType();
  return result;
}
