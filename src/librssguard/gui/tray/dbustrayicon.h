// trayiconstatusnotifier.h
#ifndef TRAYICONSTATUSNOTIFIER_H
#define TRAYICONSTATUSNOTIFIER_H

#include "trayicon.h"

#include <QDBusArgument>
#include <QDBusConnection>

class QDBusServiceWatcher;

struct DBusToolTipStruct {
    QString icon;
    QList<QVariantList> image; // Simplified - empty for now
    QString title;
    QString description;
};

Q_DECLARE_METATYPE(DBusToolTipStruct)

QDBusArgument& operator<<(QDBusArgument& argument, const DBusToolTipStruct& toolTip);
const QDBusArgument& operator>>(const QDBusArgument& argument, DBusToolTipStruct& toolTip);

class TrayIconStatusNotifier : public TrayIcon {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierItem")

    // DBus properties - simple types only
    Q_PROPERTY(QString Category READ category)
    Q_PROPERTY(QString Id READ id)
    Q_PROPERTY(QString Title READ title)
    Q_PROPERTY(QString Status READ status)
    Q_PROPERTY(int WindowId READ windowId)
    Q_PROPERTY(QString IconName READ iconName)
    Q_PROPERTY(DBusToolTipStruct ToolTip READ toolTip)

  public:
    explicit TrayIconStatusNotifier(const QString& id,
                                    const QString& title,
                                    const QPixmap& normal_icon,
                                    const QPixmap& plain_icon,
                                    QObject* parent = nullptr);
    ~TrayIconStatusNotifier() override;

    // TrayIcon interface implementation
    void setToolTip(const QString& tool_tip) override;
    void setPixmap(const QPixmap& icon) override;
    void setStatus(Status status) override;
    void setContextMenu(TrayIconMenu* menu) override;
    void showMessage(const QString& title,
                     const QString& message,
                     MessageSeverity icon = MessageSeverity::Information,
                     int milliseconds_timeout_hint = TRAY_ICON_BUBBLE_TIMEOUT,
                     const std::function<void()>& message_clicked_callback = nullptr) override;
    bool isAvailable() const override;

  public slots:
    void show() override;
    void hide() override;

    // DBus methods
    void Activate(int x, int y);
    void ContextMenu(int x, int y);

  signals:
    // DBus signals
    void NewTitle();
    void NewIcon();
    void NewToolTip();
    void NewStatus(const QString& status);

  private:
    // DBus property getters
    QString category() const {
      return QStringLiteral("ApplicationStatus");
    }
    QString id() const {
      return m_dbusId;
    }
    QString title() const {
      return m_dbusTitle;
    }
    QString status() const;
    int windowId() const {
      return 0;
    }
    QString iconName() const {
      return m_iconName;
    }
    DBusToolTipStruct toolTip() const;

    // Helper methods
    bool registerToStatusNotifierWatcher();
    void unregisterFromStatusNotifierWatcher();

  private slots:
    void onServiceOwnerChanged(const QString& service, const QString& oldOwner, const QString& newOwner);

  private:
    QString m_dbusService;
    QString m_dbusPath;
    QString m_iconName;
    QString m_toolTip;
    Status m_status;
    TrayIconMenu* m_menu;

    QDBusServiceWatcher* m_watcher;
    bool m_registered;

    static int s_instanceCounter;
};

#endif // TRAYICONSTATUSNOTIFIER_H
