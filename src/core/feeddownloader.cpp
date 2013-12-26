#include "core/feeddownloader.h"

#include "core/feedsmodelfeed.h"

#include <QThread>
#include <QDebug>
#include <QApplication>


FeedDownloader::FeedDownloader(QObject *parent) : QObject(parent) {
}

FeedDownloader::~FeedDownloader() {
  qDebug("Destroying FeedDownloader instance.");
}

void FeedDownloader::updateFeeds(const QList<FeedsModelFeed *> &feeds) {
  qDebug().nospace() << "Performing feed updates in thread: \'" <<
                        QThread::currentThreadId() << "\'.";

  for (int i = 0, total = feeds.size(); i < total; i++) {
    feeds.at(i)->update();

    qDebug("Made progress in feed updates: %d/%d (id of feed is %d).",
           i + 1, total, feeds.at(i)->id());

    emit progress(feeds.at(i), i + 1, total);
  }

  qDebug().nospace() << "Finished feed updates in thread: \'" <<
                        QThread::currentThreadId() << "\'.";

  // Update of feeds has finished.
  // NOTE: This means that now "update lock" can be unlocked
  // and feeds can be added/edited/deleted and application
  // can eventually quit.
  emit finished();
}
