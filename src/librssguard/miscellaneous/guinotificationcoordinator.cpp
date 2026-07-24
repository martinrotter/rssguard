// For license of this file, see <project-root-folder>/LICENSE.md.

#include "miscellaneous/guinotificationcoordinator.h"

#include "core/feedsmodel.h"
#include "core/messagesmodel.h"
#include "gui/dialogs/formabout.h"
#include "gui/dialogs/formmain.h"
#include "gui/feedmessageviewer.h"
#include "gui/messagebox.h"
#include "gui/messagesview.h"
#include "gui/notifications/toastnotificationsmanager.h"
#include "gui/toolbars/statusbar.h"
#include "gui/tray/qttrayicon.h"
#include "miscellaneous/application.h"
#include "miscellaneous/feedreader.h"
#include "miscellaneous/iconfactory.h"
#include "miscellaneous/notificationfactory.h"
#include "miscellaneous/settings.h"
#include "miscellaneous/windowstaskbar.h"
#include "qtlinq/qtlinq.h"
#include "services/abstract/feed.h"

#include <QGuiApplication>
#include <QMetaObject>
#include <QPixmap>
#include <QTimer>
#include <QVariantMap>

#if defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
#include <QDBusConnection>
#include <QDBusMessage>
#endif

#if defined(Q_OS_WIN) && QT_VERSION_MAJOR == 6
#include <QWindow>
#include <QtGui/qpa/qplatformwindow_p.h>
#endif

GuiNotificationCoordinator::GuiNotificationCoordinator(Application* application)
  : QObject(), m_application(application), m_trayIcon(nullptr) {}

GuiNotificationCoordinator::~GuiNotificationCoordinator() {
  delete m_trayIcon;
}

TrayIcon* GuiNotificationCoordinator::trayIcon() {
  if (m_trayIcon == nullptr) {
    QPixmap tray_icon;
    QPixmap tray_icon_unread;
    QPixmap tray_icon_paused;

    const bool monochrome_icon =
      m_application->settings()->value(GROUP(GUI), SETTING(GUI::MonochromeTrayIcon)).toBool();
    const bool custom_colored_icon =
      m_application->settings()->value(GROUP(GUI), SETTING(GUI::CustomColoredTrayIcon)).toBool();
    const bool colored_unread_icon =
      m_application->settings()->value(GROUP(GUI), SETTING(GUI::ColoredBusyTrayIcon)).toBool();
    const bool show_unread_count =
      m_application->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersInTrayIcon)).toBool();
    QColor unread_text_color(Qt::GlobalColor::white);

    if (custom_colored_icon) {
      QColor background_color(m_application->settings()
                                ->value(GROUP(GUI), SETTING(GUI::CustomColoredTrayIconBackground))
                                .toString());
      unread_text_color =
        QColor(m_application->settings()->value(GROUP(GUI), SETTING(GUI::CustomColoredTrayIconText)).toString());

      if (IconFactory::ensureCustomColoredIcons(background_color)) {
        tray_icon = QPixmap(IconFactory::customColoredTrayIconPath());
        tray_icon_unread = show_unread_count ? QPixmap(IconFactory::customColoredTrayIconUnreadPath()) : tray_icon;
        tray_icon_paused = QPixmap(IconFactory::customColoredTrayIconUnreadPath());
      }
    }

    if (tray_icon.isNull() || tray_icon_unread.isNull() || tray_icon_paused.isNull()) {
      unread_text_color = QColor(Qt::GlobalColor::white);

      if (!custom_colored_icon && monochrome_icon) {
        tray_icon = QPixmap(APP_ICON_MONO_PATH);
        tray_icon_unread = colored_unread_icon
                             ? (show_unread_count ? QPixmap(APP_ICON_UNREAD_PATH) : QPixmap(APP_ICON_PATH))
                             : (show_unread_count ? QPixmap(APP_ICON_MONO_UNREAD_PATH) : QPixmap(APP_ICON_MONO_PATH));
        tray_icon_paused = QPixmap(APP_ICON_MONO_UNREAD_PATH);
      }
      else {
        tray_icon = QPixmap(APP_ICON_PATH);
        tray_icon_unread = show_unread_count ? QPixmap(APP_ICON_UNREAD_PATH) : QPixmap(APP_ICON_PATH);
        tray_icon_paused = QPixmap(APP_ICON_UNREAD_PATH);
      }
    }

    m_trayIcon = new QtTrayIcon(QSL(APP_LOW_NAME),
                                QSL(APP_NAME),
                                tray_icon,
                                tray_icon_unread,
                                tray_icon_paused,
                                unread_text_color,
                                m_application->mainForm());
    m_trayIcon->setMainWindow(m_application->mainForm());
    m_trayIcon->setContextMenu(m_application->mainForm()->trayMenu());
    m_trayIcon->setToolTip(QSL(APP_NAME));

    connect(m_trayIcon, &TrayIcon::activated, m_application->mainForm(), [this]() {
      m_application->mainForm()->switchVisibility();
    });
    connect(m_trayIcon, &TrayIcon::shown, m_application->feedReader()->feedsModel(), &FeedsModel::notifyWithCounts);
  }

  return m_trayIcon;
}

