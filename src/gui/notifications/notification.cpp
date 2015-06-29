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

#include <QApplication>
#include <QDesktopWidget>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QTimer>

#if defined(Q_OS_MAC)
#include <Carbon/Carbon.h>
#endif


Notification::Notification() : QWidget(0), m_title(QString()), m_text(QString()), m_icon(QPixmap()), m_screen(-1),
  m_width(-1), m_height(-1), m_padding(5), m_widgetMargin(2 * m_padding) {
  setupWidget();
  loadSettings();
}

Notification::~Notification() {
}

void Notification::notify(const QString &text, const QString &title, const QIcon &icon) {
  hide();

  // Set new values.
  m_text = text;
  m_title = title;
  m_icon = icon.pixmap(NOTIFICATION_ICON_SIZE, NOTIFICATION_ICON_SIZE);

  // Show it.
  updateGeometries();
  repaint();
  show();
}

void Notification::notify(const QString &text, const QString &title, QSystemTrayIcon::MessageIcon icon) {
  notify(text, title, MessageBox::iconForStatus((QMessageBox::Icon) icon));
}

void Notification::updateGeometries() {
  // Calculate width and height of notification with given icon and text.
  m_width = m_padding +
            m_icon.width() + m_padding + /* contents */ qMax(stringWidth(m_title), stringWidth(m_text)) +
            m_padding;
  m_height = m_padding +
             /* contents */
             qMax(m_icon.height(),
                  stringHeight(m_title) + m_padding + stringHeight(m_text)) +
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
    painter.setOpacity(0.7);
  }
  else {
    painter.setOpacity(0.95);
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
  int title_height = stringHeight(m_title);
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
  QTimer::singleShot(0, this, SLOT(hide()));
}

void Notification::enterEvent(QEvent *event) {
  QWidget::enterEvent(event);
  repaint();
}

void Notification::leaveEvent(QEvent *event) {
  QWidget::leaveEvent(event);
  repaint();
}

int Notification::stringHeight(const QString &string) {
  int count_lines = string.split(QL1C('\n')).size();
  return fontMetrics().height() * count_lines;
}

int Notification::stringWidth(const QString &string) {
  QStringList lines = string.split(QL1C('\n'));
  int width = 0;

  foreach (const QString &line, lines) {
    int line_width = fontMetrics().width(line);

    if (line_width > width) {
      width = line_width;
    }
  }

  return width;
}

void Notification::loadSettings() {
  // TODO: načist z nastaveni.
  m_position = Qt::BottomRightCorner;
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

  // TODO: pokracovat
  // https://github.com/binaryking/QNotify/blob/master/QNotify.cpp
  // http://stackoverflow.com/questions/5823700/notification-window-in-mac-with-or-without-qt
  // quiterss
  // a odkazy z issue
  // promyslet esli tam dat jen ciste label a ikonu, nebo i seznam nejnovesich zprav atp.
}
