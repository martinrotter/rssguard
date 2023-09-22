// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/basetoastnotification.h"

#include "miscellaneous/iconfactory.h"

#include <QCloseEvent>
#include <QTimer>
#include <QTimerEvent>

#include <chrono>

using namespace std::chrono_literals;

BaseToastNotification::BaseToastNotification(QWidget* parent) : QDialog(parent), m_timerId(-1) {
  setAttribute(Qt::WidgetAttribute::WA_ShowWithoutActivating);
  setFixedWidth(NOTIFICATIONS_WIDTH);
  setFocusPolicy(Qt::FocusPolicy::NoFocus);

  setAttribute(Qt::WidgetAttribute::WA_DeleteOnClose, false);

  setWindowFlags(
#ifdef Q_OS_MAC
    Qt::WindowType::SubWindow |
#else
    Qt::WindowType::Tool |
#endif
    Qt::WindowType::FramelessWindowHint | Qt::WindowType::WindowStaysOnTopHint | Qt::WindowType::WindowSystemMenuHint);

  setStyleSheet(QSL("BaseToastNotification { border: 1px solid %1; }").arg(palette().windowText().color().name()));
  installEventFilter(this);
}

BaseToastNotification::~BaseToastNotification() {}

void BaseToastNotification::setupCloseButton(QAbstractButton* btn) {
  btn->setToolTip(tr("Close this notification"));
  btn->setIcon(qApp->icons()->fromTheme(QSL("dialog-close"), QSL("gtk-close")));

  connect(btn, &QAbstractButton::clicked, this, &BaseToastNotification::close);
}

void BaseToastNotification::stopTimedClosing() {
  killTimer(m_timerId);
  m_timerId = -1;
}

void BaseToastNotification::setupTimedClosing() {
  if (m_timerId < 0) {
    m_timerId = startTimer(NOTIFICATIONS_TIMEOUT);
  }
}

bool BaseToastNotification::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::Type::KeyPress) {
    return true;
  }
  else {
    if (event->type() == QEvent::Type::Enter) {
      stopTimedClosing();
    }

    if (event->type() == QEvent::Type::Leave) {
      setupTimedClosing();
    }

    return QDialog::eventFilter(watched, event);
  }
}

void BaseToastNotification::closeEvent(QCloseEvent* event) {
  stopTimedClosing();
  emit closeRequested(this);
}

void BaseToastNotification::reject() {}

void BaseToastNotification::timerEvent(QTimerEvent* event) {
  if (event->timerId() == m_timerId) {
    stopTimedClosing();
    emit closeRequested(this);
  }
}
