#include "core/feeddownloader.h"

#include "core/feedsmodelfeed.h"

#include <QThread>
#include <QDebug>


FeedDownloader::FeedDownloader(QObject *parent) : QObject(parent) {
}

FeedDownloader::~FeedDownloader() {
  qDebug("Destroying FeedDownloader instance.");
}

void FeedDownloader::updateFeeds(const QList<FeedsModelFeed *> &feeds) {
  qDebug().nospace() << "Creating main application form in thread: \'" <<
                        QThread::currentThreadId() << "\'.";
}
