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

#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include "core/messagesmodelsqllayer.h"
#include <QSqlQueryModel>

#include "core/message.h"
#include "definitions/definitions.h"
#include "services/abstract/rootitem.h"

#include <QFont>
#include <QIcon>

class MessagesModelCache;

class MessagesModel : public QSqlQueryModel, public MessagesModelSqlLayer {
  Q_OBJECT

  public:

    // Enum which describes basic filtering schemes
    // for messages.
    enum MessageHighlighter {
      NoHighlighting = 100,
      HighlightUnread = 101,
      HighlightImportant = 102
    };

    // Constructors and destructors.
    explicit MessagesModel(QObject* parent = 0);
    virtual ~MessagesModel();

    // Fetches ALL available data to the model.
    // NOTE: This activates the SQL query and populates the model with new data.
    void repopulate();

    // Model implementation.
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole);
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
    QVariant data(int row, int column, int role = Qt::DisplayRole) const;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const;
    Qt::ItemFlags flags(const QModelIndex& index) const;

    // Returns message at given index.
    Message messageAt(int row_index) const;
    int messageId(int row_index) const;
    RootItem::Importance messageImportance(int row_index) const;

    RootItem* loadedItem() const;

    void updateDateFormat();
    void reloadWholeLayout();

    // SINGLE message manipulators.
    bool switchMessageImportance(int row_index);
    bool setMessageRead(int row_index, RootItem::ReadStatus read);

    // BATCH messages manipulators.
    bool switchBatchMessageImportance(const QModelIndexList& messages);
    bool setBatchMessagesDeleted(const QModelIndexList& messages);
    bool setBatchMessagesRead(const QModelIndexList& messages, RootItem::ReadStatus read);
    bool setBatchMessagesRestored(const QModelIndexList& messages);

    // Highlights messages.
    void highlightMessages(MessageHighlighter highlight);

    // Loads messages of given feeds.
    void loadMessages(RootItem* item);

    int itemHeight() const;
    void setItemHeight(int item_height);
    void updateItemHeight();

  public slots:

    // NOTE: These methods DO NOT actually change data in the DB, just in the model.
    // These are particularly used by msg browser.
    bool setMessageImportantById(int id, RootItem::Importance important);
    bool setMessageReadById(int id, RootItem::ReadStatus read);

  private:
    void setupHeaderData();
    void setupFonts();
    void setupIcons();

    MessagesModelCache* m_cache;
    MessageHighlighter m_messageHighlighter;
    QString m_customDateFormat;
    RootItem* m_selectedItem;

    QList<QString> m_headerData;
    QList<QString> m_tooltipData;

    QFont m_normalFont;
    QFont m_boldFont;
    QFont m_normalStrikedFont;
    QFont m_boldStrikedFont;
    QIcon m_favoriteIcon;
    QIcon m_readIcon;
    QIcon m_unreadIcon;
    int m_itemHeight;
};

Q_DECLARE_METATYPE(MessagesModel::MessageHighlighter)

#endif // MESSAGESMODEL_H
