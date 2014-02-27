// This file is part of RSS Guard.
//
// Copyright (C) 2011-2014 by Martin Rotter <rotter.martinos@gmail.com>
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

#ifndef FEEDSMODELSTANDARDFEED_H
#define FEEDSMODELSTANDARDFEED_H

#include "core/feedsmodelfeed.h"

#include <QDateTime>
#include <QSqlRecord>
#include <QPair>
#include <QNetworkReply>
#include <QApplication>


class Message;

// Represents STANDARD RSS/RDF/ATOM feed with no
// online synchronization services (NO TT-RSS, NO FEEDLY).
// So, parent item is either root item or category.
class FeedsModelStandardFeed : public FeedsModelFeed {
    Q_DECLARE_TR_FUNCTIONS(FeedsModelStandardFeed)

  public:
    enum AutoUpdateType {
      DontAutoUpdate = 0,
      DefaultAutoUpdate = 1,
      SpecificAutoUpdate = 2
    };

    // Constructors and destructors.
    explicit FeedsModelStandardFeed(FeedsModelRootItem *parent_item = NULL);
    virtual ~FeedsModelStandardFeed();

    // Obtains data related to this feed.
    QVariant data(int column, int role) const;

    // Perform fetching of new messages.
    void update();

    // Removes this standard feed from persistent
    // storage.
    bool removeItself();

    // Various getters/setters.
    inline QString encoding() const {
      return m_encoding;
    }

    inline void setEncoding(const QString &encoding) {
      m_encoding = encoding;
    }

    inline QString url() const {
      return m_url;
    }

    inline void setUrl(const QString &url) {
      m_url = url;
    }

    inline int autoUpdateInitialInterval() const {
      return m_autoUpdateInitialInterval;
    }

    inline void setAutoUpdateInitialInterval(int auto_update_interval) {
      // If new initial auto-update interval is set, then
      // we should reset time that remains to the next auto-update.
      m_autoUpdateInitialInterval = auto_update_interval;
      m_autoUpdateRemainingInterval = auto_update_interval;
    }

    inline AutoUpdateType autoUpdateType() const {
      return m_autoUpdateType;
    }

    inline void setAutoUpdateType(const AutoUpdateType &autoUpdateType) {
      m_autoUpdateType = autoUpdateType;
    }

    inline int autoUpdateRemainingInterval() const {
      return m_autoUpdateRemainingInterval;
    }

    inline void setAutoUpdateRemainingInterval(int autoUpdateRemainingInterval) {
      m_autoUpdateRemainingInterval = autoUpdateRemainingInterval;
    }

    // Loads standard feed object from given SQL record.
    static FeedsModelStandardFeed *loadFromRecord(const QSqlRecord &record);

    // Tries to guess feed hidden under given URL
    // and uses given credentials.
    // Returns pointer to guessed feed (if at least partially
    // guessed) and retrieved error/status code from network layer
    // or NULL feed.
    static QPair<FeedsModelStandardFeed*, QNetworkReply::NetworkError> guessFeed(const QString &url,
                                                                                 const QString &username,
                                                                                 const QString &password);

  protected:
    // Persistently stores given messages into the database
    // and updates existing messages if newer version is
    // available.
    void updateMessages(const QList<Message> &messages);

  private:
    AutoUpdateType m_autoUpdateType;
    int m_autoUpdateInitialInterval;
    int m_autoUpdateRemainingInterval;

    QString m_encoding;
    QString m_url;
};

#endif // FEEDSMODELSTANDARDFEED_H
