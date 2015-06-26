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

#if defined(Q_OS_MAC)
#include <Carbon/Carbon.h>
#endif


Notification::Notification() : QWidget(0) {
  setupWidget();
}

Notification::~Notification() {
}

void Notification::setupWidget() {
  // Set window flags.
  Qt::WindowFlags window_flags = Qt::FramelessWindowHint | Qt::WindowSystemMenuHint |
                                 Qt::WindowStaysOnTopHint | Qt::X11BypassWindowManagerHint |
                                 Qt::WindowDoesNotAcceptFocus;

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
