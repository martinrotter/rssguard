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

#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include "definitions/definitions.h"

#include <QSqlTableModel>
#include <QFont>
#include <QIcon>
#include <QDateTime>


// Represents single message.
class Message {
  public:
    explicit Message() {
      m_title = m_url = m_author = m_contents = "";
    }

    QString m_title;
    QString m_url;
    QString m_author;
    QString m_contents;
    QDateTime m_created;

    // Is true if "created" date was obtained directly
    // from the feed, otherwise is false
    bool m_createdFromFeed;
};

class MessagesModel : public QSqlTableModel {
    Q_OBJECT

  public:
    // Enum which describes basic filtering schemes
    // for messages.
    enum MessageFilter {
      NoHighlighting = 100,
      HighlightUnread = 101,
      HighlightImportant = 102
    };

    enum MessageMode {
      MessagesFromFeeds,
      MessagesFromRecycleBin
    };

    // Constructors and destructors.
    explicit MessagesModel(QObject *parent = 0);
    virtual ~MessagesModel();

    // Model implementation.
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
    QVariant data(int row, int column, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;

    // Returns message at given index.
    Message messageAt(int row_index) const;
    int messageId(int row_index) const;

    // Access to list of currently loaded feed IDs.
    inline QList<int> currentFeeds() const {
      return m_currentFeeds;
    }

  public slots:
    // To disable persistent changes submissions.
    inline bool submitAll() {
      qFatal("Submitting changes via model is not allowed.");
      return false;
    }

    // CORE messages manipulators.
    // NOTE: These are used to change properties of one message.
    // NOTE: Model is NOT reset after one of these methods are applied
    // but changes ARE written to the database.
    bool switchMessageImportance(int row_index);
    bool setMessageRead(int row_index, int read);

    // BATCH messages manipulators.
    // NOTE: These methods are used for changing of attributes of
    // many messages via DIRECT SQL calls.
    // NOTE: Model is reset after one of these methods is applied and
    // changes ARE written to the database.
    bool switchBatchMessageImportance(const QModelIndexList &messages);
    bool setBatchMessagesDeleted(const QModelIndexList &messages, int deleted);
    bool setBatchMessagesRead(const QModelIndexList &messages, int read);
    bool setBatchMessagesRestored(const QModelIndexList &messages);

    // Fetches ALL available data to the model.
    void fetchAll();

    // Loads messages of given feeds.
    void loadMessages(const QList<int> feed_ids);

    void filterMessages(MessageFilter filter);

  signals:
    // Emitted if some persistent change is made which affects count of "unread/all" messages.
    void messageCountsChanged(bool total_message_number_changed);

  protected:
    // Returns selected feed ids in concatenated textual form,
    // which is used for SQL queries.
    QStringList textualFeeds() const;

    // Sets up header data.
    void setupHeaderData();

    // Creates "normal" and "bold" fonts.
    void setupFonts();

    // Sets up all icons which are used directly by this model.
    void setupIcons();

  private:
    MessageMode m_messageMode;
    MessageFilter m_messageFilter;

    QList<int> m_currentFeeds;
    QList<QString> m_headerData;
    QList<QString> m_tooltipData;

    QFont m_normalFont;
    QFont m_boldFont;

    QIcon m_favoriteIcon;
    QIcon m_readIcon;
    QIcon m_unreadIcon;
};

Q_DECLARE_METATYPE(MessagesModel::MessageFilter)

#endif // MESSAGESMODEL_H
