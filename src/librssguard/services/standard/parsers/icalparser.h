// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef ICALPARSER_H
#define ICALPARSER_H

#include "services/standard/parsers/feedparser.h"

class IcalendarComponent {};

class EventComponent : public IcalendarComponent {};

class Icalendar {
  public:
    enum class ProcessingState {
      BeginCalendar,
      EndCalendar,
      BeginEvent,
      EndEvent
    }

    explicit Icalendar();
    explicit Icalendar(const QByteArray& data);

    QString title() const;
    void setTitle(const QString& title);

  private:
    void processLines(const QByteArray& data);

  private:
    QString m_title;
    QList<IcalendarComponent> m_components;
};

class IcalParser : public FeedParser {
  public:
    explicit IcalParser(const QString& data);
    virtual ~IcalParser();

    virtual QPair<StandardFeed*, QList<IconLocation>> guessFeed(const QByteArray& content,
                                                                const QString& content_type) const;
};

#endif // ICALPARSER_H
