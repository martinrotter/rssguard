// trayiconstatusnotifier.cpp
#include "gui/tray/dbustrayicon.h"

#include <qdbusmetatype.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusServiceWatcher>

int TrayIconStatusNotifier::s_instanceCounter = 0;

QDBusArgument& operator<<(QDBusArgument& argument, const DBusToolTipStruct& toolTip) {
  argument.beginStructure();
  argument << toolTip.icon << toolTip.image << toolTip.title << toolTip.description;
  argument.endStructure();
  return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusToolTipStruct& toolTip) {
  argument.beginStructure();
  argument >> toolTip.icon >> toolTip.image >> toolTip.title >> toolTip.description;
  argument.endStructure();
  return argument;
}

TrayIconStatusNotifier::TrayIconStatusNotifier(const QString& id,
                                               const QString& title,
                                               const QPixmap& normal_icon,
                                               const QPixmap& plain_icon,
                                               QObject* parent)
  : TrayIcon(id, title, normal_icon, plain_icon, parent), m_iconName("dialog-information"), m_toolTip(title),
    m_status(Status::Active), m_menu(nullptr), m_watcher(nullptr), m_registered(false) {
  // Create unique DBus service and path
  m_dbusService =
    QString("org.kde.StatusNotifierItem-%1-%2").arg(QCoreApplication::applicationPid()).arg(++s_instanceCounter);
  m_dbusPath = QStringLiteral("/StatusNotifierItem");

  qDBusRegisterMetaType<DBusToolTipStruct>();

  // Register on session bus
  QDBusConnection bus = QDBusConnection::sessionBus();
  if (!bus.registerService(m_dbusService)) {
    qWarning() << "Failed to register DBus service:" << m_dbusService;
    return;
  }

  if (!bus.registerObject(m_dbusPath,
                          this,
                          QDBusConnection::ExportAllProperties | QDBusConnection::ExportAllSignals |
                            QDBusConnection::ExportAllSlots)) {
    qWarning() << "Failed to register DBus object at path:" << m_dbusPath;
    return;
  }

  // Watch for StatusNotifierWatcher service
  m_watcher = new QDBusServiceWatcher(QStringLiteral("org.kde.StatusNotifierWatcher"),
                                      bus,
                                      QDBusServiceWatcher::WatchForOwnerChange,
                                      this);
  connect(m_watcher, &QDBusServiceWatcher::serviceOwnerChanged, this, &TrayIconStatusNotifier::onServiceOwnerChanged);
}

TrayIconStatusNotifier::~TrayIconStatusNotifier() {
  unregisterFromStatusNotifierWatcher();
  QDBusConnection bus = QDBusConnection::sessionBus();
  bus.unregisterObject(m_dbusPath);
  bus.unregisterService(m_dbusService);
}

void TrayIconStatusNotifier::setToolTip(const QString& tool_tip) {
  if (m_toolTip != tool_tip) {
    m_toolTip = tool_tip;
    emit NewToolTip();
  }
}

void TrayIconStatusNotifier::setPixmap(const QPixmap& icon) {
  // Not used in simple version - we use icon name only
  Q_UNUSED(icon)
}

void TrayIconStatusNotifier::setStatus(Status status) {
  if (m_status != status) {
    m_status = status;
    emit NewStatus(this->status());
  }
}

void TrayIconStatusNotifier::setContextMenu(TrayIconMenu* menu) {
  m_menu = menu;
}

void TrayIconStatusNotifier::showMessage(const QString& title,
                                         const QString& message,
                                         MessageSeverity icon,
                                         int milliseconds_timeout_hint,
                                         const std::function<void()>& message_clicked_callback) {
  Q_UNUSED(message_clicked_callback)

  QDBusInterface notifications(QStringLiteral("org.freedesktop.Notifications"),
                               QStringLiteral("/org/freedesktop/Notifications"),
                               QStringLiteral("org.freedesktop.Notifications"));

  if (!notifications.isValid()) {
    return;
  }

  QString iconName;

  switch (icon) {
    case MessageSeverity::Information:
      iconName = QStringLiteral("dialog-information");
      break;
    case MessageSeverity::Warning:
      iconName = QStringLiteral("dialog-warning");
      break;
    case MessageSeverity::Critical:
      iconName = QStringLiteral("dialog-error");
      break;
    default:
      iconName = m_iconName;
      break;
  }

  notifications.call(QStringLiteral("Notify"),
                     m_title,
                     uint(0),
                     iconName,
                     title,
                     message,
                     QStringList(),
                     QVariantMap(),
                     milliseconds_timeout_hint);
}

bool TrayIconStatusNotifier::isAvailable() const {
  QDBusInterface watcher(QStringLiteral("org.kde.StatusNotifierWatcher"),
                         QStringLiteral("/StatusNotifierWatcher"),
                         QStringLiteral("org.kde.StatusNotifierWatcher"));
  return watcher.isValid();
}

void TrayIconStatusNotifier::show() {
  if (!m_registered) {
    if (registerToStatusNotifierWatcher()) {
      emit shown();
    }
  }
}

void TrayIconStatusNotifier::hide() {
  if (m_registered) {
    unregisterFromStatusNotifierWatcher();
    emit hidden();
  }
}

void TrayIconStatusNotifier::Activate(int x, int y) {
  Q_UNUSED(x)
  Q_UNUSED(y)
  emit activated();
}

void TrayIconStatusNotifier::ContextMenu(int x, int y) {
  if (m_menu) {
    if (m_menu->isVisible()) {
      m_menu->hide();
    }

    m_menu->popup(QPoint(x, y));
  }
}

QString TrayIconStatusNotifier::status() const {
  switch (m_status) {
    case Status::Active:
      return QStringLiteral("Active");
    case Status::Passive:
      return QStringLiteral("Passive");
    case Status::NeedsAttention:
      return QStringLiteral("NeedsAttention");
    default:
      return QStringLiteral("Active");
  }
}

DBusToolTipStruct TrayIconStatusNotifier::toolTip() const {
  DBusToolTipStruct toolTip;
  toolTip.icon = m_iconName;             // Use our icon
  toolTip.image = QList<QVariantList>(); // Empty image list
  toolTip.title = "test";
  toolTip.description = "testt";

  return toolTip;
}

bool TrayIconStatusNotifier::registerToStatusNotifierWatcher() {
  QDBusInterface watcher(QStringLiteral("org.kde.StatusNotifierWatcher"),
                         QStringLiteral("/StatusNotifierWatcher"),
                         QStringLiteral("org.kde.StatusNotifierWatcher"));

  if (!watcher.isValid()) {
    qWarning() << "StatusNotifierWatcher is not available";
    return false;
  }

  QDBusMessage reply = watcher.call(QStringLiteral("RegisterStatusNotifierItem"), m_dbusService);

  if (reply.type() == QDBusMessage::ErrorMessage) {
    qWarning() << "Failed to register with StatusNotifierWatcher:" << reply.errorMessage();
    return false;
  }

  m_registered = true;
  return true;
}

void TrayIconStatusNotifier::unregisterFromStatusNotifierWatcher() {
  m_registered = false;
}

void TrayIconStatusNotifier::onServiceOwnerChanged(const QString& service,
                                                   const QString& oldOwner,
                                                   const QString& newOwner) {
  Q_UNUSED(oldOwner)

  if (service == QStringLiteral("org.kde.StatusNotifierWatcher") && !newOwner.isEmpty()) {
    if (!m_registered) {
      registerToStatusNotifierWatcher();
    }
  }
}
