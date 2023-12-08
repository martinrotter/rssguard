// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/apiserver.h"

#include "definitions/definitions.h"

ApiServer::ApiServer(QObject* parent) : HttpServer(parent) {}

void ApiServer::answerClient(QTcpSocket* socket, const QHttpRequest& request) {
  QByteArray incoming_data = socket->readAll();

  const QByteArray output_data = incoming_data;
  const QByteArray reply_message = QSL("HTTP/1.0 200 OK \r\n"
                                       "Content-Type: application/json; charset=\"utf-8\"\r\n"
                                       "Content-Length: %1"
                                       "\r\n\r\n"
                                       "%2")
                                     .arg(QString::number(output_data.size()), output_data)
                                     .toLocal8Bit();

  socket->write(reply_message);
  socket->disconnectFromHost();
}
