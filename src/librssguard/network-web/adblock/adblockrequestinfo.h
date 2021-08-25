// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ADBLOCKREQUESTINFO_H
#define ADBLOCKREQUESTINFO_H

#include <QWebEngineUrlRequestInfo>

class AdblockRequestInfo {
  public:
    explicit AdblockRequestInfo(const QWebEngineUrlRequestInfo& webengine_info);
    explicit AdblockRequestInfo(const QUrl& url);

    QString resourceType() const;
    void setResourceType(const QString& resource_type);

    QUrl requestUrl() const;
    void setRequestUrl(const QUrl& request_url);

    QUrl firstPartyUrl() const;
    void setFirstPartyUrl(const QUrl& first_party_url);

    QByteArray requestMethod() const;
    void setRequestMethod(const QByteArray& request_method);

  private:
    void initialize(const QWebEngineUrlRequestInfo& webengine_info);
    void initialize(const QUrl& url);

    QString convertResourceType(QWebEngineUrlRequestInfo::ResourceType rt) const;

  private:
    QString m_resourceType;
    QUrl m_requestUrl;
    QUrl m_firstPartyUrl;
    QUrl m_initiator;
    QByteArray m_requestMethod;
};

#endif // ADBLOCKREQUESTINFO_H
