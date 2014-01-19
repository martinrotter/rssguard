#include "gui/lineeditwithstatus.h"

#include "gui/plaintoolbutton.h"
#include "gui/baselineedit.h"

#include <QHBoxLayout>


LineEditWithStatus::LineEditWithStatus(QWidget *parent)
  : QWidget(parent) {
  m_layout = new QHBoxLayout(this);
  m_txtInput = new BaseLineEdit(this);
  m_btnStatus = new PlainToolButton(this);

  // Compose the layout.
  m_layout->setMargin(0);
  m_layout->setSpacing(0);
  m_layout->addWidget(m_txtInput);
  m_layout->addWidget(m_btnStatus);

  // TODO: pokracovat tady, podle MarkedLineEditu z qonverteru
}

LineEditWithStatus::~LineEditWithStatus() {
}
