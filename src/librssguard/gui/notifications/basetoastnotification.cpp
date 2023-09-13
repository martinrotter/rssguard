// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/basetoastnotification.h"

#include "miscellaneous/iconfactory.h"

#include <QCloseEvent>

BaseToastNotification::BaseToastNotification(QWidget* parent) : QDialog(parent) {
  setAttribute(Qt::WidgetAttribute::WA_ShowWithoutActivating);
  setFixedWidth(NOTIFICATIONS_WIDTH);
  setFocusPolicy(Qt::FocusPolicy::NoFocus);

  setWindowFlags(
#ifdef Q_OS_MAC
    Qt::WindowType::SubWindow |
#else
    Qt::WindowType::Tool |
#endif
    Qt::WindowType::FramelessWindowHint | Qt::WindowType::WindowStaysOnTopHint | Qt::WindowType::WindowSystemMenuHint);

  installEventFilter(this);
}

BaseToastNotification::~BaseToastNotification() {}

void BaseToastNotification::setupCloseButton(QAbstractButton* btn) {
  btn->setIcon(qApp->icons()->fromTheme(QSL("dialog-close"), QSL("gtk-close")));

  connect(btn, &QAbstractButton::clicked, this, &BaseToastNotification::closeRequested);
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
  event->ignore();
}
