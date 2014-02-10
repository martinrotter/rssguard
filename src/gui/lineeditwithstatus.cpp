#include "gui/lineeditwithstatus.h"

#include "gui/plaintoolbutton.h"
#include "gui/baselineedit.h"

#include <QHBoxLayout>


LineEditWithStatus::LineEditWithStatus(QWidget *parent)
  : WidgetWithStatus(parent) {
  m_wdgInput = new BaseLineEdit(this);

  // Set correct size for the tool button.
  int txt_input_height = m_wdgInput->sizeHint().height();
  m_btnStatus->setFixedSize(txt_input_height, txt_input_height);

  // Compose the layout.
  m_layout->addWidget(m_wdgInput);
  m_layout->addWidget(m_btnStatus);
}

LineEditWithStatus::~LineEditWithStatus() {
}
