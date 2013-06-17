#ifndef SYSTEMTRAYICON_H
#define SYSTEMTRAYICON_H

#include <QSystemTrayIcon>
#include <QPointer>


class SystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT
  public:
    explicit SystemTrayIcon(QObject *parent = 0);
    ~SystemTrayIcon();

    // Returns true if tray icon CAN be constructed on this machine.
    static bool isSystemTrayAvailable();

    // Returns true if tray icon CAN be costructed and IS enabled in
    // application settings.
    static bool isSystemTrayActivated();

    // Creates new tray icon if necessary and returns it.
    // WARNING: Use this in cooperation with SystemTrayIcon::isSystemTrayActivated().
    static SystemTrayIcon *getInstance();
    
    // TODO: Implement method for manual clearing of the tray icon. Creating of tray icon
    // handled by getInstance().
  signals:
    
  public slots:

  private:
    static QPointer<SystemTrayIcon> m_trayIcon;
};

#endif // SYSTEMTRAYICON_H
