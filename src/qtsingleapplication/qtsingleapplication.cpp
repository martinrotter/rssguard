/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qtsingleapplication.h"
#include "qtlocalpeer.h"
#include <QWidget>


void QtSingleApplication::sysInit(const QString &appId) {
  actWin = 0;
  peer = new QtLocalPeer(this, appId);
  connect(peer, SIGNAL(messageReceived(const QString&)), SIGNAL(messageReceived(const QString&)));
}

QtSingleApplication::QtSingleApplication(int &argc, char **argv, bool GUIenabled)
  : QApplication(argc, argv, GUIenabled) {
  sysInit();
}

QtSingleApplication::QtSingleApplication(const QString &appId, int &argc, char **argv)
  : QApplication(argc, argv) {
  sysInit(appId);
}

#if QT_VERSION < 0x050000
QtSingleApplication::QtSingleApplication(int &argc, char **argv, Type type)
  : QApplication(argc, argv, type) {
  sysInit();
}

#if defined(Q_WS_X11)
QtSingleApplication::QtSingleApplication(Display* dpy, Qt::HANDLE visual, Qt::HANDLE cmap)
  : QApplication(dpy, visual, cmap) {
  sysInit();
}

QtSingleApplication::QtSingleApplication(Display *dpy, int &argc, char **argv, Qt::HANDLE visual, Qt::HANDLE cmap)
  : QApplication(dpy, argc, argv, visual, cmap) {
  sysInit();
}

QtSingleApplication::QtSingleApplication(Display* dpy, const QString &appId, int argc, char **argv, Qt::HANDLE visual, Qt::HANDLE cmap)
  : QApplication(dpy, argc, argv, visual, cmap) {
  sysInit(appId);
}
#  endif // Q_WS_X11
#endif // QT_VERSION < 0x050000

bool QtSingleApplication::isRunning() {
  return peer->isClient();
}

bool QtSingleApplication::sendMessage(const QString &message, int timeout) {
  return peer->sendMessage(message, timeout);
}

QString QtSingleApplication::id() const {
  return peer->applicationId();
}

bool QtSingleApplication::unlock() {
  return peer->unlock();
}

void QtSingleApplication::setActivationWindow(QWidget* aw, bool activateOnMessage) {
  actWin = aw;
  if (activateOnMessage)
    connect(peer, SIGNAL(messageReceived(const QString&)), this, SLOT(activateWindow()));
  else
    disconnect(peer, SIGNAL(messageReceived(const QString&)), this, SLOT(activateWindow()));
}

QWidget* QtSingleApplication::activationWindow() const {
  return actWin;
}

void QtSingleApplication::activateWindow() {
  if (actWin) {
    actWin->setWindowState(actWin->windowState() & ~Qt::WindowMinimized);
    actWin->raise();
    actWin->activateWindow();
  }
}