void GuiNotificationCoordinator::setMainForm() {
  if (auto* toasts = m_application->toastNotifications(); toasts != nullptr) {
    connect(toasts,
            &ToastNotificationsManager::dataChangeNotificationTriggered,
            m_application->mainForm()->tabWidget()->feedMessageViewer()->messagesView(),
            &MessagesView::reactOnExternalDataChange);
    connect(toasts,
            &ToastNotificationsManager::oneArticleSetReadUnreadById,
            m_application->mainForm()->tabWidget()->feedMessageViewer()->messagesView()->sourceModel(),
            &MessagesModel::setMessageReadById);
  }
}

void GuiNotificationCoordinator::showTrayIcon() {
  if (TrayIcon::isSystemTrayDesired()) {
    qDebugNN << LOGSEC_GUI << "User wants to have tray icon.";
    qWarningNN << LOGSEC_GUI << "Showing tray icon with little delay.";

    QTimer::singleShot(
#if defined(Q_OS_WIN)
      500,
#else
      3000,
#endif
      this,
      [this]() {
        if (trayIcon()->isAvailable()) {
          qWarningNN << LOGSEC_GUI << "Tray icon is available, showing now.";
          trayIcon()->show();
          QGuiApplication::setQuitOnLastWindowClosed(false);
        }
        else {
          m_application->feedReader()->feedsModel()->notifyWithCounts();
        }

        offerChanges();
        offerPolls();

#if defined(Q_OS_WIN) && QT_VERSION_MAJOR == 6
        using QWindowsWindow = QNativeInterface::Private::QWindowsWindow;
        if (auto window = m_application->mainForm()->windowHandle()->nativeInterface<QWindowsWindow>()) {
          window->setHasBorderInFullScreen(true);
        }
#endif
      });
  }
  else {
    m_application->feedReader()->feedsModel()->notifyWithCounts();
  }
}

void GuiNotificationCoordinator::deleteTrayIcon() {
  if (m_trayIcon != nullptr) {
    qDebugNN << LOGSEC_CORE << "Disabling tray icon, deleting it and raising main application window.";
    m_application->mainForm()->display();
    delete m_trayIcon;
    m_trayIcon = nullptr;
    QGuiApplication::setQuitOnLastWindowClosed(true);
  }
}

void GuiNotificationCoordinator::offerChanges() {
  if (m_application->isFirstRunCurrentVersion()) {
    showGuiMessage(Notification::Event::GeneralEvent,
                   {m_application->tr("Welcome"),
                    m_application
                      ->tr("Welcome to %1.\n\nPlease, check NEW stuff included in this\n"
                           "version by clicking this popup notification.")
                      .arg(QSL(APP_LONG_NAME)),
                    QSystemTrayIcon::MessageIcon::Information},
                   {},
                   GuiAction(m_application->tr("Go to changelog"),
                             m_application->mainForm(),
                             [this]() {
                               FormAbout(true, m_application->mainForm()).exec();
                             }),
                   nullptr);
  }
}

