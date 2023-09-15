// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/basetoastnotification.h"

#include "miscellaneous/iconfactory.h"

#include <QCloseEvent>
#include <QTimer>

#include <chrono>

using namespace std::chrono_literals;

BaseToastNotification::BaseToastNotification(QWidget* parent) : QDialog(parent) {
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

  setStyleSheet(QSL("BaseToastNotification { border: 1px solid black; }"));
  installEventFilter(this);
}

BaseToastNotification::~BaseToastNotification() {}

void BaseToastNotification::setupCloseButton(QAbstractButton* btn) {
  btn->setToolTip(tr("Close this notification"));
  btn->setIcon(qApp->icons()->fromTheme(QSL("dialog-close"), QSL("gtk-close")));

  connect(btn, &QAbstractButton::clicked, this, &BaseToastNotification::close);
}

void BaseToastNotification::setupTimedClosing() {
  QTimer::singleShot(NOTIFICATIONS_TIMEOUT, this, &BaseToastNotification::close);
}

bool BaseToastNotification::eventFilter(QObject* watched, QEvent* event) {
  if (event->type() == QEvent::Type::KeyPress) {
    return true;
  }
  else {
    return QDialog::eventFilter(watched, event);
  }
}

void BaseToastNotification::closeEvent(QCloseEvent* event) {
  emit closeRequested(this);
}

void BaseToastNotification::reject() {}
