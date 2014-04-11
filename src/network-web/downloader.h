#ifndef DOWNLOADER_H
#define DOWNLOADER_H

#include <QObject>


class Downloader : public QObject {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit Downloader(QObject *parent = 0);
    virtual ~Downloader();

    // TODO: zakladni downloader s timeoutem a signalem kerej informuje o prubehu
    // stahovani

  signals:

  public slots:

};

#endif // DOWNLOADER_H
