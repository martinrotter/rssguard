// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/labelwithstatus.h"

#include "gui/reusable/plaintoolbutton.h"

#include <QHBoxLayout>

LabelWithStatus::LabelWithStatus(QWidget* parent)
  : WidgetWithStatus(parent) {
  m_wdgInput = new QLabel(this);

  // Set correct size for the tool button.
  int label_height = m_wdgInput->sizeHint().height() * 1.2;

  m_btnStatus->setFixedSize(label_height, label_height);

  // Compose the layout.
  m_layout->addWidget(m_wdgInput);
  m_layout->addWidget(m_btnStatus);
}

void LabelWithStatus::setStatus(WidgetWithStatus::StatusType status,
                                const QString& label_text,
                                const QString& status_text) {
  WidgetWithStatus::setStatus(status, status_text);
  label()->setText(label_text);
}
