#include "core/feeddownloader.h"

#include "core/feedsmodelfeed.h"
#include "core/systemfactory.h"
#include "core/silentnetworkaccessmanager.h"

#include <QThread>
#include <QDebug>
#include <QReadWriteLock>
#include <QApplication>


QPointer<SilentNetworkAccessManager> FeedDownloader::m_networkManager;

FeedDownloader::FeedDownloader(QObject *parent) : QObject(parent) {
}

FeedDownloader::~FeedDownloader() {
  qDebug("Destroying FeedDownloader instance.");
}

SilentNetworkAccessManager *FeedDownloader::globalNetworkManager() {
  if (m_networkManager.isNull()) {
    m_networkManager = new SilentNetworkAccessManager(qApp);
  }

  return m_networkManager;
}

void FeedDownloader::updateFeeds(const QList<FeedsModelFeed *> &feeds) {
  qDebug().nospace() << "Performing feed updates in thread: \'" <<
                        QThread::currentThreadId() << "\'.";

  for (int i = 0, total = feeds.size(); i < total; i++) {
    feeds.at(i)->update();

    qDebug("Made progress in feed updates: %d/%d.", i + 1, total);

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
