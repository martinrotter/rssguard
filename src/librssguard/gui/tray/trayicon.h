// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef TRAYICON_H
#define TRAYICON_H

#include <QFont>
#include <QIcon>
#include <QMenu>
#include <QObject>
#include <QPixmap>
#include <QString>

#define TRAY_ICON_BUBBLE_TIMEOUT 2000

class TrayIconMenu : public QMenu {
    Q_OBJECT

  public:
    explicit TrayIconMenu(const QString& title, QWidget* parent);

  protected:
    bool event(QEvent* event);
};

class TrayIcon : public QObject {
    Q_OBJECT

  public:
    enum class Status {
      Active,
      Passive,
      NeedsAttention
    };

    enum class MessageSeverity {
      NoIcon,
      Information,
      Warning,
      Critical
    };

    explicit TrayIcon(const QString& id,
                      const QString& title,
                      const QPixmap& normal_icon,
                      const QPixmap& plain_icon,
                      QObject* parent = nullptr);

    // Base API.
    virtual void setToolTip(const QString& tool_tip) = 0;
    virtual void setPixmap(const QPixmap& icon) = 0;
    virtual void setStatus(Status status) = 0;
    virtual void setContextMenu(TrayIconMenu* menu) = 0;

    virtual void showMessage(const QString& title,
                             const QString& message,
                             MessageSeverity icon = MessageSeverity::Information,
                             int milliseconds_timeout_hint = TRAY_ICON_BUBBLE_TIMEOUT,
                             const std::function<void()>& message_clicked_callback = nullptr) = 0;

    virtual bool isAvailable() const = 0;

    // Common API.
    void setNumber(int number = 0);

    // Returns true if user wants to have tray icon displayed.
    static bool isSystemTrayDesired();

  public slots:
    virtual void show() = 0;
    virtual void hide() = 0;

  signals:
    void shown();
    void hidden();
    void activated();
    void messageClicked();

  private:
    QString m_id;
    QString m_title;
    QPixmap m_normalIcon;
    QPixmap m_plainIcon;
    QFont m_font = QFont();
};

#endif // TRAYICON_H
