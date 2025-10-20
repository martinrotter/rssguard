// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include <QHostAddress>
#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QUrl>

class HttpServer : public QObject {
    Q_OBJECT

  public:
    explicit HttpServer(QObject* parent = nullptr);
    virtual ~HttpServer();

    bool isListening() const;

    // Stops server and clear all connections.
    void stop();

    // Returns listening portnumber.
    quint16 listenPort() const;

    // Returns listening IP address, usually something like "127.0.0.1".
    QHostAddress listenAddress() const;

    // Returns full URL string.
    QString listenAddressPort() const;

    // Sets full URL string, for example "http://localhost:123456".
    void setListenAddressPort(const QString& full_uri, bool start_handler);

  protected:
    struct HttpHeader {
        QString m_name;
        QString m_value;
    };

    struct HttpRequest {
        bool readMethod(QTcpSocket* socket);
        bool readUrl(QTcpSocket* socket);
        bool readStatus(QTcpSocket* socket);
        bool readHeader(QTcpSocket* socket);

        enum class State {
          ReadingMethod,
          ReadingUrl,
          ReadingStatus,
          ReadingHeader,
          ReadingBody,
          AllDone
        } m_state = State::ReadingMethod;

        enum class Method {
          Unknown,
          Head,
          Get,
          Put,
          Post,
          Delete,
          Options
        } m_method = Method::Unknown;

        QString m_address;
        quint16 m_port = 0;
        QByteArray m_fragment;
        QUrl m_url;
        QPair<quint8, quint8> m_version;
        QMap<QByteArray, QByteArray> m_headers;
    };

    virtual void answerClient(QTcpSocket* socket, const HttpRequest& request) = 0;

  private slots:
    void clientConnected();

  private:
    void readReceivedData(QTcpSocket* socket);

  private:
    QMap<QTcpSocket*, HttpRequest> m_connectedClients;
    QTcpServer m_httpServer;
    QHostAddress m_listenAddress;
    quint16 m_listenPort;
    QString m_listenAddressPort;
};

#endif // HTTPSERVER_H
