// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef NEWSBLURNETWORK_H
#define NEWSBLURNETWORK_H

#include <QObject>

#include "network-web/networkfactory.h"
#include "services/abstract/feed.h"
#include "services/newsblur/newsblurserviceroot.h"

struct ApiResult {
  bool m_authenticated;
  int m_code;
  QStringList m_errors;
  QJsonDocument m_json;

  void decodeBaseResponse(const QByteArray& json_data);
};

struct LoginResult : ApiResult {
  QString m_sessiodId;
  int m_userId;
};

class NewsBlurNetwork : public QObject {
  Q_OBJECT

  public:
    enum class Operations {
      Login,
      Feeds
    };

    explicit NewsBlurNetwork(QObject* parent = nullptr);

    // Convenience methods.
    RootItem* categoriesFeedsLabelsTree(const QNetworkProxy& proxy);

    // API.
    QJsonDocument feeds(const QNetworkProxy& proxy);

    // Misc.
    void clearCredentials();

    QString username() const;
    void setUsername(const QString& username);

    QString password() const;
    void setPassword(const QString& password);

    QString baseUrl() const;
    void setBaseUrl(const QString& base_url);

    int batchSize() const;
    void setBatchSize(int batch_size);

    bool downloadOnlyUnreadMessages() const;
    void setDownloadOnlyUnreadMessages(bool download_only_unread);

    void setRoot(NewsBlurServiceRoot* root);

    // API methods.
    LoginResult login(const QNetworkProxy& proxy);

  private:
    QPair<QByteArray, QByteArray> authHeader() const;

    // Make sure we are logged in and if we are not, throw exception.
    void ensureLogin(const QNetworkProxy& proxy);

    QString sanitizedBaseUrl() const;
    QString generateFullUrl(Operations operation) const;

  private:
    NewsBlurServiceRoot* m_root;
    QString m_username;
    QString m_password;
    QString m_baseUrl;
    int m_batchSize;
    bool m_downloadOnlyUnreadMessages;
    QString m_authSid;
    int m_userId;
};

#endif // NEWSBLURNETWORK_H
