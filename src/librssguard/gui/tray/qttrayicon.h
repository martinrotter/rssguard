// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef QTTRAYICON_H
#define QTTRAYICON_H

#include "gui/tray/trayicon.h"

#include <QSystemTrayIcon>
#include <QTimer>

class QtTrayIcon : public TrayIcon {
    Q_OBJECT

  public:
    explicit QtTrayIcon(const QString& id,
                        const QString& title,
                        const QPixmap& normal_icon,
                        const QPixmap& plain_icon,
                        QObject* parent = nullptr);
    virtual ~QtTrayIcon();

    static TrayIcon::MessageSeverity convertIcon(QSystemTrayIcon::MessageIcon icon);
    static QSystemTrayIcon::MessageIcon convertIcon(TrayIcon::MessageSeverity severity);

  public:
    virtual void setToolTip(const QString& tool_tip);
    virtual void setPixmap(const QPixmap& icon);
    virtual void setStatus(Status status);
    virtual void setContextMenu(TrayIconMenu* menu);
    virtual void showMessage(const QString& title,
                             const QString& message,
                             MessageSeverity icon,
                             int milliseconds_timeout_hint,
                             const std::function<void()>& message_clicked_callback);
    virtual bool isAvailable() const;
    virtual void setMainWindow(QWidget* main_window);

  public slots:
    virtual void show();
    virtual void hide();

  private:
    QSystemTrayIcon* trayIcon();

  private:
    QSystemTrayIcon* m_trayIcon;
    QTimer m_tmrDoubleFire;
    QMetaObject::Connection m_connection;
};

#endif // QTTRAYICON_H
