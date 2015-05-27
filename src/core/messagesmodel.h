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

#ifndef MESSAGESMODEL_H
#define MESSAGESMODEL_H

#include "definitions/definitions.h"

#include "core/feedsselection.h"

#include <QSqlTableModel>
#include <QFont>
#include <QIcon>
#include <QDateTime>


// Represents single enclosuresh

struct Enclosure {
    QString m_url;
    QString m_mimeType;

    explicit Enclosure(const QString &url = QString(), const QString &mime = QString()) : m_url(url), m_mimeType(mime) {
    }
};

// Represents single enclosure.
class Enclosures {
  public:
    static QList<Enclosure> decodeEnclosuresFromString(const QString &enclosures_data) {
      QList<Enclosure> enclosures;

      foreach (const QString &single_enclosure, enclosures_data.split(ENCLOSURES_OUTER_SEPARATOR, QString::SkipEmptyParts)) {
        Enclosure enclosure;

        if (single_enclosure.contains(ECNLOSURES_INNER_SEPARATOR)) {
          QStringList mime_url = single_enclosure.split(ECNLOSURES_INNER_SEPARATOR);

          enclosure.m_mimeType = QByteArray::fromBase64(mime_url.at(0).toLocal8Bit());
          enclosure.m_url = QByteArray::fromBase64(mime_url.at(1).toLocal8Bit());
        }
        else {
          enclosure.m_url = QByteArray::fromBase64(single_enclosure.toLocal8Bit());
        }

        enclosures.append(enclosure);
      }

      return enclosures;
    }

    static QString encodeEnclosuresToString(const QList<Enclosure> &enclosures) {
      QStringList enclosures_str;

      foreach (const Enclosure &enclosure, enclosures) {
        if (enclosure.m_mimeType.isEmpty()) {
          enclosures_str.append(enclosure.m_url.toLocal8Bit().toBase64());
        }
        else {
          enclosures_str.append(QString(enclosure.m_mimeType.toLocal8Bit().toBase64()) +
                                ECNLOSURES_INNER_SEPARATOR +
                                enclosure.m_url.toLocal8Bit().toBase64());
        }
      }

      return enclosures_str.join(QString(ENCLOSURES_OUTER_SEPARATOR));
    }
};

// Represents single message.
class Message {
  public:
    explicit Message() {
      m_title = m_url = m_author = m_contents = "";
      m_enclosures = QList<Enclosure>();
    }

    QString m_title;
    QString m_url;
    QString m_author;
    QString m_contents;
    QDateTime m_created;

    QList<Enclosure> m_enclosures;

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

    FeedsSelection loadedSelection() const;

  public slots:
    // To disable persistent changes submissions.
    inline bool submitAll() {
      qFatal("Submitting changes via model is not allowed.");
      return false;
    }

    void updateDateFormat();
    void reloadWholeLayout();

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
    void loadMessages(const FeedsSelection &selection);

    void filterMessages(MessageFilter filter);

  signals:
    // Emitted if some persistent change is made which affects count of "unread/all" messages.
    void messageCountsChanged(FeedsSelection::SelectionMode mode, bool total_msg_count_changed, bool any_msg_restored);

  protected:
    // Sets up header data.
    void setupHeaderData();

    // Creates "normal" and "bold" fonts.
    void setupFonts();

    // Sets up all icons which are used directly by this model.
    void setupIcons();

  private:
    MessageFilter m_messageFilter;

    QString m_customDateFormat;
    FeedsSelection m_currentSelection;
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
