#include "gui/widgetwithstatus.h"

#include "gui/plaintoolbutton.h"
#include "gui/iconthemefactory.h"

#include <QHBoxLayout>


WidgetWithStatus::WidgetWithStatus(QWidget *parent)
  : QWidget(parent), m_wdgInput(NULL) {
  m_layout = new QHBoxLayout(this);
  m_btnStatus = new PlainToolButton(this);

  m_iconInformation = IconThemeFactory::instance()->fromTheme("dialog-information");
  m_iconWarning = IconThemeFactory::instance()->fromTheme("dialog-warning");
  m_iconError = IconThemeFactory::instance()->fromTheme("dialog-error");
  m_iconOk = IconThemeFactory::instance()->fromTheme("dialog-yes");

  // Set layout properties.
  m_layout->setMargin(0);

  setLayout(m_layout);
  setStatus(Information, QString());
}

WidgetWithStatus::~WidgetWithStatus() {
}

void WidgetWithStatus::setStatus(WidgetWithStatus::StatusType status,
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
