// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef APISERVER_H
#define APISERVER_H

#include "network-web/httpserver.h"

class ApiServer : public HttpServer {
  public:
    explicit ApiServer(QObject* parent = nullptr);

  protected:
    virtual void answerClient(QTcpSocket* socket, const QHttpRequest& request);
};

#endif // APISERVER_H
