// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/mutex.h"

#include "definitions/definitions.h"

Mutex::Mutex(QObject* parent) : QObject(parent), m_mutex(new QMutex()), m_isLocked(false) {}

Mutex::~Mutex() {
  qDebugNN << LOGSEC_CORE << ("Destroying Mutex instance.");
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

Mutex::operator QMutex*() const {
  return m_mutex.data();
}
