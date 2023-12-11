// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APISERVER_H
#define APISERVER_H

#include "network-web/httpserver.h"

#include <QJsonDocument>
#include <QJsonObject>

struct ApiRequest {
  public:
    enum class Method {
      Unknown = 0,
      AppVersion = 1,
      ArticlesFromFeed = 2
    };

    explicit ApiRequest(const QJsonDocument& data)
      : m_method(Method(data.object().value("method").toInt())), m_parameters(data.object().value("data")) {}

    Method m_method;
    QJsonValue m_parameters;
};

struct ApiResponse {
  public:
    enum class Result {
      Success = 1,
      Error = 2
    };

    explicit ApiResponse(Result result, ApiRequest::Method method, const QJsonValue& response)
      : m_result(result), m_method(method), m_response(response) {}

    Result m_result;
    ApiRequest::Method m_method;
    QJsonValue m_response;

    QJsonDocument toJson() const;
};

class ApiServer : public HttpServer {
  public:
    explicit ApiServer(QObject* parent = nullptr);

  protected:
    virtual void answerClient(QTcpSocket* socket, const HttpRequest& request);

  private:
    ApiResponse processRequest(const ApiRequest& req) const;
    ApiResponse processAppVersion() const;
    ApiResponse processArticlesFromFeed(const QJsonValue& req) const;
    ApiResponse processUnknown() const;
};

#endif // APISERVER_H
