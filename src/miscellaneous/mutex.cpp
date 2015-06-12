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

#include "miscellaneous/mutex.h"


Mutex::Mutex(QMutex::RecursionMode mode, QObject *parent) : QObject(parent), m_mutex(new QMutex(mode)), m_isLocked(false)  {
}

Mutex::~Mutex() {
  qDebug("Destroying Mutex instance.");
  delete m_mutex;
}

void Mutex::lock() {
  m_mutex->lock();
  setLocked();
}

bool Mutex::tryLock() {
  bool result;

  if ((result = m_mutex->tryLock())) {
    setLocked();
  }

  return result;
}

bool Mutex::tryLock(int timeout) {
  bool result;

  if ((result = m_mutex->tryLock(timeout))) {
    setLocked();
  }

  return result;
}

void Mutex::unlock() {
  m_mutex->unlock();
  setUnlocked();
}

void Mutex::setLocked() {
  m_isLocked = true;
  emit locked();
}

void Mutex::setUnlocked() {
  m_isLocked = false;
  emit unlocked();
}

bool Mutex::isLocked() const {
  return m_isLocked;
}
