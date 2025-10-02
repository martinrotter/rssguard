// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGE_H
#define MESSAGE_H

// #include "definitions/definitions.h"

#include "services/abstract/feedrtlbehavior.h"

#include <QDataStream>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringList>

class QSqlDatabase;
class Label;

// Represents single enclosure.
class RSSGUARD_DLLSPEC Enclosure : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString url READ url WRITE setUrl)
    Q_PROPERTY(QString mimeType READ mimeType WRITE setMimeType)

  public:
    explicit Enclosure(QString url = QString(), QString mime = QString(), QObject* parent = nullptr);
    Enclosure(const Enclosure& other);
    ~Enclosure();

    QString url() const;
    void setUrl(const QString& url);

    QString mimeType() const;
    void setMimeType(const QString& mime);

  private:
    QString m_url;
    QString m_mimeType;
};

// Represents single enclosure.
class RSSGUARD_DLLSPEC Enclosures {
  public:
    static QList<QSharedPointer<Enclosure>> decodeEnclosuresFromString(const QString& enclosures_data);
    static QString encodeEnclosuresToString(const QList<QSharedPointer<Enclosure>>& enclosures);
};

class Feed;

// Represents RSS, JSON or ATOM category (or tag, label - depending on terminology of each entry.
class RSSGUARD_DLLSPEC MessageCategory : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString title READ title)

  public:
    explicit MessageCategory(const QString& title, QObject* parent = nullptr);
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
    Message(const Message& other);
    ~Message();

    void sanitize(const Feed* feed, bool fix_future_datetimes);

    static Message fromSqlQuery(const QSqlQuery& record);
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
    QString m_feedTitle;
    int m_accountId;
    int m_id;
    QString m_customId;
    QString m_customHash;
    bool m_isRead;
    bool m_isImportant;
    bool m_isDeleted;
    double m_score;
    RtlBehavior m_rtlBehavior;
    QList<QSharedPointer<Enclosure>> m_enclosures;

    // List of assigned labels.
    // This field is only used when fetching entries of a feed.
    QList<MessageCategory*> m_categories;

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
