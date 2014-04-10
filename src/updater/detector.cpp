#include "updater/detector.h"


Detector::Detector(QObject *parent) : QObject(parent) {
}

void Detector::handleMessage(const QString &message) {
  if (message == "app_is_running") {
    qDebug("Detected another instance of RSS Guard/Updater was starting...");
  }
}
