#ifndef FEEDDOWNLOADER_H
#define FEEDDOWNLOADER_H

#include <QObject>


class FeedDownloader : public QObject {
    Q_OBJECT

  public:
    explicit FeedDownloader(QObject *parent = 0);

  signals:

  public slots:

};

#endif // FEEDDOWNLOADER_H
