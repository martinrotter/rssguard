// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/reusable/widgetwithstatus.h"

#include "gui/reusable/plaintoolbutton.h"
#include "miscellaneous/iconfactory.h"

#include <QHBoxLayout>

WidgetWithStatus::WidgetWithStatus(QWidget* parent) : QWidget(parent), m_wdgInput(nullptr) {
  m_layout = new QHBoxLayout(this);
  m_btnStatus = new PlainToolButton(this);
  m_btnStatus->setFocusPolicy(Qt::FocusPolicy::NoFocus);
  m_iconProgress = qApp->icons()->fromTheme(QSL("view-refresh"));
  m_iconInformation = qApp->icons()->fromTheme(QSL("dialog-information"));
  m_iconWarning = qApp->icons()->fromTheme(QSL("dialog-warning"));
  m_iconError = qApp->icons()->fromTheme(QSL("dialog-error"));
  m_iconOk = qApp->icons()->fromTheme(QSL("dialog-yes"), QSL("dialog-ok"));
  m_iconQuestion = qApp->icons()->fromTheme(QSL("dialog-question"));

  // Set layout properties.
  m_layout->setContentsMargins({});

  setLayout(m_layout);
  setStatus(StatusType::Information, {});
}

void WidgetWithStatus::setStatus(WidgetWithStatus::StatusType status, const QString& text) {
  m_status = status;

  switch (status) {
    case StatusType::Information:
      m_btnStatus->setIcon(m_iconInformation);
      break;

    case StatusType::Progress:
      m_btnStatus->setIcon(m_iconProgress);
      break;

    case StatusType::Warning:
      m_btnStatus->setIcon(m_iconWarning);
      break;

    case StatusType::Error:
      m_btnStatus->setIcon(m_iconError);
      break;

    case StatusType::Ok:
      m_btnStatus->setIcon(m_iconOk);
      break;

    case StatusType::Question:
      m_btnStatus->setIcon(m_iconQuestion);
      break;

    default:
      break;
  }

  // Setup the tooltip text.
  m_btnStatus->setToolTip(text);
}
