// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTEROBJECTS_H
#define FILTEROBJECTS_H

#include "services/abstract/label.h"

#include <QDateTime>
#include <QDomElement>
#include <QObject>

class FilteringSystem;

class FilterMessage : public QObject {
    Q_OBJECT

    Q_PROPERTY(QList<MessageCategory*> categories READ categories)
    Q_PROPERTY(QList<Label*> assignedLabels READ assignedLabels)
    Q_PROPERTY(bool hasEnclosures READ hasEnclosures)
    Q_PROPERTY(int id READ id)
    Q_PROPERTY(QString customId READ customId WRITE setCustomId)
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

    enum class DuplicityCheck {
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

    Q_ENUM(DuplicityCheck)

    explicit FilterMessage(QObject* parent = nullptr);

    void setSystem(FilteringSystem* sys);
    void setMessage(Message* message);

    Q_INVOKABLE bool isAlreadyInDatabaseWinkler(DuplicityCheck criteria, double threshold = 0.1) const;
    Q_INVOKABLE bool isAlreadyInDatabase(DuplicityCheck criteria) const;

    // Adds given label to list of assigned labels to this message.
    // Returns true if label was assigned now or if the message already has it assigned.
    Q_INVOKABLE bool assignLabel(const QString& label_custom_id) const;

    // Removes given label from list of assigned labels of this message.
    // Returns true if label was now removed or if it is not assigned to the message at all.
    Q_INVOKABLE bool deassignLabel(const QString& label_custom_id) const;

    // Removes all labels from message.
    Q_INVOKABLE void deassignAllLabels() const;

    // Takes all categories of the message and creates/finds labels for them, then
    // assigns all the labels.
    Q_INVOKABLE void exportCategoriesToLabels(bool assign_to_message) const;

    // Add multimedia attachment to the message.
    Q_INVOKABLE void addEnclosure(const QString& url, const QString& mime_type) const;

    QList<Label*> assignedLabels() const;
    QList<MessageCategory*> categories() const;

    bool hasEnclosures() const;
    int id() const;

    QString customId() const;
    void setCustomId(const QString& custom_id);

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
    QString feedCustomId() const;

    Message* m_message;
    FilteringSystem* m_system;
};

// Information about the application, access to DB etc.
class FilterApp : public QObject {
    Q_OBJECT

  public:
    void setSystem(FilteringSystem* sys);

    Q_INVOKABLE void showNotification(const QString& title, const QString& text);

  private:
    FilteringSystem* m_system;
};

// Information about current filtering run.
class FilterRun : public QObject {
    Q_OBJECT

  public:
    Q_PROPERTY(int numberOfAcceptedMessages READ numberOfAcceptedMessages)
    Q_PROPERTY(int indexOfCurrentFilter READ indexOfCurrentFilter)
    Q_PROPERTY(int totalCountOfFilters READ totalCountOfFilters)

    explicit FilterRun(QObject* parent = nullptr);

    int numberOfAcceptedMessages() const;
    void incrementNumberOfAcceptedMessages();

    int indexOfCurrentFilter() const;
    void setIndexOfCurrentFilter(int idx);

    int totalCountOfFilters() const;
    void setTotalCountOfFilters(int total);

  private:
    int m_indexOfCurrentFilter;
    int m_totalCountOfFilters;
    int m_numberOfAcceptedMessages;
};

class FilterAccount : public QObject {
    Q_OBJECT

  public:
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(int id READ id)
    Q_PROPERTY(QList<Label*> availableLabels READ availableLabels)

    QString title() const;
    int id() const;

    void setSystem(FilteringSystem* sys);

    Q_INVOKABLE QString findLabel(const QString& label_title) const;
    Q_INVOKABLE QString createLabel(const QString& label_title, const QString& hex_color = {});
    QList<Label*> availableLabels() const;

  private:
    FilteringSystem* m_system;
};

class FilterFeed : public QObject {
    Q_OBJECT

  public:
    Q_PROPERTY(QString title READ title)
    Q_PROPERTY(QString customId READ customId)

    QString title() const;
    QString customId() const;

    void setSystem(FilteringSystem* sys);

  private:
    FilteringSystem* m_system;
};

class FilterFs : public QObject {
    Q_OBJECT

  public:
    Q_INVOKABLE QString runExecutableGetOutput(const QString& executable,
                                               const QStringList& arguments = {},
                                               const QString& stdin_data = {},
                                               const QString& working_directory = {}) const;
    Q_INVOKABLE void runExecutable(const QString& executable,
                                   const QStringList& arguments = {},
                                   const QString& working_directory = {}) const;

    void setSystem(FilteringSystem* sys);

  private:
    FilteringSystem* m_system;
};

// Misc utility functions for filtering.
class FilterUtils : public QObject {
    Q_OBJECT

  public:
    Q_PROPERTY(QString hostname READ hostname)

    explicit FilterUtils(QObject* parent = nullptr);
    virtual ~FilterUtils();

    void setSystem(FilteringSystem* sys);

    // Returns hostname or empty string if failed.
    QString hostname() const;

    // Converts XML -> JSON or returns empty string if failed.
    Q_INVOKABLE QString fromXmlToJson(const QString& xml) const;

    // Parses string into date/time object.
    Q_INVOKABLE QDateTime parseDateTime(const QString& dat) const;

  private:
    FilteringSystem* m_system;
};

#endif // FILTEROBJECTS_H
