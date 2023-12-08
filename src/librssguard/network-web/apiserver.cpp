// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/apiserver.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <QJsonArray>

ApiServer::ApiServer(QObject* parent) : HttpServer(parent) {}

void ApiServer::answerClient(QTcpSocket* socket, const QHttpRequest& request) {
  QByteArray incoming_data = socket->readAll();
  QByteArray output_data;

  QJsonParseError json_err;
  QJsonDocument incoming_doc = QJsonDocument::fromJson(incoming_data, &json_err);

  if (json_err.error != QJsonParseError::ParseError::NoError) {
    output_data =
      ApiResponse(ApiResponse::Result::Error, ApiRequest::Method::Unknown, QJsonValue(json_err.errorString()))
        .toJson()
        .toJson();
  }
  else {
    ApiRequest req(incoming_doc);
    ApiResponse resp(processRequest(req));

    output_data = resp.toJson().toJson();
  }

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

ApiResponse ApiServer::processRequest(const ApiRequest& req) const {
  switch (req.m_method) {
    case ApiRequest::Method::AppVersion:
      return processAppVersion();

    case ApiRequest::Method::ArticlesFromFeed:
      return processArticlesFromFeed(req.m_parameters);

    case ApiRequest::Method::Unknown:
    default:
      return processUnknown();
  }
}

ApiResponse ApiServer::processAppVersion() const {
  return ApiResponse(ApiResponse::Result::Success, ApiRequest::Method::AppVersion, QSL(APP_VERSION));
}

ApiResponse ApiServer::processArticlesFromFeed(const QJsonValue& req) const {
  QJsonObject data = req.toObject();
  QString feed_id = data.value(QSL("feed")).toString();
  int account_id = data.value(QSL("account")).toInt();

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  QList<Message> msgs = DatabaseQueries::getUndeletedMessagesForFeed(database, feed_id, account_id);
  QJsonArray msgs_json_array;

  for (const Message& msg : msgs) {
    QJsonObject msg_obj;

    msg_obj.insert(QSL("contents"), msg.m_contents);
    msgs_json_array.append(msg_obj);
  }

  ApiResponse resp(ApiResponse::Result::Success, ApiRequest::Method::ArticlesFromFeed, msgs_json_array);

  return resp;
}

ApiResponse ApiServer::processUnknown() const {
  return ApiResponse(ApiResponse::Result::Error, ApiRequest::Method::Unknown, QSL("unknown method"));
}

QJsonDocument ApiResponse::toJson() const {
  QJsonObject obj;

  obj.insert("method", int(m_method));
  obj.insert("result", int(m_result));
  obj.insert("data", m_response);

  return QJsonDocument(obj);
}
