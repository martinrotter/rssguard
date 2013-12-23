#ifndef FEEDDOWNLOADER_H
#define FEEDDOWNLOADER_H

#include <QObject>
#include <QPointer>


class FeedsModelFeed;
class SilentNetworkAccessManager;

// This class offers means to "update" feeds
// and "special" categories.
// NOTE: This class is used within separate thread.
class FeedDownloader : public QObject {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit FeedDownloader(QObject *parent = 0);
    virtual ~FeedDownloader();

  signals:
    // Emitted if all items from update queue are
    // processed.
    void finished();

    // Emitted if any item is processed.
    // "Current" number indicates count of processed feeds
    // and "total" number indicates total number of feeds
    // which are in the initial queue.
    void progress(FeedsModelFeed *feed, int current, int total);

  public slots:
    // Performs update of all feeds from the "feeds" parameter.
    // New messages are downloaded for each feed and they
    // are stored persistently in the database.
    // Appropriate signals are emitted.
    void updateFeeds(const QList<FeedsModelFeed*> &feeds);
};

#endif // FEEDDOWNLOADER_H
