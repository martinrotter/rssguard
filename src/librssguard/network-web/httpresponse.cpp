// For license of this file, see <project-root-folder>/LICENSE.md.

#include "network-web/httpresponse.h"

HttpResponse::HttpResponse() : m_body(QString()) {}

QString HttpResponse::body() const {
  return m_body;
}

QList<HttpHeader> HttpResponse::headers() const {
  return m_headers;
}

void HttpResponse::appendHeader(const QString& name, const QString& value) {
  HttpHeader head;

  head.first = name;
  head.second = value;

  m_headers.append(head);
}

void HttpResponse::setBody(const QString& body) {
  m_body = body;
}
