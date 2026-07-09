// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include <QList>
#include <QPair>

typedef QPair<QString, QString> HttpHeader;

class RSSGUARD_DLLSPEC HttpResponse {
  public:
    explicit HttpResponse();

    const QString& body() const;
    void setBody(const QString& body);
    const QList<HttpHeader>& headers() const;

    void appendHeader(const QString& name, const QString& value);

  private:
    QList<HttpHeader> m_headers;
    QString m_body;
};

#endif // HTTPRESPONSE_H
