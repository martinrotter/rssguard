// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef SYSTEMTRAYICON_H
#define SYSTEMTRAYICON_H

#include <QSystemTrayIcon>

#include "definitions/definitions.h"

#include <QMenu>
#include <QPixmap>

#include <functional>

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

class SystemTrayIcon : public QSystemTrayIcon {
  Q_OBJECT

  public:

    // Constructors and destructors.
    explicit SystemTrayIcon(const QString& normal_icon, const QString& plain_icon, FormMain* parent = nullptr);
    virtual ~SystemTrayIcon();

    // Sets the number to be visible in the tray icon, number <= 0 removes it.
    void setNumber(int number = -1, bool any_new_message = false);

    void showMessage(const QString& title, const QString& message, MessageIcon icon = Information,
                     int milliseconds_timeout_hint = TRAY_ICON_BUBBLE_TIMEOUT, std::function<void()> functor = nullptr);

    // Returns true if tray area is available and icon can be displayed.
    static bool isSystemTrayAreaAvailable();

    // Returns true if user wants to have tray icon displayed.
    static bool isSystemTrayDesired();

    // Determines whether balloon tips are enabled or not on tray icons.
    static bool areNotificationsEnabled();

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

    QMetaObject::Connection m_connection;
};

#endif // SYSTEMTRAYICON_H
