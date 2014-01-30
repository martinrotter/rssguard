#include "gui/lineeditwithstatus.h"

#include "gui/plaintoolbutton.h"
#include "gui/baselineedit.h"
#include "gui/iconthemefactory.h"

#include <QHBoxLayout>


LineEditWithStatus::LineEditWithStatus(QWidget *parent)
  : QWidget(parent) {
  m_layout = new QHBoxLayout(this);
  m_txtInput = new BaseLineEdit(this);
  m_btnStatus = new PlainToolButton(this);

  m_iconInformation = IconThemeFactory::instance()->fromTheme("help-about");
  m_iconWarning = IconThemeFactory::instance()->fromTheme("dialog-warning");
  m_iconError = IconThemeFactory::instance()->fromTheme("dialog-error");
  m_iconOk = IconThemeFactory::instance()->fromTheme("dialog-yes");

  // Set correct size for the tool button.
  int txt_input_height = m_txtInput->sizeHint().height();
  m_btnStatus->setFixedSize(txt_input_height, txt_input_height);

  // Compose the layout.
  m_layout->setMargin(0);
  m_layout->addWidget(m_txtInput);
  m_layout->addWidget(m_btnStatus);

  setLayout(m_layout);
  setStatus(Information, QString());
}

LineEditWithStatus::~LineEditWithStatus() {
}

void LineEditWithStatus::setStatus(LineEditWithStatus::StatusType status,
                                   const QString &tooltip_text) {
  m_status = status;

  switch (status) {
    case Information:
      m_btnStatus->setIcon(m_iconInformation);
      break;

    case Warning:
      m_btnStatus->setIcon(m_iconWarning);
      break;

    case Error:
      m_btnStatus->setIcon(m_iconError);
      break;

    case Ok:
      m_btnStatus->setIcon(m_iconOk);
      break;

    default:
      break;
  }

  // Setup the tooltip text.
  m_btnStatus->setToolTip(tooltip_text);
}


