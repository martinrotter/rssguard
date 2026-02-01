// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TRAYICONSTATUSNOTIFIER_H
#define TRAYICONSTATUSNOTIFIER_H

#include "gui/tray/trayicon.h"

#include <QDBusArgument>
#include <QDBusConnection>

class QDBusServiceWatcher;

struct DBusImageStruct {
    int width;
    int height;
    QByteArray data;
};

Q_DECLARE_METATYPE(DBusImageStruct)

QDBusArgument& operator<<(QDBusArgument& argument, const DBusImageStruct& image);
const QDBusArgument& operator>>(const QDBusArgument& argument, DBusImageStruct& image);

typedef QList<DBusImageStruct> DBusImageVector;
Q_DECLARE_METATYPE(DBusImageVector)

struct DBusToolTipStruct {
    QString icon;
    DBusImageVector image;
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
    Q_PROPERTY(DBusImageVector IconPixmap READ iconPixmap)
    Q_PROPERTY(DBusToolTipStruct ToolTip READ toolTip)

  public:
    explicit TrayIconStatusNotifier(const QString& id,
                                    const QString& title,
                                    const QPixmap& normal_icon,
                                    const QPixmap& plain_icon,
                                    QObject* parent = nullptr);
    virtual ~TrayIconStatusNotifier() override;

    // TrayIcon interface implementation
    virtual void setToolTip(const QString& tool_tip);
    virtual void setPixmap(const QPixmap& icon);
    virtual void setStatus(Status status);
    virtual void setContextMenu(TrayIconMenu* menu);
    virtual void showMessage(const QString& title,
                             const QString& message,
                             MessageSeverity icon = MessageSeverity::Information,
                             int milliseconds_timeout_hint = TRAY_ICON_BUBBLE_TIMEOUT,
                             const std::function<void()>& message_clicked_callback = nullptr);
    virtual bool isAvailable() const;
    virtual void setMainWindow(QWidget* main_window);

  public slots:
    void show() override;
    void hide() override;

  public slots:
    void Activate(int x, int y);
    void ContextMenu(int x, int y);

  signals:
    void NewTitle();
    void NewIcon();
    void NewToolTip();
    void NewStatus(const QString& status);

  private:
    // DBus property getters.
    QString category() const;
    QString id() const;
    QString title() const;
    QString status() const;
    int windowId() const;
    DBusImageVector iconPixmap() const;
    DBusToolTipStruct toolTip() const;

    // Helper methods
    bool registerToStatusNotifierWatcher();
    void unregisterFromStatusNotifierWatcher();

  private slots:
    void onServiceOwnerChanged(const QString& service, const QString& oldOwner, const QString& newOwner);

  private:
    QString m_dbusService;
    QString m_dbusPath;
    QPixmap m_currentIcon;
    QString m_toolTip;
    Status m_status;
    TrayIconMenu* m_menu;
    QDBusServiceWatcher* m_watcher;
    bool m_registered;
    int m_windowId;

    static int s_instanceCounter;
};

#endif // TRAYICONSTATUSNOTIFIER_H
