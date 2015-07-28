// This file is part of RSS Guard.
//
// Copyright (C) 2011-2015 by Martin Rotter <rotter.martinos@gmail.com>
//
// RSS Guard is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// RSS Guard is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with RSS Guard. If not, see <http://www.gnu.org/licenses/>.

#include "gui/notifications/notification.h"

#include "gui/messagebox.h"
#include "definitions/definitions.h"
#include "miscellaneous/application.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/textfactory.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QTimer>

#if defined(Q_OS_MAC)
#include <Carbon/Carbon.h>
#endif

#if defined(Q_OS_LINUX)
#include <QtDBus>
#endif


Notification::Notification() : QWidget(0), m_title(QString()), m_text(QString()), m_icon(QPixmap()), m_screen(-1),
  m_width(-1), m_height(-1), m_padding(5), m_widgetMargin(2 * m_padding), m_timerId(0), m_clickTarget(NULL),
  m_clickSlot(NULL) {

#if defined(Q_OS_LINUX)
  m_dBusActiveNotification = 0;
  m_dBusInterface = new QDBusInterface("org.freedesktop.Notifications",
                                       "/org/freedesktop/Notifications",
                                       "org.freedesktop.Notifications",
                                       QDBusConnection::sessionBus(), this);

  if (m_dBusInterface->isValid()) {
    m_dBusInterface->connection().connect("org.freedesktop.Notifications",
                                          "/org/freedesktop/Notifications",
                                          "org.freedesktop.Notifications",
                                          "NotificationClosed",
                                          this, SLOT(notificationClosed(uint,uint)));
  }
#endif

  setupWidget();
  loadSettings();
}

Notification::~Notification() {
  qDebug("Destroying Notification instance.");
}

bool Notification::areNotificationsActivated() {
  return qApp->settings()->value(GROUP(GUI), SETTING(GUI::UseFancyNotifications)).toBool();
}

void Notification::notify(const QString &text, const QString &title, const QIcon &icon,
                          QObject *invokation_target, const char *invokation_slot) {
  cancel();

  // Set new values.
  m_clickTarget = invokation_target;
  m_clickSlot = invokation_slot;
  m_text = text;
  m_title = title;
  m_icon = icon.pixmap(NOTIFICATION_ICON_SIZE, NOTIFICATION_ICON_SIZE);

#if defined(Q_OS_LINUX)
  if (m_dBusInterface->isValid()) {
    QVariantMap hints;
    hints["image-path"] = ""; // "application-exit";

    QList<QVariant> argument_list;
    argument_list << APP_NAME;      // app_name
    argument_list << (uint)0;       // replace_id
    argument_list << "";            // app_icon
    argument_list << title;         // summary
    argument_list << text;          // body
    argument_list << QStringList(); // actions
    argument_list << hints;         // hints
    argument_list << (int)-1;       // timeout in ms

    // TODO: obrazky https://dev.visucore.com/bitcoin/doxygen/notificator_8cpp_source.html

    QDBusMessage response = m_dBusInterface->callWithArgumentList(QDBus::AutoDetect, "Notify", argument_list);

    if (response.arguments().size() == 1) {
      // Message was sent, notification should display.
      m_dBusActiveNotification = response.arguments().at(0).toUInt();
    }
  }
#else
  if (m_clickTarget != NULL && m_clickSlot != NULL) {
    // Connect invokation target.
    connect(this, SIGNAL(clicked()), m_clickTarget, m_clickSlot, Qt::QueuedConnection);
  }

  // Show it.
  updateGeometries();

  QTimer::singleShot(20, this, SLOT(show()));
  QTimer::singleShot(0, this, SLOT(repaint()));

  m_timerId = startTimer(10000);
#endif
}

void Notification::notify(const QString &text, const QString &title, QSystemTrayIcon::MessageIcon icon,
                          QObject *invokation_target, const char *invokation_slot) {
  notify(text, title, MessageBox::iconForStatus((QMessageBox::Icon) icon), invokation_target, invokation_slot);
}

void Notification::cancel() {
  hide();

  if (m_clickTarget != NULL && m_clickSlot != NULL) {
    // Disconnect previous bubble click signalling.
    disconnect(this, SIGNAL(clicked()), m_clickTarget, m_clickSlot);
  }

  m_clickSlot = NULL;
  m_clickTarget = NULL;

  if (m_timerId != 0) {
    killTimer(m_timerId);
  }
}

void Notification::notificationClosed(uint id, uint reason) {
  if (m_clickTarget != NULL && m_clickSlot != NULL && m_dBusActiveNotification == id && reason == 2) {
    QMetaObject::invokeMethod(m_clickTarget, m_clickSlot);
  }
}

