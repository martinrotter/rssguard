// For license of this file, see <project-root-folder>/LICENSE.md.

#include "gui/tray/dbustrayicon.h"

#include "definitions/definitions.h"
#include "miscellaneous/application.h"

#include <qdbusmetatype.h>

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusServiceWatcher>

int TrayIconStatusNotifier::s_instanceCounter = 0;

QDBusArgument& operator<<(QDBusArgument& argument, const DBusImageStruct& image) {
  argument.beginStructure();
  argument << image.width << image.height << image.data;
  argument.endStructure();
  return argument;
}

const QDBusArgument& operator>>(const QDBusArgument& argument, DBusImageStruct& image) {
  argument.beginStructure();
  argument >> image.width >> image.height >> image.data;
  argument.endStructure();
  return argument;
}

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
  : TrayIcon(id, title, normal_icon, plain_icon, parent), m_toolTip(title), m_status(Status::Active), m_menu(nullptr),
    m_watcher(nullptr), m_registered(false), m_windowId(0) {
  // Create unique DBus service and path
  m_dbusService = QString("org.kde.StatusNotifierItem-%1-%2-%3")
                    .arg(id, QString::number(QCoreApplication::applicationPid()), QString::number(++s_instanceCounter));
  m_dbusPath = QSL("/StatusNotifierItem");

  qDBusRegisterMetaType<DBusImageStruct>();
  qDBusRegisterMetaType<DBusImageVector>();
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
  m_watcher =
    new QDBusServiceWatcher(QSL("org.kde.StatusNotifierWatcher"), bus, QDBusServiceWatcher::WatchForOwnerChange, this);
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
  m_currentIcon = icon;
  emit NewIcon();
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

  QDBusInterface notifications(QSL("org.freedesktop.Notifications"),
                               QSL("/org/freedesktop/Notifications"),
                               QSL("org.freedesktop.Notifications"));

  if (!notifications.isValid()) {
    return;
  }

  QString icon_name;

  switch (icon) {
    case MessageSeverity::Warning:
      icon_name = QSL("dialog-warning");
      break;
    case MessageSeverity::Critical:
      icon_name = QSL("dialog-error");
      break;

    case MessageSeverity::Information:
    default:
      icon_name = QSL("dialog-information");
      break;
  }

  notifications.call(QSL("Notify"),
                     m_title,
                     uint(0),
                     icon_name,
                     title,
                     message,
                     QStringList(),
                     QVariantMap(),
                     milliseconds_timeout_hint);
}

bool TrayIconStatusNotifier::isAvailable() const {
  QDBusInterface watcher(QSL("org.kde.StatusNotifierWatcher"),
                         QSL("/StatusNotifierWatcher"),
                         QSL("org.kde.StatusNotifierWatcher"));
  return watcher.isValid();
}

void TrayIconStatusNotifier::setMainWindow(QWidget* main_window) {
  m_windowId = main_window == nullptr ? 0 : int(main_window->winId());
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

QString TrayIconStatusNotifier::category() const {
  return QSL("Communications");
}

QString TrayIconStatusNotifier::id() const {
  return m_id;
}

QString TrayIconStatusNotifier::title() const {
  return m_title;
}

QString TrayIconStatusNotifier::status() const {
  switch (m_status) {
    case Status::Active:
      return QSL("Active");
    case Status::Passive:
      return QSL("Passive");
    case Status::NeedsAttention:
      return QSL("NeedsAttention");
    default:
      return QSL("Active");
  }
}

int TrayIconStatusNotifier::windowId() const {
  return m_windowId;
}

DBusImageVector TrayIconStatusNotifier::iconPixmap() const {
  DBusImageVector imageVector;

  if (!m_currentIcon.isNull()) {
    QImage image = m_currentIcon.toImage().convertToFormat(QImage::Format_ARGB32);

    DBusImageStruct imageStruct;
    imageStruct.width = image.width();
    imageStruct.height = image.height();

    // Allocate buffer for big-endian ARGB data
    imageStruct.data.resize(image.width() * image.height() * 4);

    // Convert to network byte order (big-endian)
    qToBigEndian<quint32>(reinterpret_cast<const quint32*>(image.constBits()),
                          image.width() * image.height(),
                          reinterpret_cast<quint32*>(imageStruct.data.data()));

    imageVector.append(imageStruct);
  }

  return imageVector;
}

DBusToolTipStruct TrayIconStatusNotifier::toolTip() const {
  DBusToolTipStruct toolTip;
  toolTip.icon = "dialog-information"; // Use our icon
  // toolTip.image = QList<QVariantList>(); // Empty image list
  toolTip.title = m_title;
  toolTip.description = m_toolTip;

  return toolTip;
}

bool TrayIconStatusNotifier::registerToStatusNotifierWatcher() {
  QDBusInterface watcher(QSL("org.kde.StatusNotifierWatcher"),
                         QSL("/StatusNotifierWatcher"),
                         QSL("org.kde.StatusNotifierWatcher"));

  if (!watcher.isValid()) {
    qWarning() << "StatusNotifierWatcher is not available";
    return false;
  }

  QDBusMessage reply = watcher.call(QSL("RegisterStatusNotifierItem"), m_dbusService);

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

  if (service == QSL("org.kde.StatusNotifierWatcher") && !newOwner.isEmpty()) {
    if (!m_registered) {
      registerToStatusNotifierWatcher();
    }
  }
}
