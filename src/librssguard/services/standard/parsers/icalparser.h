// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ICALPARSER_H
#define ICALPARSER_H

#include "services/standard/parsers/feedparser.h"

class IcalendarComponent {
  public:
    QString uid() const;
    void setUid(const QString& uid);

  private:
    QString m_uid;
};

class EventComponent : public IcalendarComponent {
  public:
    QString title() const;
    void setTitle(const QString& title);

    QString description() const;
    void setDescription(const QString& description);

    QDateTime created() const;
    void setCreated(const QDateTime& created);

  private:
    QString m_title;
    QString m_description;
    QDateTime m_created;
};

class Icalendar : public FeedParser {
  public:
    explicit Icalendar(const QByteArray& data = {});

    QString title() const;
    void setTitle(const QString& title);

  private:
    void processLines(const QString& data);
    void processComponentCalendar(const QString& body);
    void processComponentEvent(const QString& body);

    QDateTime parseDateTime(const QString& date_time) const;
    QMap<QString, QString> tokenizeBody(const QString& body) const;

  private:
    QString m_title;
    QList<IcalendarComponent> m_components;
};

class IcalParser : public FeedParser {
  public:
    explicit IcalParser(const QString& data);
    virtual ~IcalParser();

    virtual QList<StandardFeed*> discoverFeeds(ServiceRoot* root, const QUrl& url, bool greedy) const;

    virtual QPair<StandardFeed*, QList<IconLocation>> guessFeed(const QByteArray& content,
                                                                const QString& content_type) const;
};

#endif // ICALPARSER_H