void GuiNotificationCoordinator::offerPolls() const {
  // Reserved for version-specific polls.
}

void GuiNotificationCoordinator::showGuiMessage(Notification::Event event,
                                                const GuiMessage& message,
                                                const GuiMessageDestination& destination,
                                                const GuiAction& action,
                                                QWidget* parent) {
  QMetaObject::invokeMethod(
    this,
    [this, event, message, destination, action, parent]() {
      showGuiMessageCore(event, message, destination, action, parent);
    },
    Qt::ConnectionType::QueuedConnection);
}

void GuiNotificationCoordinator::showGuiMessageCore(Notification::Event event,
                                                    const GuiMessage& message,
                                                    const GuiMessageDestination& destination,
                                                    const GuiAction& action,
                                                    QWidget* parent) {
  bool show_dialog = true;

  if (m_application->notifications()->areNotificationsEnabled()) {
    auto notification = m_application->notifications()->notificationForEvent(event);
    show_dialog = notification.dialogEnabled();

    if (notification.soundEnabled()) {
      notification.playSound(m_application);
    }

    if (notification.balloonEnabled() && destination.m_tray) {
      if (notification.event() == Notification::Event::ArticlesFetchingStarted &&
          m_application->mainForm() != nullptr && m_application->mainForm()->isActiveWindow() &&
          m_application->mainForm()->isVisible()) {
        return;
      }

      if (auto* toasts = m_application->toastNotifications(); toasts != nullptr) {
        toasts->showNotification(event, message, action);
      }
      else if (TrayIcon::isSystemTrayDesired() && m_trayIcon != nullptr && m_trayIcon->isAvailable()) {
        m_trayIcon->showMessage(message.m_title.simplified().isEmpty()
                                  ? Notification::nameForEvent(notification.event())
                                  : message.m_title,
                                message.m_message,
                                QtTrayIcon::convertIcon(message.m_type),
                                TRAY_ICON_BUBBLE_TIMEOUT,
                                action.m_action);
      }

      return;
    }
  }

  if (show_dialog && (destination.m_messageBox || message.m_type == QSystemTrayIcon::MessageIcon::Critical)) {
    MsgBox::show(parent,
                 QMessageBox::Icon(message.m_type),
                 message.m_title,
                 message.m_message,
                 {},
                 {},
                 QMessageBox::StandardButton::Ok,
                 QMessageBox::StandardButton::Ok,
                 {},
                 {MsgBox::CustomBoxAction{action.m_title, action.m_action}});
  }
  else if (destination.m_statusBar && m_application->mainForm()->statusBar() != nullptr &&
           m_application->mainForm()->statusBar()->isVisible()) {
    m_application->mainForm()->statusBar()->showMessage(message.m_message, 20000);
  }
  else {
    qDebugNN << LOGSEC_CORE << "Silencing GUI message:" << QUOTE_W_SPACE_DOT(message.m_message);
  }
}

void GuiNotificationCoordinator::showMessagesNumber(int unread_messages, bool any_feed_has_new_unread_messages) {
  Q_UNUSED(any_feed_has_new_unread_messages)

  if (m_trayIcon != nullptr) {
    m_trayIcon->setNumber(unread_messages);
  }

#if defined(Q_OS_MACOS) && QT_VERSION >= 0x060500
  m_application->setBadgeNumber(unread_messages);
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MACOS)
  const bool task_bar_count_enabled =
    m_application->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool();
  QDBusMessage signal = QDBusMessage::createSignal(QSL("/"), QSL("com.canonical.Unity.LauncherEntry"), QSL("Update"));
  signal << QSL("application://%1.desktop").arg(APP_REVERSE_NAME);
  QVariantMap properties;
  properties.insert("count", qint64(unread_messages));
  properties.insert("count-visible", task_bar_count_enabled && unread_messages > 0);
  signal << properties;
  QDBusConnection::sessionBus().send(signal);
