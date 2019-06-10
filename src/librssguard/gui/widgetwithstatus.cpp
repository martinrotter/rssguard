// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/widgetwithstatus.h"

#include "gui/plaintoolbutton.h"
#include "miscellaneous/iconfactory.h"

#include <QHBoxLayout>

WidgetWithStatus::WidgetWithStatus(QWidget* parent)
  : QWidget(parent), m_wdgInput(nullptr) {
  m_layout = new QHBoxLayout(this);
  m_btnStatus = new PlainToolButton(this);
  m_btnStatus->setFocusPolicy(Qt::NoFocus);
  m_iconProgress = qApp->icons()->fromTheme(QSL("view-refresh"));
  m_iconInformation = qApp->icons()->fromTheme(QSL("dialog-information"));
  m_iconWarning = qApp->icons()->fromTheme(QSL("dialog-warning"));
  m_iconError = qApp->icons()->fromTheme(QSL("dialog-error"));
  m_iconOk = qApp->icons()->fromTheme(QSL("dialog-yes"));

  // Set layout properties.
  m_layout->setMargin(0);
  setLayout(m_layout);
  setStatus(Information, QString());
}

WidgetWithStatus::~WidgetWithStatus() {}

void WidgetWithStatus::setStatus(WidgetWithStatus::StatusType status, const QString& tooltip_text) {
  m_status = status;

  switch (status) {
    case Information:
      m_btnStatus->setIcon(m_iconInformation);
      break;

    case Progress:
      m_btnStatus->setIcon(m_iconProgress);
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
