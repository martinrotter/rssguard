// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/basetoastnotification.h"

#include "miscellaneous/iconfactory.h"
#include "miscellaneous/settings.h"

#include <chrono>

#include <QAbstractButton>
#include <QCloseEvent>
#include <QLabel>
#include <QTimerEvent>

using namespace std::chrono_literals;

BaseToastNotification::BaseToastNotification(QWidget* parent) : QDialog(parent), m_timerId(-1) {
  setAttribute(Qt::WidgetAttribute::WA_ShowWithoutActivating);
  setFocusPolicy(Qt::FocusPolicy::NoFocus);
  setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, false);

  setWindowFlags(
#if defined(Q_OS_MAC)
    Qt::WindowType::SubWindow |
#else
    Qt::WindowType::Tool |
#endif
    Qt::WindowType::FramelessWindowHint | Qt::WindowType::WindowStaysOnTopHint | Qt::WindowType::WindowSystemMenuHint);

  setStyleSheet(QSL("BaseToastNotification { border: 1px solid %1; }").arg(palette().windowText().color().name()));
  installEventFilter(this);

  m_timerClosingClick.setInterval(200);
  m_timerClosingClick.setSingleShot(true);

  connect(&m_timerClosingClick, &QTimer::timeout, this, &BaseToastNotification::close);
}

BaseToastNotification::~BaseToastNotification() {}

void BaseToastNotification::reject() {
  close();
}

void BaseToastNotification::setupCloseButton(QAbstractButton* btn) {
  btn->setToolTip(tr("Close this notification"));
  btn->setIcon(qApp->icons()->fromTheme(QSL("dialog-close"), QSL("gtk-close")));

  connect(btn, &QAbstractButton::clicked, this, &BaseToastNotification::close);
}

void BaseToastNotification::setupHeading(QLabel* lbl) {
  auto fon = lbl->font();

  fon.setBold(true);
  fon.setPointSize(fon.pointSize() * 1.2);

  lbl->setFont(fon);
}

void BaseToastNotification::stopTimedClosing() {
  if (m_timerId >= 0) {
    killTimer(m_timerId);
    m_timerId = -1;

    qDebugNN << LOGSEC_NOTIFICATIONS << "Stopping timed closing for notification.";
  }
}

void BaseToastNotification::setupTimedClosing(bool want_shorter_timeout) {
  if (m_timerId < 0) {
    m_timerId = startTimer(want_shorter_timeout ? NOTIFICATION_SHORT_TIMEOUT : NOTIFICATIONS_TIMEOUT);

    qDebugNN << LOGSEC_NOTIFICATIONS << "Starting timed closing for notification.";
  }
}

bool BaseToastNotification::eventFilter(QObject* watched, QEvent* event) {
  if (watched == this && event->type() == QEvent::Type::Enter) {
    stopTimedClosing();
  }

  if (watched == this && event->type() == QEvent::Type::Leave) {
    setupTimedClosing(true);
  }

  if (event->type() == QEvent::Type::MouseButtonPress || event->type() == QEvent::Type::MouseButtonRelease) {
    if (dynamic_cast<QMouseEvent*>(event)->button() == Qt::MouseButton::RightButton) {
      event->accept();
      QCoreApplication::processEvents();
      m_timerClosingClick.start();
      return true;
    }
  }

  return QDialog::eventFilter(watched, event);
}

void BaseToastNotification::closeEvent(QCloseEvent* event) {
  Q_UNUSED(event)

  stopTimedClosing();
  emit closeRequested(this);
}

void BaseToastNotification::timerEvent(QTimerEvent* event) {
  if (event->timerId() == m_timerId) {
    stopTimedClosing();
    emit closeRequested(this);
  }
}
