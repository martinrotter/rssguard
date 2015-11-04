#ifndef MESSAGE_H
#define MESSAGE_H

#include "definitions/definitions.h"

#include <QDateTime>
#include <QStringList>

// Represents single enclosuresh

struct Enclosure {
    QString m_url;
    QString m_mimeType;

    explicit Enclosure(const QString &url = QString(), const QString &mime = QString());
};

// Represents single enclosure.
class Enclosures {
  public:
    static QList<Enclosure> decodeEnclosuresFromString(const QString &enclosures_data);
    static QString encodeEnclosuresToString(const QList<Enclosure> &enclosures);
};

// Represents single message.
class Message {
  public:
    explicit Message();

    QString m_title;
    QString m_url;
    QString m_author;
    QString m_contents;
    QDateTime m_created;
    int m_feedId;

    QList<Enclosure> m_enclosures;

    // Is true if "created" date was obtained directly
    // from the feed, otherwise is false
    bool m_createdFromFeed;
};

#endif // MESSAGE_H
