#ifndef SYSTEMTRAYICON_H
#define SYSTEMTRAYICON_H

#include <QSystemTrayIcon>
#include <QPointer>
#include <QPixmap>
#include <QMenu>


class FormMain;
class QEvent;

#if defined(Q_OS_WIN)
class TrayIconMenu : public QMenu {
  public:
    // Constructors and destructors.
    explicit TrayIconMenu(const QString &title, QWidget *parent);
    virtual ~TrayIconMenu();

  protected:
    bool event(QEvent *event);
};
#endif

class SystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit SystemTrayIcon(const QString &normal_icon,
                            const QString &plain_icon,
                            FormMain *parent = 0);
    virtual ~SystemTrayIcon();

    // Returns true if tray icon CAN be constructed on this machine.
    static bool isSystemTrayAvailable();

    // Returns true if tray icon CAN be costructed and IS enabled in
    // application settings.
    static bool isSystemTrayActivated();

    // Creates new tray icon if necessary and returns it.
    // WARNING: Use this in cooperation with SystemTrayIcon::isSystemTrayActivated().
    static SystemTrayIcon *instance();
    
    // Sets the number to be visible in the tray icon, number <= 0 removes it.
    void setNumber(int number = -1);

    // Explicitle clears SystemTrayIcon instance from the memory.
    static void deleteInstance();
    
  public slots:
    void show();

  private slots:
    void showPrivate();
    void onActivated(const QSystemTrayIcon::ActivationReason &reason);

  private:
    QIcon m_normalIcon;
    QPixmap m_plainPixmap;
    QFont m_font;

    static QPointer<SystemTrayIcon> s_trayIcon;
};

#endif // SYSTEMTRAYICON_H
