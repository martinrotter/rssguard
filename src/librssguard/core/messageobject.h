// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef MESSAGEOBJECT_H
#define MESSAGEOBJECT_H

#include <QObject>

#include "services/abstract/label.h"

class MessageObject : public QObject {
  Q_OBJECT

  Q_PROPERTY(QList<Label*> assignedLabels READ assignedLabels)
  Q_PROPERTY(QList<Label*> availableLabels READ availableLabels)
  Q_PROPERTY(QString feedCustomId READ feedCustomId)
  Q_PROPERTY(int accountId READ accountId)
  Q_PROPERTY(int id READ id)
  Q_PROPERTY(QString customId READ customId)
  Q_PROPERTY(QString title READ title WRITE setTitle)
  Q_PROPERTY(QString url READ url WRITE setUrl)
  Q_PROPERTY(QString author READ author WRITE setAuthor)
  Q_PROPERTY(QString contents READ contents WRITE setContents)
  Q_PROPERTY(QString rawContents READ rawContents WRITE setRawContents)
  Q_PROPERTY(QDateTime created READ created WRITE setCreated)
  Q_PROPERTY(bool createdIsMadeup READ createdIsMadeup WRITE setCreatedIsMadeup)
  Q_PROPERTY(double score READ score WRITE setScore)
  Q_PROPERTY(bool isRead READ isRead WRITE setIsRead)
  Q_PROPERTY(bool isImportant READ isImportant WRITE setIsImportant)
  Q_PROPERTY(bool isDeleted READ isDeleted WRITE setIsDeleted)
  Q_PROPERTY(bool runningFilterWhenFetching READ runningFilterWhenFetching)

  public:
    enum class FilteringAction {
      // Message is normally accepted and stored in DB or updated.
      Accept = 1,

      // Message is ignored and will not be stored in DB but is not purged if it already exists.
      Ignore = 2,

      // Message is purged from DB if it already exists.
      Purge = 4
    };

    Q_ENUM(FilteringAction)

    enum class DuplicateCheck {
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
      AllFeedsSameAccount = 16,

      // Messages with same custom ID as provided by feed/service.
      SameCustomId = 32
    };

    Q_ENUM(DuplicateCheck)

    explicit MessageObject(QSqlDatabase* db,
                           const QString& feed_custom_id,
                           int account_id,
                           const QList<Label*>& available_labels,
                           bool is_new_message,
                           QObject* parent = nullptr);

    void setMessage(Message* message);

    // Check if message is duplicate with another messages in DB.
    Q_INVOKABLE bool isAlreadyInDatabase(MessageObject::DuplicateCheck attribute_check) const;
    Q_INVOKABLE bool isDuplicate(MessageObject::DuplicateCheck attribute_check) const;
    Q_INVOKABLE bool isDuplicateWithAttribute(MessageObject::DuplicateCheck attribute_check) const;

    // Adds given label to list of assigned labels to this message.
    // Returns true if label was assigned now or if the message already has it assigned.
    Q_INVOKABLE bool assignLabel(const QString& label_custom_id) const;

    // Removes given label from list of assigned labels of this message.
    // Returns true if label was now removed or if it is not assigned to the message at all.
    Q_INVOKABLE bool deassignLabel(const QString& label_custom_id) const;

    // Returns list of assigned and available messages.
    QList<Label*> assignedLabels() const;
    QList<Label*> availableLabels() const;

    bool runningFilterWhenFetching() const;

    // Generic Message's properties bindings.
    QString feedCustomId() const;

    int accountId() const;

    QString customId() const;

    int id() const;

    QString title() const;
    void setTitle(const QString& title);

    QString url() const;
    void setUrl(const QString& url);

    QString author() const;
    void setAuthor(const QString& author);

    QString contents() const;
    void setContents(const QString& contents);

    QString rawContents() const;
    void setRawContents(const QString& raw_contents);

    QDateTime created() const;
    void setCreated(const QDateTime& created);

    bool createdIsMadeup() const;
    void setCreatedIsMadeup(bool madeup);

    bool isRead() const;
    void setIsRead(bool is_read);

    bool isImportant() const;
    void setIsImportant(bool is_important);

    bool isDeleted() const;
    void setIsDeleted(bool is_deleted);

    double score() const;
    void setScore(double score);

  private:
    QSqlDatabase* m_db;
    QString m_feedCustomId;
    int m_accountId;
    Message* m_message;
    QList<Label*> m_availableLabels;
    bool m_runningAfterFetching;
};

inline MessageObject::DuplicateCheck operator|(MessageObject::DuplicateCheck lhs,
                                               MessageObject::DuplicateCheck rhs) {
  return static_cast<MessageObject::DuplicateCheck>(int(lhs) | int(rhs));
}

inline MessageObject::DuplicateCheck operator&(MessageObject::DuplicateCheck lhs,
                                               MessageObject::DuplicateCheck rhs) {
  return static_cast<MessageObject::DuplicateCheck>(int(lhs) & int(rhs));
}

#endif // MESSAGEOBJECT_H
