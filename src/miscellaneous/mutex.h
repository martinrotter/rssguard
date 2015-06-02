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

#ifndef MUTEX_H
#define MUTEX_H

#include <QMutex>
#include <QObject>


class Mutex : public QObject {
    Q_OBJECT

  public:
    // Constructors.
    explicit Mutex(QMutex::RecursionMode mode = QMutex::NonRecursive, QObject *parent = 0);
    virtual ~Mutex();

    // Main methods.
    void lock();
    bool tryLock();
    bool tryLock(int timeout);
    void unlock();

    // Identifies if mutes is locked or not.
    bool isLocked() const;

  protected:
    // These methods set proper value for m_isLocked and emit signals.
    void setLocked();
    void setUnlocked();

  signals:
    void locked();
    void unlocked();

  private:
    QMutex *m_mutex;
    bool m_isLocked;
};

#endif // MUTEX_H
