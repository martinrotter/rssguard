#include "gui/tabcontent.h"


TabContent::TabContent(QWidget *parent) : QWidget(parent), m_index(-1) {
}

TabContent::~TabContent() {
}

void TabContent::setIndex(int index) {
  m_index = index;
}

int TabContent::index() const {
  return m_index;
}
