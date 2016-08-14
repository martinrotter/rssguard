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

#ifndef FEEDREADER_H
#define FEEDREADER_H

#include <QObject>


class FeedDownloader;
class FeedsModel;
class MessagesModel;
class ServiceEntryPoint;

class FeedReader : public QObject {
    Q_OBJECT

  public:
    explicit FeedReader(QObject *parent = 0);
    virtual ~FeedReader();

    // List of all installed "feed service plugins", including obligatory
    // "standard" service entry point.
    QList<ServiceEntryPoint*> feedServices();

    FeedDownloader *feedDownloader() const;
    FeedsModel *feedsModel() const;
    MessagesModel *messagesModel() const;

  public slots:
    void start();
    void stop();

  private:
    QList<ServiceEntryPoint*> m_feedServices;

    FeedDownloader *m_feedDownloader;
    FeedsModel *m_feedsModel;
    MessagesModel *m_messagesModel;
};

#endif // FEEDREADER_H
