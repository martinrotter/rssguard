// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APISERVER_H
#define APISERVER_H

#include "network-web/httpserver.h"

#include <QJsonDocument>
#include <QJsonObject>

struct ApiRequest {
    Q_GADGET

  public:
    enum class Method {
      Unknown = 0,
      AppVersion = 1,
      ArticlesFromFeed = 2
    };

    Q_ENUM(Method)

    explicit ApiRequest(const QJsonDocument& data);

    Method m_method;
    QJsonValue m_parameters;
};

struct ApiResponse {
    Q_GADGET

  public:
    enum class Result {
      Success = 1,
      Error = 2
    };

    Q_ENUM(Result)

    explicit ApiResponse(Result result, ApiRequest::Method method, const QJsonValue& response);

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
    QByteArray processCorsPreflight() const;
    QByteArray processHtmlPage() const;

    ApiResponse processRequest(const ApiRequest& req) const;
    ApiResponse processAppVersion() const;
    ApiResponse processArticlesFromFeed(const QJsonValue& req) const;
    ApiResponse processUnknown() const;
};

#endif // APISERVER_H
