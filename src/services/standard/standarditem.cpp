#include "services/standard/standarditem.h"

#include "services/standard/standardserviceroot.h"


StandardItem::StandardItem(StandardServiceRoot *service_root) : m_serviceRoot(service_root) {
}

StandardItem::~StandardItem() {
}

StandardServiceRoot *StandardItem::serviceRoot() const {
  return m_serviceRoot;
}

void StandardItem::setServiceRoot(StandardServiceRoot *service_root) {
  m_serviceRoot = service_root;
}
