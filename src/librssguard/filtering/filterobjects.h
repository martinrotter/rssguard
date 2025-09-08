// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FILTEROBJECTS_H
#define FILTEROBJECTS_H

#include "filtering/filtermessage.h"
#include "services/abstract/label.h"

#include <QDateTime>
#include <QDomElement>
#include <QObject>

class FilteringSystem;

// Information about the application, access to DB etc.
class FilterApp : public QObject {
    Q_OBJECT

  public:
    Q_PROPERTY(QList<Label*> availableLabels READ availableLabels)

    void setSystem(FilteringSystem* sys);

    Q_INVOKABLE QString findLabel(const QString& label_title) const;
    Q_INVOKABLE QString createLabel(const QString& label_title, const QString& hex_color = {});
    Q_INVOKABLE void showNotification(const QString& title, const QString& text);

    QList<Label*> availableLabels() const;

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

class FilterFeed : public QObject {
    Q_OBJECT

  public:
    Q_PROPERTY(QString title READ title)

    QString title() const;

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
    Q_INVOKABLE QString runExecutableGetOutput(const QString& executable,
                                               const QStringList& arguments = {},
                                               const QString& working_directory = {}) const;
    Q_INVOKABLE void runExecutable(const QString& executable,
                                   const QStringList& arguments = {},
                                   const QString& working_directory = {}) const;

  private:
    FilteringSystem* m_system;
};

#endif // FILTEROBJECTS_H
