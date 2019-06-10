// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGE_H
#define MESSAGE_H

#include "definitions/definitions.h"

#include <QDataStream>
#include <QDateTime>
#include <QSqlRecord>
#include <QStringList>

// Represents single enclosure.
struct Enclosure {
  public:
    explicit Enclosure(QString url = QString(), QString mime = QString());

    QString m_url;
    QString m_mimeType;
};

// Represents single enclosure.
class Enclosures {
  public:
    static QList<Enclosure> decodeEnclosuresFromString(const QString& enclosures_data);
    static QString encodeEnclosuresToString(const QList<Enclosure>& enclosures);
};

// Represents single message.
class Message {
  public:
    explicit Message();

    // Creates Message from given record, which contains
    // row from query SELECT * FROM Messages WHERE ....;
    static Message fromSqlRecord(const QSqlRecord& record, bool* result = nullptr);
    QString m_title;
    QString m_url;
    QString m_author;
    QString m_contents;
    QDateTime m_created;
    QString m_feedId;
    int m_accountId;
    int m_id;
    QString m_customId;
    QString m_customHash;
    bool m_isRead;
    bool m_isImportant;

    QList<Enclosure> m_enclosures;

    // Is true if "created" date was obtained directly
    // from the feed, otherwise is false
    bool m_createdFromFeed = false;

    friend inline bool operator==(const Message& lhs, const Message& rhs) {
      return lhs.m_accountId == rhs.m_accountId && lhs.m_id == rhs.m_id;
    }

    friend inline bool operator!=(const Message& lhs, const Message& rhs) {
      return !(lhs == rhs);
    }

};

// Serialize message state.
// NOTE: This is used for persistent caching of message state changes.
QDataStream& operator<<(QDataStream& out, const Message& myObj);
QDataStream& operator>>(QDataStream& in, Message& myObj);

uint qHash(Message key, uint seed);
uint qHash(const Message& key);

#endif // MESSAGE_H
