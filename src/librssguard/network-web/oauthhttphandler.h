// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef OAUTHHTTPHANDLER_H
#define OAUTHHTTPHANDLER_H

#include "network-web/httpserver.h"

class OAuthHttpHandler : public HttpServer {
    Q_OBJECT

  public:
    explicit OAuthHttpHandler(const QString& success_text, QObject* parent = nullptr);
    virtual ~OAuthHttpHandler();

  signals:
    void authRejected(const QString& error_description, const QString& state);
    void authGranted(const QString& auth_code, const QString& state);

  protected:
    virtual void answerClient(QTcpSocket* socket, const HttpRequest& request);

  private:
    void handleRedirection(const QVariantMap& data);

  private:
    QString m_successText;
};

#endif // OAUTHHTTPHANDLER_H
