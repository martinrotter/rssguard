// This file is part of RSS Guard.

//
// Copyright (C) 2011-2017 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef DATABASECLEANER_H
#define DATABASECLEANER_H

#include <QObject>

#include <QSqlDatabase>

struct CleanerOrders {
  bool m_removeReadMessages;
  bool m_shrinkDatabase;
  bool m_removeOldMessages;
  bool m_removeRecycleBin;
  bool m_removeStarredMessages;
  int m_barrierForRemovingOldMessagesInDays;
};

class DatabaseCleaner : public QObject {
  Q_OBJECT

  public:

    // Constructors.
    explicit DatabaseCleaner(QObject* parent = 0);
    virtual ~DatabaseCleaner();

  signals:
    void purgeStarted();
    void purgeProgress(int progress, const QString& description);
    void purgeFinished(bool result);

  public slots:
    void purgeDatabaseData(const CleanerOrders& which_data);

  private:
    bool purgeStarredMessages(const QSqlDatabase& database);
    bool purgeReadMessages(const QSqlDatabase& database);
    bool purgeOldMessages(const QSqlDatabase& database, int days);
    bool purgeRecycleBin(const QSqlDatabase& database);
};

#endif // DATABASECLEANER_H
