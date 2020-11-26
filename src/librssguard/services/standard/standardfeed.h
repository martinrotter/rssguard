// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef FEEDSMODELFEED_H
#define FEEDSMODELFEED_H

#include "services/abstract/feed.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QMetaType>
#include <QNetworkReply>
#include <QPair>
#include <QSqlRecord>

class StandardServiceRoot;

// Represents BASE class for feeds contained in FeedsModel.
// NOTE: This class should be derived to create PARTICULAR feed types.
class StandardFeed : public Feed {
  Q_OBJECT

  public:
    enum class Type {
      Rss0X = 0,
      Rss2X = 1,
      Rdf = 2,      // Sometimes denoted as RSS 1.0.
      Atom10 = 3,
      Json = 4
    };

    // Constructors and destructors.
    explicit StandardFeed(RootItem* parent_item = nullptr);
    explicit StandardFeed(const StandardFeed& other);
    explicit StandardFeed(const QSqlRecord& record);
    virtual ~StandardFeed();

    StandardServiceRoot* serviceRoot() const;

    QList<QAction*> contextMenuFeedsList();

    QString additionalTooltip() const;

    bool canBeEdited() const;
    bool canBeDeleted() const;

    bool editViaGui();
    bool deleteViaGui();

    // Obtains data related to this feed.
    Qt::ItemFlags additionalFlags() const;
    bool performDragDropChange(RootItem* target_item);

    bool addItself(RootItem* parent);
    bool editItself(StandardFeed* new_feed_data);
    bool removeItself();

    // Other getters/setters.
    Type type() const;
    void setType(Type type);

    QString encoding() const;
    void setEncoding(const QString& encoding);

    QNetworkReply::NetworkError networkError() const;

    QList<Message> obtainNewMessages(bool* error_during_obtaining);

    // Tries to guess feed hidden under given URL
    // and uses given credentials.
    // Returns pointer to guessed feed (if at least partially
    // guessed) and retrieved error/status code from network layer
    // or NULL feed.
    static QPair<StandardFeed*, QNetworkReply::NetworkError> guessFeed(const QString& url,
                                                                       const QString& username = QString(),
                                                                       const QString& password = QString());

    // Converts particular feed type to string.
    static QString typeToString(Type type);

  public slots:
    void fetchMetadataForItself();

  private:
    Type m_type;

    QNetworkReply::NetworkError m_networkError;
    QString m_encoding;
};

Q_DECLARE_METATYPE(StandardFeed::Type)

#endif // FEEDSMODELFEED_H
