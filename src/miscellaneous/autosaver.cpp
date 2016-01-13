// This file is part of RSS Guard.
//
// Copyright (C) 2011-2016 by Martin Rotter <rotter.martinos@gmail.com>
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

#include "miscellaneous/autosaver.h"

#include <QDir>
#include <QCoreApplication>
#include <QMetaObject>

#define AUTOSAVE_IN  1000 * 3  // seconds
#define MAXWAIT      1000 * 15 // seconds


AutoSaver::AutoSaver(QObject *parent) : QObject(parent) {
  Q_ASSERT(parent);
}

AutoSaver::~AutoSaver() {
  if (m_timer.isActive()) {
    qWarning("AutoSaver: still active when destroyed, changes not saved.");

    if (parent() && parent()->metaObject()) {
      qWarning("Should call saveIfNeccessary.");
    }
  }
}

void AutoSaver::changeOccurred() {
  if (m_firstChange.isNull()) {
    m_firstChange.start();
  }

  if (m_firstChange.elapsed() > MAXWAIT) {
    saveIfNeccessary();
  }
  else {
    m_timer.start(AUTOSAVE_IN, this);
  }
}

void AutoSaver::timerEvent(QTimerEvent *event) {
  if (event->timerId() == m_timer.timerId()) {
    saveIfNeccessary();
  }
  else {
    QObject::timerEvent(event);
  }
}

void AutoSaver::saveIfNeccessary() {
  if (m_timer.isActive()) {
    m_timer.stop();
    m_firstChange = QTime();

    if (!QMetaObject::invokeMethod(parent(), "save", Qt::DirectConnection)) {
      qWarning("AutoSaver: error invoking slot save() on parent.");
    }
  }
}
