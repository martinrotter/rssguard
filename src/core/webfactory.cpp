#include "core/webfactory.h"

#include <QApplication>


QPointer<WebFactory> WebFactory::s_instance;

WebFactory::WebFactory(QObject *parent) : QObject(parent) {
}

WebFactory::~WebFactory() {
  qDebug("Destroying WebFactory instance.");
}

WebFactory *WebFactory::instance() {
  if (s_instance.isNull()) {
    s_instance = new WebFactory(qApp);
  }

  return s_instance;
}
