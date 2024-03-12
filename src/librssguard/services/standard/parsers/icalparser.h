// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ICALPARSER_H
#define ICALPARSER_H

#include "services/standard/parsers/feedparser.h"

class IcalendarComponent {
  public:
    QString uid() const;

    QVariantMap properties() const;
    void setProperties(const QVariantMap& properties);

  protected:
    QVariant getPropertyValue(const QString& property_name) const;

    QVariantMap m_properties;
};

Q_DECLARE_METATYPE(IcalendarComponent)

class EventComponent : public IcalendarComponent {
  public:
    QDateTime startsOn() const;
    QDateTime endsOn() const;
    QString title() const;
    QString url() const;
    QString organizer() const;
    QString location() const;
    QString description() const;
    QDateTime created() const;
    QDateTime lastModified() const;
};

Q_DECLARE_METATYPE(EventComponent)

class Icalendar : public FeedParser {
    friend class IcalParser;

  public:
    explicit Icalendar(const QByteArray& data = {});

    QString title() const;
    void setTitle(const QString& title);

  private:
    void processLines(const QString& data);
    void processComponentCalendar(const QString& body);
    void processComponentEvent(const QString& body);

    QDateTime parseDateTime(const QString& date_time) const;
    QVariantMap tokenizeBody(const QString& body) const;

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

    virtual QVariantList objMessageElements();
    virtual QString objMessageTitle(const QVariant& msg_element) const;
    virtual QString objMessageUrl(const QVariant& msg_element) const;
    virtual QString objMessageDescription(const QVariant& msg_element) const;
    virtual QString objMessageAuthor(const QVariant& msg_element) const;
    virtual QDateTime objMessageDateCreated(const QVariant& msg_element) const;
    virtual QString objMessageId(const QVariant& msg_element) const;
    virtual QList<Enclosure> objMessageEnclosures(const QVariant& msg_element) const;
    virtual QList<MessageCategory> objMessageCategories(const QVariant& msg_element) const;
    virtual QString objMessageRawContents(const QVariant& msg_element) const;

  private:
    Icalendar m_iCalendar;
};

#endif // ICALPARSER_H
