// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef URLINTERCEPTOR_H
#define URLINTERCEPTOR_H

#include <QObject>
#include <QWebEngineUrlRequestInfo>

class UrlInterceptor : public QObject {
    Q_OBJECT

  public:
    explicit UrlInterceptor(QObject* parent = nullptr) : QObject(parent) {}

    virtual void interceptRequest(QWebEngineUrlRequestInfo& info) = 0;
};

#endif // URLINTERCEPTOR_H
