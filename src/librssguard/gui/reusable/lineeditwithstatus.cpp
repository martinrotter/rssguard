// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/lineeditwithstatus.h"

#include "gui/reusable/baselineedit.h"
#include "gui/reusable/plaintoolbutton.h"

#include <QHBoxLayout>

LineEditWithStatus::LineEditWithStatus(QWidget* parent) : WidgetWithStatus(parent) {
  m_wdgInput = new BaseLineEdit(this);
  setFocusProxy(m_wdgInput);

  // Set correct size for the tool button.
  const int txt_input_height = m_wdgInput->sizeHint().height();

  m_btnStatus->setFixedSize(txt_input_height, txt_input_height);

  // Compose the layout.
  m_layout->addWidget(m_wdgInput);
  m_layout->addWidget(m_btnStatus);
}


