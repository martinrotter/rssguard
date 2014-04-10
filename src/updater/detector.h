#ifndef DETECTOR_H
#define DETECTOR_H

#include <QObject>


class Detector : public QObject {
    Q_OBJECT

  public:
    explicit Detector(QObject *parent = 0);

  public slots:
    void handleMessage(const QString& message);
};

#endif // DETECTOR_H
