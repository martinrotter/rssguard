#ifndef SYSTEMTRAYICON_H
#define SYSTEMTRAYICON_H

#include <QSystemTrayIcon>


class SystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT
  public:
    explicit SystemTrayIcon(QObject *parent = 0);
    
  signals:
    
  public slots:
    
};

#endif // SYSTEMTRAYICON_H
