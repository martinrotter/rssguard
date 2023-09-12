// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/notifications/basetoastnotification.h"

#include "miscellaneous/iconfactory.h"

BaseToastNotification::BaseToastNotification(QWidget* parent) : QDialog(parent) {
  setAttribute(Qt::WidgetAttribute::WA_ShowWithoutActivating);
  setAttribute(Qt::WidgetAttribute::WA_TranslucentBackground);

  setWindowFlags(
#ifdef Q_OS_MAC
    Qt::WindowType::SubWindow |
#else
    Qt::WindowType::Tool |
#endif
    Qt::WindowType::FramelessWindowHint | Qt::WindowType::WindowStaysOnTopHint | Qt::WindowType::WindowSystemMenuHint);
}

BaseToastNotification::~BaseToastNotification() {}

void BaseToastNotification::setupCloseButton(QAbstractButton* btn) {
  btn->setIcon(qApp->icons()->fromTheme(QSL("dialog-close"), QSL("gtk-close")));
}
