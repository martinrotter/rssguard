// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SYSTEMTRAYICON_H
#define SYSTEMTRAYICON_H

#include "definitions/definitions.h"

#include <functional>

#include <QMenu>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <QTimer>

class FormMain;
class QEvent;

#if defined(Q_OS_WIN)

class TrayIconMenu : public QMenu {
    Q_OBJECT

  public:
    // Constructors and destructors.
    explicit TrayIconMenu(const QString& title, QWidget* parent);

  protected:
    bool event(QEvent* event);
};

#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QDBusConnection>
#include <QDBusConnectionInterface>

class DbusTrayStatusController {
  public:
    bool available() const {
      return QDBusConnection::sessionBus().interface()->isServiceRegistered(QSL("org.kde.StatusNotifierWatcher"));
    }

    QString service() const {
      auto iface = QDBusConnection::sessionBus().interface();
      const QStringList services = iface->registeredServiceNames();

      for (const QString& s : services) {
        if (s.startsWith(QSL("org.kde.StatusNotifierItem"))) {
          return s;
        }
      }

      return {};
    }

    void setStatus(const QString& status) {
      QString s = service();

      if (s.isEmpty()) {
        return;
      }

      QDBusMessage msg = QDBusMessage::createMethodCall(s,
                                                        QSL("/StatusNotifierItem"),
                                                        QSL("org.freedesktop.DBus.Properties"),
                                                        QSL("Set"));

      msg << QSL("org.freedesktop.StatusNotifierItem") << QSL("Status") << QVariant::fromValue(QDBusVariant(status));

      QDBusConnection::sessionBus().send(msg);
    }
};
#endif

class SystemTrayIcon : public QSystemTrayIcon {
    Q_OBJECT

  public:
    explicit SystemTrayIcon(const QString& normal_icon, const QString& plain_icon, FormMain* parent = nullptr);
    virtual ~SystemTrayIcon();

    // Sets the number to be visible in the tray icon, number <= 0 removes it.
    void setNumber(int number = -1, bool any_feed_has_new_unread_messages = false);

    void showMessage(const QString& title,
                     const QString& message,
                     MessageIcon icon = Information,
                     int milliseconds_timeout_hint = TRAY_ICON_BUBBLE_TIMEOUT,
                     const std::function<void()>& functor = nullptr);

    // Returns true if tray area is available and icon can be displayed.
    static bool isSystemTrayAreaAvailable();

    // Returns true if user wants to have tray icon displayed.
    static bool isSystemTrayDesired();

  public slots:
    void show();

  private slots:
    void showPrivate();
    void onActivated(QSystemTrayIcon::ActivationReason reason);

  signals:
    void shown();

  private:
    QIcon m_normalIcon;
    QPixmap m_plainPixmap;
    QFont m_font = QFont();
    QTimer m_tmrDoubleFire;

    QMetaObject::Connection m_connection;
};

#endif // SYSTEMTRAYICON_H