void Notification::updateGeometries() {
  // Calculate width and height of notification with given icon and text.
  QFont bold_font = font();
  bold_font.setBold(true);
  QFontMetrics bold_metrics(bold_font);

  m_width = m_padding +
            m_icon.width() + m_padding + /* contents */ qMax(TextFactory::stringWidth(m_title, bold_metrics),
                                                             TextFactory::stringWidth(m_text, fontMetrics())) +
            m_padding;
  m_height = m_padding +
             /* contents */
             qMax(m_icon.height(),
                  TextFactory::stringHeight(m_title, bold_metrics) + m_padding + TextFactory::stringHeight(m_text, fontMetrics())) +
             m_padding;

  // Calculate real position.
  int x, y;
  QRect screen_geometry = QApplication::desktop()->availableGeometry(m_screen);

  switch (m_position) {
    case Qt::BottomLeftCorner:
      x = m_widgetMargin;
      y = screen_geometry.height() - m_widgetMargin - m_height;
      break;

    case Qt::TopLeftCorner:
      x = m_widgetMargin;
      y = m_widgetMargin;
      break;

    case Qt::TopRightCorner:
      x = screen_geometry.width() - m_widgetMargin - m_width;
      y = m_widgetMargin;
      break;

    case Qt::BottomRightCorner:
    default:
      x = screen_geometry.width() - m_widgetMargin - m_width;
      y = screen_geometry.height() - m_widgetMargin - m_height;
      break;
  }

  setGeometry(x, y, m_width, m_height);
}

void Notification::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event)

  QPainter painter(this);
  painter.setFont(font());

  if (!underMouse()) {
    painter.setOpacity(0.88);
  }
  else {
    painter.setOpacity(0.97);
  }

  // Draw background.
#if QT_VERSION >= 0x050000
  painter.setRenderHints(QPainter::HighQualityAntialiasing | QPainter::Qt4CompatiblePainting);
#else
  painter.setRenderHints(QPainter::HighQualityAntialiasing);
#endif
  painter.setBrush(QColor(220, 220, 220));

  painter.setPen(Qt::NoPen);
  painter.drawRoundedRect(0, 0, width(), height(), 5.0, 5.0);

  // Draw icon.
  painter.drawPixmap(m_padding, m_padding, m_icon);

  // Draw text.
  painter.setPen(Qt::black);

  // Needed heighs/widths.
  int title_height = TextFactory::stringHeight(m_title, fontMetrics());
  int remaining_width = width() - m_padding - m_icon.width() - m_padding /* - here comes contents */ - m_padding;
  int remaining_height = height() - m_padding - title_height - m_padding /* - here comes contents */ - m_padding;

  painter.drawText(m_padding + m_icon.width() + m_padding, m_padding + title_height + m_padding,
                   remaining_width, remaining_height,
                   Qt::AlignLeft, m_text);

  // Draw heading.
  QFont font = painter.font();
  font.setBold(true);
  painter.setFont(font);

  painter.drawText(m_padding + m_icon.width() + m_padding, m_padding,
                   remaining_width, remaining_height,
                   Qt::AlignHCenter | Qt::AlignTop, m_title);
}

void Notification::mousePressEvent(QMouseEvent *event) {
  QWidget::mousePressEvent(event);
  emit clicked();
  cancel();
}

void Notification::enterEvent(QEvent *event) {
  QWidget::enterEvent(event);
  repaint();
}

void Notification::leaveEvent(QEvent *event) {
  QWidget::leaveEvent(event);
  repaint();
}

void Notification::timerEvent(QTimerEvent *event) {
  QWidget::timerEvent(event);
  cancel();
}

void Notification::loadSettings() {
  m_position = static_cast<Qt::Corner>(qApp->settings()->value(GROUP(GUI), SETTING(GUI::FancyNotificationsPosition)).toInt());
}

void Notification::setupWidget() {
  // Set window flags.
  Qt::WindowFlags window_flags = Qt::FramelessWindowHint | Qt::WindowSystemMenuHint |
                                 Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint;

#if QT_VERSION >= 0x050000
  window_flags |= Qt::WindowDoesNotAcceptFocus;
#endif

#if defined (Q_OS_MAC)
  window_flags |= Qt::SubWindow;
#else
  window_flags |= Qt::Tool;
#endif

  setWindowFlags(window_flags);

  // Set widget attributes.
  setAttribute(Qt::WA_TranslucentBackground);
  setAttribute(Qt::WA_X11DoNotAcceptFocus);
  setAttribute(Qt::WA_ShowWithoutActivating);

#if defined (Q_OS_MAC)
  winId();

  int setAttr[] = {kHIWindowBitDoesNotHide, kHIWindowBitDoesNotCycle, kHIWindowBitNoShadow, 0};
  int clearAttr[] = {0};
  HIWindowChangeAttributes(qt_mac_window_for(this), setAttr, clearAttr);
#endif

  // Window will be meant to be on top, but should not steal focus.
  setFocusPolicy(Qt::NoFocus);

  QFont fon(font());
  fon.setPointSize(fon.pointSize() + 5);
  setFont(fon);
}
