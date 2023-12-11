// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGE_H
#define MESSAGE_H

// #include "definitions/definitions.h"

#include <QDataStream>
#include <QDateTime>
#include <QSqlRecord>
#include <QStringList>

class QSqlDatabase;
class Label;

// Represents single enclosure.
struct RSSGUARD_DLLSPEC Enclosure {
  public:
    explicit Enclosure(QString url = QString(), QString mime = QString());

    QString m_url;
    QString m_mimeType;
};

// Represents single enclosure.
class RSSGUARD_DLLSPEC Enclosures {
  public:
    static QList<Enclosure> decodeEnclosuresFromString(const QString& enclosures_data);
    static QJsonArray encodeEnclosuresToJson(const QList<Enclosure>& enclosures);
    static QString encodeEnclosuresToString(const QList<Enclosure>& enclosures);
};

class Feed;

// Represents RSS, JSON or ATOM category (or tag, label - depending on terminology of each entry.
class RSSGUARD_DLLSPEC MessageCategory : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString title READ title)

  public:
    explicit MessageCategory(const QString& title);
    MessageCategory(const MessageCategory& other);

    QString title() const;

    MessageCategory& operator=(const MessageCategory& other);

  private:
    QString m_title;
};

// Represents single message.
class RSSGUARD_DLLSPEC Message {
  public:
    explicit Message();

    void sanitize(const Feed* feed, bool fix_future_datetimes);

    QJsonObject toJson() const;

    // Creates Message from given record, which contains
    // row from query SELECT * FROM Messages WHERE ....;
    static Message fromSqlRecord(const QSqlRecord& record, bool* result = nullptr);
    static QString generateRawAtomContents(const Message& msg);

  public:
    QString m_title;
    QString m_url;
    QString m_author;
    QString m_contents;
    QString m_rawContents;

    // This should be preferably in UTC and should be converted
    // to localtime when needed.
    QDateTime m_created;
    QString m_feedId;
    int m_accountId;
    int m_id;
    QString m_customId;
    QString m_customHash;
    bool m_isRead;
    bool m_isImportant;
    bool m_isDeleted;
    double m_score;
    bool m_isRtl;
    QList<Enclosure> m_enclosures;

    // List of assigned labels.
    // This field is only field when fetching entries of a feed.
    QList<MessageCategory> m_categories;

    // List of custom IDs of labels assigned to this message.
    QList<Label*> m_assignedLabels;

    QList<Label*> m_assignedLabelsByFilter;
    QList<Label*> m_deassignedLabelsByFilter;

    QStringList m_assignedLabelsIds;

    // Is true if "created" date was obtained directly
    // from the feed, otherwise is false
    bool m_createdFromFeed = false;

    // Notice if the article was actually inserted/updated into DB when feed was fetched.
    bool m_insertedUpdated = false;
};

inline bool operator==(const Message& lhs, const Message& rhs) {
  return lhs.m_accountId == rhs.m_accountId &&
         ((lhs.m_id > 0 && rhs.m_id > 0 && lhs.m_id == rhs.m_id) ||
          (!lhs.m_customId.isEmpty() && !rhs.m_customId.isEmpty() && lhs.m_customId == rhs.m_customId));
}

inline bool operator!=(const Message& lhs, const Message& rhs) {
  return !(lhs == rhs);
}

// Serialize message state.
// NOTE: This is used for persistent caching of message state changes.
RSSGUARD_DLLSPEC QDataStream& operator<<(QDataStream& out, const Message& my_obj);
RSSGUARD_DLLSPEC QDataStream& operator>>(QDataStream& in, Message& my_obj);

uint qHash(const Message& key, uint seed);
uint qHash(const Message& key);

#endif // MESSAGE_H
