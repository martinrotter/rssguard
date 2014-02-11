#include "gui/labelwithstatus.h"

#include "gui/plaintoolbutton.h"

#include <QHBoxLayout>


LabelWithStatus::LabelWithStatus(QWidget *parent)
  : WidgetWithStatus(parent) {
  m_wdgInput = new QLabel(this);

  // Set correct size for the tool button.
  int txt_input_height = m_wdgInput->sizeHint().height();
  m_btnStatus->setFixedSize(txt_input_height + 4, txt_input_height + 4);

  // Compose the layout.
  m_layout->addWidget(m_wdgInput);
  m_layout->addWidget(m_btnStatus);
}

LabelWithStatus::~LabelWithStatus() {
}

void LabelWithStatus::setStatus(WidgetWithStatus::StatusType status,
                                const QString &label_text) {
  WidgetWithStatus::setStatus(status, label_text);
  label()->setText(label_text);
}