#elif defined(Q_OS_WIN)
  const bool task_bar_count_enabled =
    m_application->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool();
  auto* taskbar = m_application->windowsTaskbar();
  if (m_application->mainForm() != nullptr && taskbar != nullptr && taskbar->isAvailable()) {
    const bool paused = m_application->settings()->value(GROUP(Feeds), SETTING(Feeds::PauseFeedFetching)).toBool();
    if ((task_bar_count_enabled && unread_messages > 0) || paused) {
      taskbar->setUnreadOverlayIcon(m_application->mainForm()->winId(),
                                    unread_messages,
                                    paused && unread_messages <= 0);
    }
    else {
      taskbar->clearOverlayIcon(m_application->mainForm()->winId());
    }

    if (!task_bar_count_enabled) {
      taskbar->clearProgress(m_application->mainForm()->winId());
    }
    else if (!m_application->feedReader()->isFeedUpdateRunning()) {
      if (paused) {
        taskbar->setProgressValue(m_application->mainForm()->winId(), 100, 100);
        taskbar->setProgressState(m_application->mainForm()->winId(), WindowsTaskbar::ProgressState::Paused);
      }
      else {
        taskbar->clearProgress(m_application->mainForm()->winId());
      }
    }
  }
#endif

  if (m_application->mainForm() != nullptr) {
    m_application->mainForm()
      ->setWindowTitle((m_application->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnWindow)).toBool() &&
                        unread_messages > 0)
                         ? QSL("[%2] %1").arg(QSL(APP_LONG_NAME), QString::number(unread_messages))
                         : QSL(APP_LONG_NAME));
  }
}

void GuiNotificationCoordinator::onFeedUpdatesStarted() {
#if defined(Q_OS_WIN)
  auto* taskbar = m_application->windowsTaskbar();
  if (m_application->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool() &&
      m_application->mainForm() != nullptr && taskbar != nullptr && taskbar->isAvailable()) {
    taskbar->setProgressState(m_application->mainForm()->winId(), WindowsTaskbar::ProgressState::Indeterminate);
  }
#endif
}

void GuiNotificationCoordinator::onFeedUpdatesProgress(const Feed* feed, int current, int total) {
  Q_UNUSED(feed)
#if defined(Q_OS_WIN)
  auto* taskbar = m_application->windowsTaskbar();
  if (m_application->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool() &&
      m_application->mainForm() != nullptr && taskbar != nullptr && taskbar->isAvailable()) {
    if (total > 0) {
      taskbar->setProgressValue(m_application->mainForm()->winId(), current, total);
    }
    else {
      taskbar->setProgressState(m_application->mainForm()->winId(), WindowsTaskbar::ProgressState::Indeterminate);
    }
  }
#endif
}

void GuiNotificationCoordinator::onFeedUpdatesFinished(const FeedDownloadResults& results) {
  const bool some_unquiet_feed = qlinq::from(results.updatedFeeds().keys()).any([](Feed* feed) {
    return !feed->isQuiet();
  });

  if (some_unquiet_feed) {
    GuiMessage message = {m_application->tr("Unread articles fetched"),
                          QString(),
                          QSystemTrayIcon::MessageIcon::NoIcon};
    if (m_application->toastNotifications() != nullptr) {
      message.m_feedFetchResults = results;
    }
    else {
      message.m_message = results.overview(10);
    }
    showGuiMessage(Notification::Event::NewUnreadArticlesFetched, message, {}, {}, nullptr);
  }

#if defined(Q_OS_WIN)
  auto* taskbar = m_application->windowsTaskbar();

  if (m_application->settings()->value(GROUP(GUI), SETTING(GUI::UnreadNumbersOnTaskBar)).toBool() &&
      m_application->mainForm() != nullptr && taskbar != nullptr && taskbar->isAvailable()) {
    if (results.erroredFeeds().isEmpty() ||
        !m_application->settings()->value(GROUP(GUI), SETTING(GUI::TaskbarErrorProgress)).toBool()) {
      taskbar->clearProgress(m_application->mainForm()->winId());
    }
    else {
      taskbar->setProgressValue(m_application->mainForm()->winId(), 100, 100);
      taskbar->setProgressState(m_application->mainForm()->winId(), WindowsTaskbar::ProgressState::Error);
    }
  }
#endif
}
