// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/apiserver.h"

#include "database/databasefactory.h"
#include "database/databasequeries.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <QJsonArray>
#include <QMetaEnum>

ApiServer::ApiServer(QObject* parent) : HttpServer(parent) {}

void ApiServer::answerClient(QTcpSocket* socket, const HttpRequest& request) {
  QByteArray incoming_data = socket->readAll();
  QByteArray reply_message;

  if (request.m_method == HttpRequest::Method::Options) {
    reply_message = processCorsPreflight();
  }
  else {
    QJsonParseError json_err;
    QByteArray json_data;
    QJsonDocument incoming_doc = QJsonDocument::fromJson(incoming_data, &json_err);

    if (json_err.error != QJsonParseError::ParseError::NoError) {
      json_data =
        ApiResponse(ApiResponse::Result::Error, ApiRequest::Method::Unknown, QJsonValue(json_err.errorString()))
          .toJson()
          .toJson();
    }
    else {
      ApiRequest req(incoming_doc);

      try {
        ApiResponse resp(processRequest(req));

        json_data = resp.toJson().toJson();
      }
      catch (const ApplicationException& ex) {
        ApiResponse err_resp(ApiResponse::Result::Error, req.m_method, ex.message());

        json_data = err_resp.toJson().toJson();
      }
    }

    reply_message = QSL("HTTP/1.0 200 OK \r\n"
                        "Content-Type: application/json; charset=\"utf-8\"\r\n"
                        "Content-Length: %1"
                        "\r\n\r\n")
                      .arg(QString::number(json_data.size()))
                      .toLocal8Bit();

    reply_message += json_data;

#if !defined(NDEBUG)
    IOFactory::writeFile("a.out", json_data);
#endif
  }

  socket->write(reply_message);
  socket->disconnectFromHost();
}

QByteArray ApiServer::processCorsPreflight() const {
  QString answer = QSL("HTTP/1.0 204 No Content\r\n"
                       "Access-Control-Allow-Origin: *\r\n"
                       "Access-Control-Allow-Methods: POST, GET, OPTIONS, DELETE\r\n"
                       "Access-Control-Max-Age: 86400");

  return answer.toLocal8Bit();
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
  bool newest_first = data.value(QSL("newest_first")).toBool();
  bool unread_only = data.value(QSL("unread_only")).toBool();
  int row_offset = data.value(QSL("row_offset")).toInt();
  int row_limit = data.value(QSL("row_limit")).toInt();

  QSqlDatabase database = qApp->database()->driver()->connection(metaObject()->className());
  QList<Message> msgs =
    DatabaseQueries::getArticlesSlice(database, feed_id, account_id, newest_first, unread_only, row_offset, row_limit);
  QJsonArray msgs_json_array;

  for (const Message& msg : msgs) {
    msgs_json_array.append(msg.toJson());
  }

  ApiResponse resp(ApiResponse::Result::Success, ApiRequest::Method::ArticlesFromFeed, msgs_json_array);

  return resp;
}

ApiResponse ApiServer::processUnknown() const {
  return ApiResponse(ApiResponse::Result::Error, ApiRequest::Method::Unknown, QSL("unknown method"));
}

ApiResponse::ApiResponse(Result result, ApiRequest::Method method, const QJsonValue& response)
  : m_result(result), m_method(method), m_response(response) {}

QJsonDocument ApiResponse::toJson() const {
  QJsonObject obj;

  static QMetaEnum enumer_method = QMetaEnum::fromType<ApiRequest::Method>();
  static QMetaEnum enumer_result = QMetaEnum::fromType<ApiResponse::Result>();

  obj.insert(QSL("method"), enumer_method.valueToKey(int(m_method)));
  obj.insert(QSL("result"), enumer_result.valueToKey(int(m_result)));
  obj.insert(QSL("data"), m_response);

  return QJsonDocument(obj);
}

ApiRequest::ApiRequest(const QJsonDocument& data) : m_method(), m_parameters(data.object().value(QSL("data"))) {
  static QMetaEnum enumer = QMetaEnum::fromType<ApiRequest::Method>();

  QByteArray method_name = data.object().value(QSL("method")).toString().toLocal8Bit();

  m_method = Method(enumer.keysToValue(method_name.constData()));
}
