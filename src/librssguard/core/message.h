// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGE_H
#define MESSAGE_H

#include "definitions/definitions.h"

#include <QDataStream>
#include <QDateTime>
#include <QSqlRecord>
#include <QStringList>

class QSqlDatabase;

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

class Label;

// Represents single message.
class Message {
  public:
    explicit Message();

    // Creates Message from given record, which contains
    // row from query SELECT * FROM Messages WHERE ....;
    static Message fromSqlRecord(const QSqlRecord& record, bool* result = nullptr);

  public:
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

    // List of custom IDs of labels assigned to this message.
    QList<Label*> m_assignedLabels;

    // Is true if "created" date was obtained directly
    // from the feed, otherwise is false
    bool m_createdFromFeed = false;
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
QDataStream& operator<<(QDataStream& out, const Message& myObj);
QDataStream& operator>>(QDataStream& in, Message& myObj);

uint qHash(const Message& key, uint seed);
uint qHash(const Message& key);

enum class FilteringAction {
  // Message is normally accepted and stored in DB.
  Accept = 1,

  // Message is ignored and now stored in DB.
  Ignore = 2
};

enum class DuplicationAttributeCheck {
  // Message with same title in DB.
  SameTitle = 1,

  // Message with same URL in DB.
  SameUrl = 2,

  // Message with same author in DB.
  SameAuthor = 4,

  // Messages with same creation date in DB.
  SameDateCreated = 8,

  // Compare with all messages from the account not only with messages from same feed.
  // Note that this value must be used via bitwise OR with other values,
  // for example 2 | 4 | 16.
  AllFeedsSameAccount = 16
};

inline DuplicationAttributeCheck operator|(DuplicationAttributeCheck lhs, DuplicationAttributeCheck rhs) {
  return static_cast<DuplicationAttributeCheck>(int(lhs) | int(rhs));
}

inline DuplicationAttributeCheck operator&(DuplicationAttributeCheck lhs, DuplicationAttributeCheck rhs) {
  return static_cast<DuplicationAttributeCheck>(int(lhs) & int(rhs));
}

class MessageObject : public QObject {
  Q_OBJECT

  Q_PROPERTY(QList<Label*> assignedLabels READ assignedLabels)
  Q_PROPERTY(QString feedCustomId READ feedCustomId)
  Q_PROPERTY(int accountId READ accountId)
  Q_PROPERTY(QString title READ title WRITE setTitle)
  Q_PROPERTY(QString url READ url WRITE setUrl)
  Q_PROPERTY(QString author READ author WRITE setAuthor)
  Q_PROPERTY(QString contents READ contents WRITE setContents)
  Q_PROPERTY(QDateTime created READ created WRITE setCreated)
  Q_PROPERTY(bool isRead READ isRead WRITE setIsRead)
  Q_PROPERTY(bool isImportant READ isImportant WRITE setIsImportant)

  public:
    explicit MessageObject(QSqlDatabase* db, const QString& feed_custom_id, int account_id, QObject* parent = nullptr);

    void setMessage(Message* message);

    // Check if message is duplicate with another messages in DB.
    // Parameter "attribute_check" is DuplicationAttributeCheck enum
    // value casted to int.
    Q_INVOKABLE bool isDuplicateWithAttribute(int attribute_check) const;

    QList<Label*> assignedLabels() const;

    // Generic Message's properties bindings.
    QString feedCustomId() const;
    int accountId() const;

    QString title() const;
    void setTitle(const QString& title);

    QString url() const;
    void setUrl(const QString& url);

    QString author() const;
    void setAuthor(const QString& author);

    QString contents() const;
    void setContents(const QString& contents);

    QDateTime created() const;
    void setCreated(const QDateTime& created);

    bool isRead() const;
    void setIsRead(bool is_read);

    bool isImportant() const;
    void setIsImportant(bool is_important);

  private:
    QSqlDatabase* m_db;
    QString m_feedCustomId;
    int m_accountId;
    Message* m_message;
};

#endif // MESSAGE_H
