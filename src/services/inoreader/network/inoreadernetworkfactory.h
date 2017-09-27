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

#ifndef INOREADERNETWORKFACTORY_H
#define INOREADERNETWORKFACTORY_H

#include <QObject>

#include "core/message.h"

#include <QNetworkReply>

class RootItem;
class OAuth2Service;

class InoreaderNetworkFactory : public QObject {
  Q_OBJECT

  public:
    explicit InoreaderNetworkFactory(QObject* parent = nullptr);

    OAuth2Service* oauth() const;

    QString userName() const;
    void setUsername(const QString& username);

    // Gets/sets the amount of messages to obtain during single feed update.
    int batchSize() const;
    void setBatchSize(int batch_size);

    // Returns tree of feeds/categories.
    // Top-level root of the tree is not needed here.
    // Returned items do not have primary IDs assigned.
    RootItem* feedsCategories(bool obtain_icons);

    QList<Message> messages(const QString& stream_id, bool* is_error);

  private:
    QList<Message> decodeMessages(const QString& messages_json_data, const QString& stream_id);
    RootItem* decodeFeedCategoriesData(const QString& categories, const QString& feeds, bool obtain_icons);

    void initializeOauth();

  private:
    QString m_username;
    int m_batchSize;
    OAuth2Service* m_oauth2;
};

#endif // INOREADERNETWORKFACTORY_H
