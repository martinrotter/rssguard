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

#include "core/networkfactory.h"

#include "core/defs.h"
#include "core/silentnetworkaccessmanager.h"
#include "core/feedsmodelstandardfeed.h"
#include "core/settings.h"

#include <QEventLoop>
#include <QTimer>
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
      return tr("success");

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

QNetworkReply::NetworkError NetworkFactory::downloadIcon(const QString &url,
                                                         int timeout,
                                                         QIcon &output) {
#if QT_VERSION >= 0x050000
  QString google_s2_with_url = QString("http://www.google.com/s2/favicons?domain=%1").arg(url.toHtmlEscaped());
#else
  QString google_s2_with_url = QString("http://www.google.com/s2/favicons?domain=%1").arg(Qt::escape(url));
#endif
  QByteArray icon_data;

  QNetworkReply::NetworkError network_result =  downloadFeedFile(google_s2_with_url,
                                                                 timeout,
                                                                 icon_data);

  if (network_result == QNetworkReply::NoError) {
    QPixmap icon_pixmap;
    icon_pixmap.loadFromData(icon_data);
    output = QIcon(icon_pixmap);
  }

  return network_result;
}

QNetworkReply::NetworkError NetworkFactory::downloadFeedFile(const QString &url,
                                                             int timeout,
                                                             QByteArray &output,
                                                             bool protected_contents,
                                                             const QString &username,
                                                             const QString &password) {
  // Original asynchronous behavior of QNetworkAccessManager
  // is replaced by synchronous behavior in order to make
  // process of downloading of a file easier to understand.

  // Make necessary variables.
  SilentNetworkAccessManager manager;
  QEventLoop loop;
  QTimer timer;
  QNetworkRequest request;
  QNetworkReply *reply;
  QObject originatingObject;

  // Set credential information as originating object.
  originatingObject.setProperty("protected", protected_contents);
  originatingObject.setProperty("username", username);
  originatingObject.setProperty("password", password);
  request.setOriginatingObject(&originatingObject);

  // Set url for this reques.
  request.setUrl(url);

  // Create necessary connections.
  QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
  QObject::connect(&manager, SIGNAL(finished(QNetworkReply*)), &loop, SLOT(quit()));

  forever {
    // This timer fires just ONCE.
    timer.setSingleShot(true);

    // Try to open communication channel.
    reply = manager.get(request);

    // Start the timeout timer.
    timer.start(timeout);

    // Enter the event loop.
    loop.exec();

    // At this point one of two things happened:
    //  a) file download was completed,
    //  b) communication timed-out.

    if (timer.isActive()) {
      // Download is complete because timer is still running.
      timer.stop();
    }
    else {
      reply->deleteLater();

      // Timer already fired. Download is NOT successful.
      return QNetworkReply::TimeoutError;
    }

    // In this phase, some part of downloading process is completed.
    QUrl redirection_url = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();

    if (redirection_url.isValid()) {
      // Communication indicates that HTTP redirection is needed.
      // Setup redirection URL and download again.
      request.setUrl(redirection_url);
    }
    else {
      // No redirection is indicated. Final file is obtained
      // in our "reply" object.
      break;
    }
  }

  // Read the data into output buffer.
  output = reply->readAll();

  QNetworkReply::NetworkError reply_error = reply->error();

  qDebug("File '%s' fetched with status '%s' (code %d).",
         qPrintable(url),
         qPrintable(networkErrorText(reply_error)),
         reply_error);

  // Delete needed stuff and exit.
  reply->deleteLater();
  return reply_error;
}
