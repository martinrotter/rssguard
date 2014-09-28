// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "network-web/networkfactory.h"

#include "definitions/definitions.h"
#include "miscellaneous/settings.h"
#include "network-web/silentnetworkaccessmanager.h"
#include "network-web/downloader.h"

#include <QEventLoop>
#include <QTimer>
#include <QIcon>
#include <QPixmap>
#include <QTextDocument>


NetworkFactory::NetworkFactory() {
}

QString NetworkFactory::networkErrorText(QNetworkReply::NetworkError error_code) {
  switch (error_code) {
    case QNetworkReply::ProtocolUnknownError:
    case QNetworkReply::ProtocolFailure:
      //: Network status.
      return tr("protocol error");

    case QNetworkReply::HostNotFoundError:
      //: Network status.
      return tr("host not found");

    case QNetworkReply::RemoteHostClosedError:
    case QNetworkReply::ConnectionRefusedError:
      //: Network status.
      return tr("connection refused");

    case QNetworkReply::TimeoutError:
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
      return tr("uknown content");

    case QNetworkReply::ContentNotFoundError:
      //: Network status.
      return tr("content not found");

    default:
      //: Network status.
      return tr("unknown error");
  }
}

QNetworkReply::NetworkError NetworkFactory::downloadIcon(const QString &url, int timeout, QIcon &output) {
#if QT_VERSION >= 0x050000
  QString google_s2_with_url = QString("http://www.google.com/s2/favicons?domain=%1").arg(url.toHtmlEscaped());
#else
  QString google_s2_with_url = QString("http://www.google.com/s2/favicons?domain=%1").arg(Qt::escape(url));
#endif
  QByteArray icon_data;
  QNetworkReply::NetworkError network_result =  downloadFile(google_s2_with_url, timeout, icon_data).first;

  if (network_result == QNetworkReply::NoError) {
    QPixmap icon_pixmap;
    icon_pixmap.loadFromData(icon_data);
    output = QIcon(icon_pixmap);
  }

  return network_result;
}

NetworkResult NetworkFactory::downloadFeedFile(const QString &url, int timeout,
                                               QByteArray &output, bool protected_contents,
                                               const QString &username, const QString &password) {
  // Here, we want to achieve "synchronous" approach because we want synchronout download API for
  // some use-cases too.
  Downloader downloader;
  QEventLoop loop;
  NetworkResult result;

  downloader.appendRawHeader("Accept", ACCEPT_HEADER_FOR_FEED_DOWNLOADER);

  // We need to quit event loop when the download finishes.
  QObject::connect(&downloader, SIGNAL(completed(QNetworkReply::NetworkError)), &loop, SLOT(quit()));

  downloader.downloadFile(url, timeout, protected_contents, username, password);
  loop.exec();
  output = downloader.lastOutputData();
  result.first = downloader.lastOutputError();
  result.second = downloader.lastContentType();

  return result;
}

NetworkResult NetworkFactory::downloadFile(const QString &url, int timeout,
                                           QByteArray &output, bool protected_contents,
                                           const QString &username, const QString &password) {
  // Here, we want to achieve "synchronous" approach because we want synchronout download API for
  // some use-cases too.
  Downloader downloader;
  QEventLoop loop;
  NetworkResult result;

  // We need to quit event loop when the download finishes.
  QObject::connect(&downloader, SIGNAL(completed(QNetworkReply::NetworkError)), &loop, SLOT(quit()));

  downloader.downloadFile(url, timeout, protected_contents, username, password);
  loop.exec();
  output = downloader.lastOutputData();
  result.first = downloader.lastOutputError();
  result.second = downloader.lastContentType();

  return result;
}
