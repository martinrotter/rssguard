// For license of this file, see <project-root-folder>/LICENSE.md.

#ifndef GUINOTIFICATIONCOORDINATOR_H
#define GUINOTIFICATIONCOORDINATOR_H

#include "core/feeddownloader.h"
#include "miscellaneous/notification.h"

#include <QObject>
#include <QPointer>

class Application;
class Feed;
class TrayIcon;
class QWidget;
struct GuiAction;
struct GuiMessage;
struct GuiMessageDestination;

class GuiNotificationCoordinator : public QObject {
  public:
    explicit GuiNotificationCoordinator(Application* application);
    ~GuiNotificationCoordinator() override;

    TrayIcon* trayIcon();
    void setMainForm();
    void showTrayIcon();
    void deleteTrayIcon();
    void offerChanges();
    void offerPolls() const;
    void showGuiMessage(Notification::Event event,
                        const GuiMessage& message,
                        const GuiMessageDestination& destination,
                        const GuiAction& action,
                        QWidget* parent);
    void showMessagesNumber(int unread_messages, bool any_feed_has_new_unread_messages);
    void onFeedUpdatesStarted();
    void onFeedUpdatesProgress(const Feed* feed, int current, int total);
    void onFeedUpdatesFinished(const FeedDownloadResults& results);

  private:
    void showGuiMessageCore(Notification::Event event,
                            const GuiMessage& message,
                            const GuiMessageDestination& destination,
                            const GuiAction& action,
                            QWidget* parent);

  private:
    Application* m_application;
    QPointer<TrayIcon> m_trayIcon;
};

#endif // GUINOTIFICATIONCOORDINATOR_H
