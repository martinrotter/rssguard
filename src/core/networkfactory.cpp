#include "core/networkfactory.h"

#include "core/silentnetworkaccessmanager.h"

#include <QEventLoop>
#include <QTimer>


NetworkFactory::NetworkFactory() {
}

QNetworkReply::NetworkError NetworkFactory::downloadFile(const QString &url,
                                                         int timeout,
                                                         QByteArray &output) {
  // Original asynchronous behavior of QNetworkAccessManager
  // is replaced by synchronous behavior in order to make
  // process of downloading of a file easier to understand.

  // Make necessary variables.
  SilentNetworkAccessManager manager;
  QEventLoop loop;
  QTimer timer;
  QNetworkRequest request;
  QNetworkReply *reply;

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

    if (reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl().isValid()) {
      // Communication indicates that HTTP redirection is needed.
      request.setUrl(reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl());
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

  qDebug("File '%s' fetched with status %d.",
         qPrintable(url),
         reply_error);

  // Delete needed stuff and exit.
  reply->deleteLater();
  return reply_error;
}
